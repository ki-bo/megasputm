#include "inventory.h"
#include "dma.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"

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
  __auto_type obj_hdr = (struct object_code __huge *)(res_get_huge_ptr(obj_page[local_object_id]) + obj_offset[local_object_id]);
  uint16_t size = obj_hdr->chunk_size;

  uint16_t free_bytes = INVENTORY_BASE + INVENTORY_SIZE - (uint16_t)vm_state.inv_next_free;
  if (free_bytes < size) {
    fatal_error(ERR_OUT_OF_INVENTORY_SPACE);
  }
  else if (vm_state.inv_num_objects == MAX_INVENTORY) {
    fatal_error(ERR_TOO_MANY_INVENTORY_OBJECTS);
  }

  memcpy_bank((void __far *)vm_state.inv_next_free, (void __far *)obj_hdr, size);

  vm_state.inv_objects[vm_state.inv_num_objects] = (struct object_code *)vm_state.inv_next_free;
  ++vm_state.inv_num_objects;
  vm_state.inv_next_free += size;
}

struct object_code *inv_get_object_by_id(uint16_t global_object_id)
{
  UNMAP_DS

  for (uint8_t i = 0; i < vm_state.inv_num_objects; ++i) {
    if (vm_state.inv_objects[i]->id == global_object_id) {
      return vm_state.inv_objects[i];
    }
  }

  return NULL;
}

uint8_t inv_object_available(uint16_t global_object_id)
{
  SAVE_DS_AUTO_RESTORE
  UNMAP_DS
  uint8_t result = 0;

  for (uint8_t i = 0; i < vm_state.inv_num_objects; ++i) {
    if (vm_state.inv_objects[i]->id == global_object_id) {
      result = 1;
      break;
    }
  }
  
  return result;
}

const char *inv_get_object_name(uint8_t position)
{
  UNMAP_DS
  const char *name_ptr = (const char *)vm_state.inv_objects[position] + vm_state.inv_objects[position]->name_offset;
  return name_ptr;
}

uint16_t inv_get_global_object_id(uint8_t position)
{
  UNMAP_DS
  return vm_state.inv_objects[position]->id;
}

uint8_t inv_get_position_by_id(uint16_t global_object_id)
{
  UNMAP_DS

  for (uint8_t i = 0; i < vm_state.inv_num_objects; ++i) {
    if (vm_state.inv_objects[i]->id == global_object_id) {
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
  
  inv_ui_entries.num_entries = 0;
  inv_ui_entries.prev_id     = 0xff;
  inv_ui_entries.next_id     = 0xff;

  do {
    for (uint8_t i = 0; i < vm_state.inv_num_objects; ++i) {
      __auto_type object_ptr   = vm_state.inv_objects[i];
      uint16_t    object_id    = object_ptr->id;
      uint8_t     object_owner = vm_state.global_game_objects[object_id] & 0x0f;
      
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
        debug_out("inv p %d n %d, num %d, entries %d %d %d %d", inv_ui_entries.prev_id, inv_ui_entries.next_id, inv_ui_entries.num_entries, inv_ui_entries.displayed_ids[0], inv_ui_entries.displayed_ids[1], inv_ui_entries.displayed_ids[2], inv_ui_entries.displayed_ids[3]);
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
