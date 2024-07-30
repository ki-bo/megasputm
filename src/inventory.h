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

struct inventory_display {
    uint8_t num_entries;
    uint8_t prev_id;
    uint8_t displayed_ids[4];
    uint8_t next_id;
};

extern struct inventory_display inv_ui_entries;

// code_init functions
void inv_init();

//code_main functions
void inv_add_object(uint8_t local_object_id);
void inv_copy_object_data(uint8_t target_pos, struct object_code __huge *object);
void inv_remove_object(uint8_t position);
struct object_code *inv_get_object_by_id(uint16_t global_object_id);
uint8_t inv_object_available(uint16_t global_object_id);
const char *inv_get_object_name(uint8_t position);
uint16_t inv_get_global_object_id(uint8_t position);
uint8_t inv_get_position_by_id(uint16_t global_object_id);
void inv_update_displayed_inventory(void);
void inv_set_name(uint16_t global_object_id, const char *name);
