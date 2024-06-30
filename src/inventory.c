#include "inventory.h"
#include "dma.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"

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
  * @brief Get the displayed inventory.
  *
  * This function will return the inventory entries that should be displayed on the screen.
  * It filters the inventory starting at inventory item start_position for entries that match
  * the provided owner_id and returns the number of entries that should be displayed (0-4).
  *
  * It also fills the inventory display structure with the previous and next ids that match
  * the owner_id. If no previous or next entries are available, the corresponding id will be
  * set to 0xff. This information can be used to hide or display the inventory scroll buttons.
  * 
  * @param entries Pointer to the inventory display structure.
  * @param start_position Position to start displaying from.
  * @param owner_id Owner of the objects to display.
  * @return Number of entries available for displaying on scren.
  *
  * Code section: code_main
  */
uint8_t inv_get_displayed_inventory(struct inventory_display *entries, uint8_t start_position, uint8_t owner_id)
{
  UNMAP_DS

  entries->prev_id = 0xff;
  entries->next_id = 0xff;

  uint8_t num_entries = 0;
  for (uint8_t i = 0; i < vm_state.inv_num_objects; ++i) {
    __auto_type object_ptr   = vm_state.inv_objects[i];
    uint16_t    object_id    = object_ptr->id;
    uint8_t     object_owner = vm_state.global_game_objects[object_id] & 0x0f;
    if (object_owner == owner_id) {
      if (i < start_position) {
        entries->prev_id = i;
      }
      else if (num_entries < 4) {
        entries->displayed_ids[num_entries] = i;
        ++num_entries;
      }
      else {
        entries->next_id = i;
        break;
      }
    }
  }

  return num_entries;
}

///@} inv_public
