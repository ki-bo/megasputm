/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
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

#include <stdint.h>

enum {
    INPUT_BUTTON_LEFT  = 1,
    INPUT_BUTTON_RIGHT = 2
};

extern uint16_t input_cursor_x;
extern uint8_t  input_cursor_y;
extern uint8_t  input_button_pressed;
extern uint8_t  input_key_pressed;

#define HOTSPOT_OFFSET_X 7
#define HOTSPOT_OFFSET_Y 7
#define INPUT_CURSOR_X2 (U8(input_cursor_x >> 1))

// code_init functions
void input_init(void);

// code_main functions
void input_update(void);
