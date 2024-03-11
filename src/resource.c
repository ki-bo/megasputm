#include "resource.h"
#include "diskio.h"
#include "map.h"
#include "util.h"


#pragma clang section bss="zdata"

uint8_t page_res_type[256];
uint8_t page_res_index[256];

//-----------------------------------------------------------------------------------------------

// Private resource functions
static uint8_t allocate(uint8_t type, uint8_t id, uint8_t num_pages);
static uint8_t defragment_memory(void);
static void update_flags(uint8_t type_and_flags, uint8_t id, uint8_t hint);

// Private wrapper functions
static uint16_t start_resource_loading(uint8_t type, uint8_t id);
static void continue_resource_loading(void);

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
 * If setting the RES_LOCKED_MASK flag, the resource will be locked in memory and
 * will not be overwritten by other resources. This is useful for resources that
 * are used frequently and should not be reloaded from disk.
 *
 * @param type_and_flags Resource type and flags to provide
 * @param id Resource ID to provide
 * @param hint The position in the page list to start searching for the resource
 * @return uint8_t Page of the resource in the resource memory
 *
 * Code section: code_main
 */
uint8_t res_provide(uint8_t type_and_flags, uint8_t id, uint8_t hint)
{
  uint8_t i = hint;
  do {
    if(page_res_index[i] == id && page_res_type[i] == (type_and_flags & RES_TYPE_MASK)) {
      if (type_and_flags != page_res_type[i]) {
        // resource is available, but need to update flags
        update_flags(type_and_flags, id, i);
      }
      debug_out("Found resource %d at page %d", id, i);
      return i;
    }
    i++;
  }
  while(i != hint);

  debug_out("Loading resource type %d id %d", type_and_flags & RES_TYPE_MASK, id);

  uint16_t chunk_size = start_resource_loading(type_and_flags & RES_TYPE_MASK, id);
  debug_out("  Size: %d", chunk_size);

  if (chunk_size > MAX_RESOURCE_SIZE) {
    fatal_error(ERR_RESOURCE_TOO_LARGE);
  }
  uint8_t page = allocate(type_and_flags, id, (chunk_size + 255) / 256);
  uint16_t ds_save = map_get_ds();
  map_ds_resource(page);
  continue_resource_loading();
  map_set_ds(ds_save);
  return page;
}

void res_lock(uint8_t type, uint8_t id, uint8_t hint)
{
  uint8_t new_type = type | RES_LOCKED_MASK;
  update_flags(new_type, id, hint);
}

void res_unlock(uint8_t type, uint8_t id, uint8_t hint)
{
  uint8_t new_type = type & ~RES_LOCKED_MASK;
  update_flags(new_type, id, hint);
}

static void update_flags(uint8_t type_and_flags, uint8_t id, uint8_t hint)
{
  uint8_t type = type_and_flags & RES_TYPE_MASK;
  uint8_t found = 0;
  uint8_t i = hint;
  do {
    if (page_res_index[i] == id && page_res_type[i] == type) {
      page_res_type[i] = type_and_flags;
      found = 1;
    }
    else if (found) {
      // current page does not match, but we already had matching pages before.
      // this means we reached the end of the resource's pages and can stop.
      break;
    }
  }
  while (++i != hint);
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
 * Private function
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
 * Private function
 */
static uint8_t defragment_memory(void)
{
  uint8_t read_idx = 0;
  uint8_t write_idx = 0;

  do {
    if (page_res_type[read_idx] == RES_TYPE_NONE) {
      ++read_idx;
    }
    else {
      if (read_idx != write_idx) {
        page_res_type[write_idx] = page_res_type[read_idx];
        page_res_index[write_idx] = page_res_index[read_idx];
      }
      ++write_idx;
      ++read_idx;
    }
  }
  while (read_idx != 0);

  for (uint8_t i = 255; i >= write_idx; i--) {
    page_res_type[i] = RES_TYPE_NONE;
  }

  return write_idx;
}

/** @} */ // res_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup res_wrapper Resource Private Wrapper Functions
 * Placed in code section to protect from being overlayed by mapped modules.
 * @{
 */
#pragma clang section text="code"

/**
 * @brief Wrapper function for diskio_start_resource_loading
 *
 * Parameters are forwarded to diskio_start_resource_loading().
 *
 * @see diskio_start_resource_loading
 *
 * Code section: code
 * Private function
 */
static uint16_t start_resource_loading(uint8_t type, uint8_t id)
{
  map_cs_diskio();
  uint16_t chunk_size = diskio_start_resource_loading(type, id);
  unmap_cs();
  return chunk_size;
}

/**
 * @brief Wrapper function for diskio_continue_resource_loading
 *
 * Placed in code section to avoid being overwritten by the diskio functions.
 * @see diskio_continue_resource_loading
 *
 * Code section: code
 * Private function
 */
static void continue_resource_loading(void)
{
  map_cs_diskio();
  diskio_continue_resource_loading();
  unmap_cs();
}

/** @} */ // res_wrapper
