#pragma once

#include <stdlib.h>
#include <stdint.h>

void c64_sound_init(void);

void c64_start_sound(uint8_t sound_id);
void c64_stop_sound(uint8_t sound_id);
void c64_stop_all_sounds();

uint8_t c64_sound_is_playing(uint8_t sound_id);
void c64_sound_update(); // $481B
void c64_sound_handle_play_triggers(void);


