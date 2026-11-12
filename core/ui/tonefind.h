// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The RadioFoundry Authors
// Find an FM carrier's frequency and its unknown sub-audio tone (docs/spec/radio-features.md).
// Pure state machine: the caller passes what it observes and performs the action returned. No waits, no RTOS calls,
// no headers from the radio core, so the host tests drive it one step at a time.
#ifndef UI_TONEFIND_H
#define UI_TONEFIND_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "hal_radio.h"

// A candidate needs this long before tone_ok is read: the RX tone changed, and the detector and the radio core's
// debounce both need time to settle (radio_sm.c follow_tone).
#define TONEFIND_CANDIDATE_MS	250
// Give up looking for a carrier after this long, so a quiet band does not leave a scan running forever.
#define TONEFIND_SEEK_MS	60000

typedef enum {
	TONEFIND_IDLE,
	TONEFIND_SEEK,		// the VFO scan is running; wait for a carrier to hold on
	TONEFIND_SETTLE,	// the scan was stopped on that carrier; try the first candidate
	TONEFIND_MATCH,		// trying the standard sub-audio list one entry at a time
	TONEFIND_FOUND,		// stay here with the decoded result
} tonefind_phase_t;

typedef enum {
	TONEFIND_A_NONE,
	TONEFIND_A_SCAN_ON,	// start (or resume) the VFO scan, with no RX tone
	TONEFIND_A_SCAN_OFF,	// stop the scan on this frequency
	TONEFIND_A_TONE,	// set the RX tone to .tone
	TONEFIND_A_RESTORE,	// put the RX tone back to what it was before the search
} tonefind_act_t;

typedef struct {
	uint8_t phase;
	uint16_t candidate;	// 1..TONES_PICKER_COUNT-1 (entry 0 is "no tone")
	uint32_t hz;		// the carrier the scan stopped on
	subtone_t found;
	subtone_t saved;	// the RX tone before the search, restored on exit
	uint32_t since_ms;
	uint32_t started_ms;
} tonefind_t;

typedef struct {
	uint8_t act;
	subtone_t tone;		// TONEFIND_A_TONE: the candidate to try
} tonefind_action_t;

// Begin: remember the RX tone. False while busy (transmitting or a hotspot). The caller starts the VFO scan itself.
bool tonefind_start(tonefind_t *f, subtone_t rx_tone, bool busy, uint32_t now_ms);
// Every UI pass. held: the VFO scan is holding on a carrier at hz. tone_ok: the RX tone now set is heard.
tonefind_action_t tonefind_poll(tonefind_t *f, bool held, uint32_t hz, bool tone_ok, bool busy, uint32_t now_ms);
// Leave, restoring the RX tone. Safe when idle.
tonefind_action_t tonefind_cancel(tonefind_t *f);
// Progress through the standard list, 0..100, for the screen.
uint8_t tonefind_progress(const tonefind_t *f);

#endif
