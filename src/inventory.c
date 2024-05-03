#include "inventory.h"
#include "dma.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"

#define INV_MAX_OBJECTS 80

#pragma clang section rodata="cdata_main" data="data_main" bss="zdata"

uint8_t             inv_num_objects;
struct object_code *inv_objects[INV_MAX_OBJECTS];
uint8_t            *inv_next_free;

/**
 * @defgroup inv_init Inventory Init Functions
 * @{
 */
#pragma clang section text="code_init"  rodata="cdata_init" data="data_init" bss="bss_init"

void inv_init()
{
  inv_num_objects = 0;
  inv_objects[0]  = NULL;
  inv_next_free   = (void *)INVENTORY_BASE;
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

  uint16_t free_bytes = INVENTORY_BASE + INVENTORY_SIZE - (uint16_t)inv_next_free;
  if (free_bytes < size) {
    fatal_error(ERR_OUT_OF_INVENTORY_SPACE);
  }
  else if (inv_num_objects == INV_MAX_OBJECTS) {
    fatal_error(ERR_TOO_MANY_INVENTORY_OBJECTS);
  }

  static dmalist_t dmalist_inv_add_object = {
    .end_of_options = 0,
    .command        = 0,
    .count          = 0,
    .src_addr       = 0,
    .src_bank       = 0,
    .dst_addr       = 0,
    .dst_bank       = 0
  };

  global_dma_list.command = 0;
  global_dma_list.count = size;
  global_dma_list.src_addr = LSB16(obj_hdr);
  global_dma_list.src_bank = BANK(obj_hdr);
  global_dma_list.dst_addr = (uint16_t)inv_next_free;
  global_dma_list.dst_bank = 0;
  dma_trigger(&global_dma_list);

  inv_objects[inv_num_objects] = (struct object_code *)inv_next_free;
  ++inv_num_objects;
  inv_next_free += size;
}

struct object_code *inv_get_object_by_id(uint8_t global_object_id)
{
  unmap_ds();

  for (uint8_t i = 0; i < inv_num_objects; ++i) {
    debug_out("compare %d %d", inv_objects[i]->id, global_object_id);
    if (inv_objects[i]->id == global_object_id) {
      return inv_objects[i];
    }
  }

  return NULL;
}

uint8_t inv_object_available(uint16_t id)
{
  uint16_t save_ds = map_get_ds();
  uint8_t result = 0;

  for (uint8_t i = 0; i < inv_num_objects; ++i) {
    if (inv_objects[i]->id == id) {
      result = 1;
      break;
    }
  }
  
  map_set_ds(save_ds);
  return result;
}

const char *inv_get_object_name(uint8_t position)
{
  unmap_ds();
  debug_out("inv_objects %p name_offset %02x", inv_objects[position], inv_objects[position]->name_offset);
  const char *name_ptr = (const char *)inv_objects[position] + inv_objects[position]->name_offset;
  return name_ptr;
}

uint8_t inv_get_object_id(uint8_t position)
{
  unmap_ds();
  return inv_objects[position]->id;
}

///@} inv_public
