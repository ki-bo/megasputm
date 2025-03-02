/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * MEGASPUTM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stdlib.h>
#include <stdint.h>

void c64_sound_init(void);

void c64_start_sound(uint8_t sound_id);
void c64_stop_sound(uint8_t sound_id);
void c64_stop_music(void);
void c64_stop_all_sounds(void);

uint8_t c64_sound_is_playing(uint8_t sound_id);
void c64_sound_update(); // $481B
void c64_sound_handle_play_triggers(void);


