// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The RadioFoundry Authors
// The search runs the existing VFO scan until it holds on a carrier, stops there, then walks the standard CTCSS/DCS
// list: each candidate becomes the VFO's RX tone, and the radio core's tone_ok says whether that tone is on air.
// Trying one tone at a time needs no board-specific audio path, so every radio with CONFIG_SUBTONE can do it.
#include "config.h"

#if CONFIG_TONE_FIND

#include "ui/tonefind.h"
#include "radio/tones.h"

static tonefind_action_t act_none(void)
{
	tonefind_action_t a = { TONEFIND_A_NONE, { TONE_NONE, 0 } };
	return a;
}

static tonefind_action_t act_simple(uint8_t act)
{
	tonefind_action_t a = act_none();
	a.act = act;
	return a;
}

static tonefind_action_t act_tone(subtone_t t)
{
	tonefind_action_t a = act_none();
	a.act = TONEFIND_A_TONE;
	a.tone = t;
	return a;
}

bool tonefind_start(tonefind_t *f, subtone_t rx_tone, bool busy, uint32_t now_ms)
{
	*f = (tonefind_t){ 0 };
	if (busy) {
		return false;
	}
	f->saved = rx_tone;
	f->phase = TONEFIND_SEEK;
	f->started_ms = now_ms;
	f->since_ms = now_ms;
	return true;
}

tonefind_action_t tonefind_poll(tonefind_t *f, bool held, uint32_t hz, bool tone_ok, bool busy, uint32_t now_ms)
{
	switch (f->phase) {
	case TONEFIND_SEEK:
		if (busy || (int32_t)(now_ms - f->started_ms) >= TONEFIND_SEEK_MS) {
			f->phase = TONEFIND_IDLE;
			return act_simple(TONEFIND_A_SCAN_OFF);
		}
		if (held) {
			f->hz = hz;
			f->phase = TONEFIND_SETTLE;
			return act_simple(TONEFIND_A_SCAN_OFF);
		}
		return act_none();

	case TONEFIND_SETTLE:
		f->candidate = 1;
		f->phase = TONEFIND_MATCH;
		f->since_ms = now_ms;
		return act_tone(tones_picker(f->candidate));

	case TONEFIND_MATCH:
		if ((int32_t)(now_ms - f->since_ms) < TONEFIND_CANDIDATE_MS) {
			return act_none();
		}
		if (tone_ok) {
			f->found = tones_picker(f->candidate);
			f->phase = TONEFIND_FOUND;
			return act_tone(f->found);
		}
		if (++f->candidate >= TONES_PICKER_COUNT) {
			// No sub-audio on this carrier: clear the RX tone and look for the next one.
			f->phase = TONEFIND_SEEK;
			f->started_ms = now_ms;
			return act_simple(TONEFIND_A_SCAN_ON);
		}
		f->since_ms = now_ms;
		return act_tone(tones_picker(f->candidate));

	default:
		return act_none();
	}
}

tonefind_action_t tonefind_cancel(tonefind_t *f)
{
	f->phase = TONEFIND_IDLE;
	return act_simple(TONEFIND_A_RESTORE);
}

uint8_t tonefind_progress(const tonefind_t *f)
{
	if (f->phase == TONEFIND_FOUND) {
		return 100;
	}
	if (f->phase != TONEFIND_MATCH) {
		return 0;
	}
	return (uint8_t)((uint32_t)f->candidate * 100u / (TONES_PICKER_COUNT - 1u));
}

#endif
