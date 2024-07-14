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

struct walk_box {
  uint8_t top_y;
  uint8_t bottom_y;
  uint8_t topleft_x;
  uint8_t topright_x;
  uint8_t bottomleft_x;
  uint8_t bottomright_x;
  uint8_t mask;
  uint8_t classes;
};

enum walk_box_class {
  WALKBOX_CLASS_BOX_LOCKED    = 0x40,
  WALKBOX_CLASS_BOX_INVISIBLE = 0x80
};

extern uint8_t          num_walk_boxes;
extern struct walk_box *walk_boxes;
extern uint8_t         *walk_box_matrix;

// main functions
uint8_t walkbox_get_next_box(uint8_t cur_box, uint8_t target_box);
uint8_t walkbox_get_box_masking(uint8_t box_id);
uint8_t walkbox_get_box_classes(uint8_t box_id);
uint8_t walkbox_correct_position_to_closest_box(uint8_t *x, uint8_t *y);
uint16_t walkbox_get_corrected_box_position(struct walk_box *box, uint8_t *x, uint8_t *y);
void walkbox_find_closest_box_point(uint8_t box_id, uint8_t *px, uint8_t *py);
