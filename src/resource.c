#include "resource.h"
#include "diskio.h"
#include "dma.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include "util.h"

//-----------------------------------------------------------------------------------------------

#pragma clang section bss="zdata"

//-----------------------------------------------------------------------------------------------

uint8_t page_res_type[256];
uint8_t page_res_index[256];
uint8_t res_pages_invalidated;

//-----------------------------------------------------------------------------------------------

// Private resource functions
static uint16_t find_resource(uint8_t type, uint8_t id, uint8_t hint);
static void find_and_set_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags);
static void find_and_clear_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags);
static uint8_t allocate(uint8_t type, uint8_t id, uint8_t num_pages);
static uint8_t defragment_memory(void);

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup res_init Resource Init Functions
 * @{
 */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Initializes the resource memory
 *
 * This function initializes the resource memory by setting all pages to
 * RES_TYPE_NONE.
 *
 * Code section: code_init
 */
void res_init(void)
{
  memset(page_res_type, RES_TYPE_NONE, 256);
  res_pages_invalidated = 0;
}

/** @} */ // res_init

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup res_public Resource Public Functions
 * @{
 */
#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

/**
 * @brief Ensure a resource is available in memory
 *
 * This function ensures that a resource is available in memory. If the resource
 * is not already in memory, it will be loaded from disk. The function returns
 * the page of the resource in the resource memory, and maps the resource to the
 * data segment.
 * 
 * The resource is identified by a type and an id. The type is a combination of
 * the resource type and flags. The resource type is the lower 3 bits of the type
 * (see res_type_flags_t) and the flags are the upper 5 bits (see res_type_flags_t). 
 * The id is a number that identifies the resource within its type.
 *
 * @param type_and_flags Resource type to provide
 * @param id Resource ID to provide
 * @param hint The position in the page list to start searching for the resource
 * @return uint8_t Page of the resource in the resource memory
 *
 * Code section: code_main
 */
uint8_t res_provide(uint8_t type, uint8_t id, uint8_t hint)
{
  // will deal with sound later, as those resources are too big to fit into the chipram heap
  // (maybe will use attic ram for those)
  if (type == RES_TYPE_SOUND) {
    return 0;
  }

  uint8_t i = hint;
  do {
    if(page_res_index[i] == id && (page_res_type[i] & RES_TYPE_MASK) == type) {
      return i;
    }
    i++;
  }
  while(i != hint);

  map_cs_diskio();
  uint16_t chunk_size = diskio_start_resource_loading(type, id);
  
  if (chunk_size > MAX_RESOURCE_SIZE) {
    fatal_error(ERR_RESOURCE_TOO_LARGE);
  }
  uint8_t page = allocate(type, id, (chunk_size + 255) / 256);
  uint16_t ds_save = map_get_ds();
  map_ds_resource(page);
  diskio_continue_resource_loading();
  unmap_cs();

  map_set_ds(ds_save);
  
  return page;
}

void create_object_resource(uint8_t* data, uint16_t size, uint8_t id)
{
  uint8_t page = allocate(RES_TYPE_OBJECT, id, (size + 255) / 256);
  void __far* dest = (void __far*) (RESOURCE_BASE + page * 256);
  memcpy_to_bank(dest, data, size);
}

/**
 * @brief Locks a resource in memory
 *
 * This function locks a resource in memory. The resource will not be overwritten
 * by other resources. This is useful for resources that are used frequently and
 * should not be reloaded from disk.
 *
 * @param type Resource type and flags
 * @param id Resource ID
 * @param hint The position in the page list to start searching for the resource
 *
 * Code section: code_main
 */
void res_lock(uint8_t type, uint8_t id, uint8_t hint)
{
  find_and_set_flags(type, id, hint, RES_LOCKED_MASK);
}

/**
 * @brief Unlocks a resource in memory
 *
 * This function unlocks a resource in memory. The resource will be overwritten
 * by other resources if necessary.
 *
 * @param type Resource type and flags
 * @param id Resource ID
 * @param hint The position in the page list to start searching for the resource
 *
 * Code section: code_main
 */
void res_unlock(uint8_t type, uint8_t id, uint8_t hint)
{
  find_and_clear_flags(type, id, hint, RES_LOCKED_MASK);
}

void res_activate(uint8_t type, uint8_t id, uint8_t hint)
{
  find_and_set_flags(type, id, hint, RES_ACTIVE_MASK);
}

void res_deactivate(uint8_t type, uint8_t id, uint8_t hint)
{
  find_and_clear_flags(type, id, hint, RES_ACTIVE_MASK);
}

void res_set_flags(uint8_t slot, uint8_t flags)
{
  uint8_t current_type_and_flags = page_res_type[slot];
  res_reset_flags(slot, current_type_and_flags | flags);
}

void res_clear_flags(uint8_t slot, uint8_t flags)
{
  uint8_t current_type_and_flags = page_res_type[slot];
  res_reset_flags(slot, current_type_and_flags & ~flags);
}

void res_reset_flags(uint8_t slot, uint8_t flags)
{
  uint8_t current_type_and_flags = page_res_type[slot];
  uint8_t current_id = page_res_index[slot];
  uint8_t new_type_and_flags = (current_type_and_flags & RES_TYPE_MASK) | flags;

  while (page_res_type[slot] == current_type_and_flags &&
         page_res_index[slot] == current_id) {
    page_res_type[slot] = new_type_and_flags;
    ++slot;
  }
}

/** @} */ // res_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup res_private Resource Private Functions
 * @{
 */

uint16_t find_resource(uint8_t type, uint8_t id, uint8_t hint)
{
  uint8_t i = hint;
  do {
    if(page_res_index[i] == id && (page_res_type[i] & RES_TYPE_MASK) == type) {
      return i;
    }
  }
  while(++i != hint);
  return 0xffff;
}

void find_and_set_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags)
{
  uint16_t result = find_resource(type & RES_TYPE_MASK, id, hint);
  if (result == 0xffff) {
    return;
  }
  uint8_t slot = (uint8_t) result;
  res_set_flags(slot, flags);
}

void find_and_clear_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags)
{
  uint16_t result = find_resource(type & RES_TYPE_MASK, id, hint);
  if (result == 0xffff) {
    return;
  }
  uint8_t slot = (uint8_t) result;
  res_clear_flags(slot, flags);
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
/**
 * @brief Allocates memory for the specified resource
 * 
 * This function allocates memory for the specified resource.
 * If a free block of memory is available, the function will allocate
 * there. If not, it will defragment the memory and allocate directly
 * after the last allocated block.
 *
 * @note Pages of resources already allocated before
 *       might have moved after calling this function.
 *
 * @param type Type and flags of the resource
 * @param id ID of the resource
 * @param num_pages Contiguous pages to allocate
 * @return uint8_t Index of the first page of the allocated memory
 *
 * Code section: code_main
 */
static uint8_t allocate(uint8_t type, uint8_t id, uint8_t num_pages)
{
  uint8_t cur_size = 0;
  uint8_t block_idx = 0;
  uint8_t num_free_blocks = 0;
  
  do {
    if(page_res_type[block_idx] == RES_TYPE_NONE) {
      ++num_free_blocks;
      ++cur_size;
      if(cur_size == num_pages) {
        for (uint8_t i = 0; i < num_pages; i++) {
          page_res_type[block_idx] = type;
          page_res_index[block_idx] = id;
          --block_idx;
        }
        return block_idx + 1;
      }
    }
    else {
      cur_size = 0;
    }
  }
  while (++block_idx != 0);

  // At this point, there is no contiguos free block available to allocate
  // Check whether there is enough free memory to defragment and allocate 
  if (num_free_blocks >= num_pages) {
    uint8_t first_free_block = defragment_memory();
    for (uint8_t i = 0; i < num_pages; i++) {
      page_res_type[first_free_block + i] = type;
      page_res_index[first_free_block + i] = id;
    }
    return first_free_block;
  }

  // todo: free unlocked memory at this point and defragment memory
  fatal_error(ERR_OUT_OF_RESOURCE_MEMORY);
}
#pragma clang diagnostic pop

/**
 * @brief Defragments resource memory
 *
 * This function defragments the resource memory by moving all allocated
 * blocks to the beginning of the memory and freeing the rest of the memory.
 * 
 * @return uint8_t The number of pages that are now allocated (= the index of the first free page)
 *
 * Code section: code_main
 */
static uint8_t defragment_memory(void)
{
  debug_msg("Warning: Defragmenting memory");
  fatal_error(ERR_OUT_OF_RESOURCE_MEMORY);

  static dmalist_t dmalist_res_defrag = {
    .end_of_options = 0,
    .command        = 0,
    .count          = 256,
    .src_addr       = 0,
    .src_bank       = 0,
    .dst_addr       = 0,
    .dst_bank       = 0
  };

  uint8_t read_idx = 0;
  uint8_t write_idx = 0;

  dmalist_res_defrag.src_addr_page = RESOURCE_BASE >> 8;
  dmalist_res_defrag.dst_addr_page = RESOURCE_BASE >> 8;

  do {
    if (page_res_type[read_idx] == RES_TYPE_NONE) {
      ++read_idx;
      ++dmalist_res_defrag.src_addr_page;
    }
    else {
      if (read_idx != write_idx) {
        page_res_type[write_idx] = page_res_type[read_idx];
        page_res_index[write_idx] = page_res_index[read_idx];
        dma_trigger(&dmalist_res_defrag);
      }
      ++write_idx;
      ++dmalist_res_defrag.dst_addr_page;
      ++read_idx;
      ++dmalist_res_defrag.src_addr_page;
    }
  }
  while (read_idx != 0);

  for (uint8_t i = 255; i >= write_idx; i--) {
    page_res_type[i] = RES_TYPE_NONE;
  }

  res_pages_invalidated = 1;

  return write_idx;
}

/** @} */ // res_private

//-----------------------------------------------------------------------------------------------
