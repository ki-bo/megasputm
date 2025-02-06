#pragma once

#include <stdlib.h>
#include <stdint.h>


enum sid_reg_t {
	FREQ_VOICE1,
	FREQ_VOICE2,
	FREQ_VOICE3,
	FREQ_FILTER,
	PULSE_VOICE1,
	PULSE_VOICE2,
	PULSE_VOICE3
};

void c64_sound_init(void);
void c64_sound_fin(void);

void c64_startSound(uint8_t sound_id);
void c64_stopSound(uint8_t sound_id);
void c64_stopAllSounds();

uint8_t c64_sound_is_playing(uint8_t sound_id);
void c64_sound_update(); // $481B
void c64_sound_handle_play_triggers(void);


