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

#include "vm.h"
#include <stdint.h>

#define INDEX_FILE_CHKS 0xdd69

#define NUM_GAME_OBJECTS 780
#define NUM_ROOMS 61
#define NUM_COSTUMES 40
#define NUM_SCRIPTS 179
#define NUM_SOUNDS 120

#define MAX_DISKS 2

extern char disk_header[29];
extern uint16_t index_lang_chks[LANG_COUNT];
