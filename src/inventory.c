#include "inventory.h"
#include "dma.h"
#include "memory.h"
#include "util.h"

#define INV_MAX_OBJECTS 80

uint8_t  inv_num_objects;
uint8_t *inv_objects[INV_MAX_OBJECTS];
uint8_t *inv_next_free;

/**
 * @defgroup inv_init Inventory Init Functions
 * @{
 */

void inv_init()
{
  inv_num_objects = 0;
  inv_objects[0] = NULL;
  inv_next_free = (void *)INVENTORY_BASE;
}

 ///@} inv_init

/**
 * @defgroup inv_public Inventory Public Functions
 * @{
 */
#pragma clang section text="code_main"  rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Add an object to the inventory.
 * 
 * @param id The global object id.
 * @param object Pointer to the object chunk.
 * @param size Amount of bytes to copy.
 */
void inv_add_object(uint8_t id, uint8_t __far* object, uint8_t size)
{
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

  dmalist_inv_add_object.count_lsb = size;
  dmalist_inv_add_object.src_addr = LSB16(object);
  dmalist_inv_add_object.src_bank = BANK(object);
  dmalist_inv_add_object.dst_addr = (uint16_t)(inv_next_free);
  dma_trigger(&dmalist_inv_add_object);

  ++inv_num_objects;
  inv_next_free += size;
}

///@} inv_public
