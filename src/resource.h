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

#include <stdint.h>

enum res_type_t {
    RES_TYPE_NONE = 0,
    RES_TYPE_ROOM,
    RES_TYPE_COSTUME,
    RES_TYPE_SCRIPT,
    RES_TYPE_SOUND,
    RES_TYPE_HEAP
};

enum res_type_flags_t {
  RES_TYPE_MASK   = 0x07,
  RES_LOCKED_MASK = 0x80,
  RES_ACTIVE_MASK = 0x40
};

// code_init functions
void res_init(void);

// code_main functions
uint8_t res_provide(uint8_t type_and_flags, uint8_t id, uint8_t hint);
void res_deactivate_and_unlock_all(void);
uint8_t __huge *res_get_huge_ptr(uint8_t slot);
void res_reset(void);
void res_lock(uint8_t type, uint8_t id, uint8_t hint);
void res_unlock(uint8_t type, uint8_t id, uint8_t hint);
void res_activate(uint8_t type, uint8_t id, uint8_t hint);
void res_deactivate(uint8_t type, uint8_t id, uint8_t hint);
void res_activate_slot(uint8_t slot);
void res_deactivate_slot(uint8_t slot);
uint8_t res_get_locked_resources(uint16_t *locked_resources, uint8_t max_entries);
uint8_t res_get_flags(uint8_t slot);
uint8_t res_get_num_locked(void);
uint16_t res_get_type_and_index(uint8_t slot);
uint8_t res_reserve_heap(uint8_t size_blocks);
void res_free_heap(uint8_t slot);
