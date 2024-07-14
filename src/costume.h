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

//-----------------------------------------------------------------------------------------------

enum {
  ANIM_WALKING    =  0,
  ANIM_STANDING   =  4,
  ANIM_HEAD       =  8,
  ANIM_MOUTH_OPEN = 12,
  ANIM_MOUTH_SHUT = 16,
  ANIM_TALKING    = 20
};

//-----------------------------------------------------------------------------------------------
struct costume_header {
    uint16_t chunk_size;
    uint16_t unused1;
    uint8_t  num_animations;
    uint8_t  disable_mirroring_and_format;
    uint8_t  color;
    uint16_t animation_commands_offset;
    uint16_t level_table_offsets[16];
    uint16_t animation_offsets[];
};

struct costume_cel{
  uint16_t width;
  uint16_t height;
  int16_t offset_x;
  int16_t offset_y;
  int16_t move_x;
  int16_t move_y;
};
