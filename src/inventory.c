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

#include "inventory.h"
#include "dma.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdlib.h>

#pragma clang section bss="zdata"

struct inventory_display inv_ui_entries;

/**
  * @defgroup inv_init Inventory Init Functions
  * @{
  */
#pragma clang section text="code_init"  rodata="cdata_init" data="data_init" bss="bss_init"

void inv_init()
{
}

 ///@} inv_init

/**
  * @defgroup inv_public Inventory Public Functions
  * @{
  */
#pragma clang section text="code_main"  rodata="cdata_main" data="data_main" bss="zdata"

/**
  * @brief Add an object to the inventory.
  * 
  * @param object Pointer to the object chunk.
  * @param size Amount of bytes to copy.
  */
void inv_add_object(uint8_t local_object_id)
{
  uint8_t inv_slot = 0xff;

  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    if (!vm_state.inv_objects[i]) {
      inv_slot = i;
      break;
    }
  }
  if (inv_slot == 0xff) {
    fatal_error(ERR_TOO_MANY_INVENTORY_OBJECTS);
  }

  __auto_type obj_hdr = (struct object_code __huge *)(res_get_huge_ptr(obj_page[local_object_id]) + obj_offset[local_object_id]);
  uint16_t size = obj_hdr->chunk_size;

  inv_copy_object_data(inv_slot, obj_hdr);
  ++vm_state.inv_num_objects;
}

void inv_copy_object_data(uint8_t target_pos, struct object_code __huge *object)
{
  SAVE_DS_AUTO_RESTORE
  UNMAP_DS

  uint16_t size = object->chunk_size;

  void *inv_obj = malloc(size);
  if (!inv_obj) {
    fatal_error(ERR_OUT_OF_HEAP_MEMORY);
  }
  memcpy_chipram((void __far *)inv_obj, (void __far *)object, size);

  vm_state.inv_objects[target_pos] = inv_obj;
}

void inv_remove_object(uint8_t position)
{
  SAVE_DS_AUTO_RESTORE
  UNMAP_DS

  free(vm_state.inv_objects[position]);
  vm_state.inv_objects[position] = NULL;
}

struct object_code *inv_get_object_by_id(uint16_t global_object_id)
{
  UNMAP_DS

  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    __auto_type obj_hdr = vm_state.inv_objects[i];
    if (obj_hdr && obj_hdr->id == global_object_id) {
      return obj_hdr;
    }
  }

  return NULL;
}

uint8_t inv_object_available(uint16_t global_object_id)
{
  SAVE_DS_AUTO_RESTORE
  return inv_get_position_by_id(global_object_id) != 0xff;
}

const char *inv_get_object_name(uint8_t position)
{
  UNMAP_DS
  return (const char *)vm_state.inv_objects[position] + vm_state.inv_objects[position]->name_offset;
}

uint16_t inv_get_global_object_id(uint8_t position)
{
  UNMAP_DS
  return vm_state.inv_objects[position]->id;
}

uint8_t inv_get_position_by_id(uint16_t global_object_id)
{
  UNMAP_DS

  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    __auto_type obj_hdr = vm_state.inv_objects[i];
    if (obj_hdr && obj_hdr->id == global_object_id) {
      return i;
    }
  }

  return 0xff;
}

/**
  * @brief Updates the inventory items to be displayed on screen.
  *
  * This function will update inv_ui_entries with items that should be displayed
  * on the screen. It filters the inventory starting at inventory item inventory_pos for entries
  * that match the actor in VAR_SELECTED_ACTOR.
  *
  * It also fills the inv_ui_entries structure with the previous and next ids that match
  * VAR_SELECTED_ACTOR. If no previous or next entries are available, the corresponding id will be
  * set to 0xff. This information can be used to hide or display the inventory scroll buttons.
  *
  * Code section: code_main
  */
void inv_update_displayed_inventory(void)
{
  UNMAP_DS

  uint8_t actor_id            = vm_read_var8(VAR_SELECTED_ACTOR);
  uint8_t owner_inventory_pos = 0;
  uint8_t num_objects         = vm_state.inv_num_objects;

  inv_ui_entries.num_entries = 0;
  inv_ui_entries.prev_id     = 0xff;
  inv_ui_entries.next_id     = 0xff;

  do {
    uint8_t inv_obj_processed = 0;
    for (uint8_t i = 0; i < MAX_INVENTORY && inv_obj_processed < vm_state.inv_num_objects; ++i) {
      __auto_type object_ptr = vm_state.inv_objects[i];
      
      if (!object_ptr) {
        continue;
      }

      ++inv_obj_processed;

      uint16_t object_id    = object_ptr->id;
      uint8_t  object_owner = vm_state.global_game_objects[object_id] & 0x0f;
      
      if (object_owner == actor_id) {
        if (owner_inventory_pos < inventory_pos) {
          inv_ui_entries.prev_id = i;
        }
        else if (inv_ui_entries.num_entries < 4) {
          inv_ui_entries.displayed_ids[inv_ui_entries.num_entries] = i;
          ++inv_ui_entries.num_entries;
        }
        else {
          inv_ui_entries.next_id = i;
          break;
        }
        ++owner_inventory_pos;
      }
    }

    if (inv_ui_entries.num_entries == 0) {
      if (inventory_pos == 0) {
        return;
      }
      inv_ui_entries.prev_id = 0xff;
      inventory_pos = 0;
      owner_inventory_pos = 0;
    }
  }
  while (inv_ui_entries.num_entries == 0);
}

///@} inv_public
