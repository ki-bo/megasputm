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
#include "vm.h"

extern uint8_t __attribute__((zpage)) parallel_script_count;

// code_init functions
void script_init(void);

// code_main functions
void script_schedule_init_script(void);
uint8_t script_execute_slot(uint8_t slot);
uint16_t script_get_current_pc(void);
void script_break(void);
uint8_t script_start(uint8_t script_id);
void script_execute_room_script(uint16_t room_script_offset);
void script_execute_object_script(uint8_t verb, uint16_t object, uint8_t background);
void script_stop_slot(uint8_t slot);
void script_stop(uint8_t script_id);
void script_print_slot_table(void);
uint8_t script_is_room_object_script(uint8_t slot);
