// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The RadioFoundry Authors
// The unknown-frequency / unknown-tone finder's state machine, without a radio (docs/spec/radio-features.md).
#include <stdio.h>
#include "config.h"
#include "ui/tonefind.h"
#include "radio/tones.h"

static int failures;

#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); failures++; } } while (0)

static bool same(subtone_t a, subtone_t b)
{
	return a.type == b.type && a.value == b.value;
}

static void find_first(tonefind_t *f, uint32_t hz)
{
	(void)tonefind_start(f, (subtone_t){ TONE_CTCSS, 1000 }, false, 0);
	CHECK(tonefind_poll(f, true, hz, false, false, 100).act == TONEFIND_A_SCAN_OFF);
	CHECK(tonefind_poll(f, false, 0, false, false, 200).act == TONEFIND_A_TONE);
}

int main(void)
{
	tonefind_t f;
	tonefind_action_t a;
	subtone_t before = { TONE_CTCSS, 1000 };

	// A transmission or a hotspot is not a time to start a search
	CHECK(!tonefind_start(&f, before, true, 0));
	CHECK(f.phase == TONEFIND_IDLE);
	CHECK(tonefind_start(&f, before, false, 0));
	CHECK(f.phase == TONEFIND_SEEK);
	CHECK(same(f.saved, before));

	// Nothing heard: keep the scan running
	CHECK(tonefind_poll(&f, false, 0, false, false, 100).act == TONEFIND_A_NONE);

	// The scan holds on a carrier: stop there, then try the first CTCSS candidate
	a = tonefind_poll(&f, true, 439500000, false, false, 200);
	CHECK(a.act == TONEFIND_A_SCAN_OFF);
	CHECK(f.hz == 439500000);
	CHECK(f.phase == TONEFIND_SETTLE);
	a = tonefind_poll(&f, false, 0, false, false, 300);
	CHECK(a.act == TONEFIND_A_TONE);
	CHECK(same(a.tone, tones_picker(1)));
	CHECK(f.phase == TONEFIND_MATCH);

	// The detector needs its settle time before tone_ok means anything
	CHECK(tonefind_poll(&f, false, 0, true, false, 400).act == TONEFIND_A_NONE);
	a = tonefind_poll(&f, false, 0, true, false, 600);
	CHECK(a.act == TONEFIND_A_TONE);
	CHECK(same(a.tone, tones_picker(1)));
	CHECK(f.phase == TONEFIND_FOUND);
	CHECK(same(f.found, tones_picker(1)));
	CHECK(tonefind_progress(&f) == 100);
	// Stay on the result: no more actions until the user saves or leaves
	CHECK(tonefind_poll(&f, false, 0, false, false, 1000).act == TONEFIND_A_NONE);
	a = tonefind_cancel(&f);
	CHECK(a.act == TONEFIND_A_RESTORE);
	CHECK(f.phase == TONEFIND_IDLE);

	// Nothing in the list matches: clear the RX tone and look for another carrier
	find_first(&f, 145500000);
	CHECK(f.candidate == 1);
	for (uint16_t i = 1; i < TONES_PICKER_COUNT; i++) {
		uint32_t now = 300u + (uint32_t)i * TONEFIND_CANDIDATE_MS;
		a = tonefind_poll(&f, false, 0, false, false, now);
		if (i + 1u < TONES_PICKER_COUNT) {
			CHECK(a.act == TONEFIND_A_TONE);
			CHECK(same(a.tone, tones_picker((uint16_t)(i + 1u))));
		}
	}
	CHECK(a.act == TONEFIND_A_SCAN_ON);
	CHECK(f.phase == TONEFIND_SEEK);

	// A quiet band gives up rather than leaving a scan on forever
	CHECK(tonefind_start(&f, before, false, 0));
	a = tonefind_poll(&f, false, 0, false, false, TONEFIND_SEEK_MS);
	CHECK(a.act == TONEFIND_A_SCAN_OFF);
	CHECK(f.phase == TONEFIND_IDLE);

	// Transmission ends a running search the same way
	CHECK(tonefind_start(&f, before, false, 0));
	CHECK(tonefind_poll(&f, false, 0, false, true, 100).act == TONEFIND_A_SCAN_OFF);
	CHECK(f.phase == TONEFIND_IDLE);

	if (failures) {
		printf("# FAILED tonefind (%d)\n", failures);
		return 1;
	}
	printf("# PASSED tonefind\n");
	return 0;
}
