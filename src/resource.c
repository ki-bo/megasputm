#include "resource.h"
#include "diskio.h"
#include "dma.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include <stdint.h>
#include <string.h>

//#define HEAP_DEBUG_OUT 1

//-----------------------------------------------------------------------------------------------

#pragma clang section bss="zdata"

//-----------------------------------------------------------------------------------------------

enum heap_strategy_t {
  HEAP_STRATEGY_FREE_ONLY,
  HEAP_STRATEGY_ALLOW_UNLOCKED,
  HEAP_STRATEGY_ALLOW_LOCKED
};

//-----------------------------------------------------------------------------------------------

uint8_t page_res_type[256];
uint8_t page_res_index[256];

//-----------------------------------------------------------------------------------------------

// Private resource functions
static void set_flags(uint8_t slot, uint8_t flags);
static void clear_flags(uint8_t slot, uint8_t flags);
static void reset_flags(uint8_t slot, uint8_t flags);
static uint16_t find_resource(uint8_t type, uint8_t id, uint8_t hint);
static uint16_t find_and_set_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags);
static uint16_t find_and_clear_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags);
static uint8_t allocate(uint8_t type, uint8_t id, uint8_t num_pages);
static uint16_t find_free_block_range(uint8_t num_pages, enum heap_strategy_t strategy);
static void free_resource(uint8_t slot);
static uint8_t defragment_memory(void);
static void clear_inactive_blocks(void);
static void print_heap(void);

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
  memset(page_res_index, 0, 256);

  // allocate pages for the compiler's heap and activate them
  uint8_t heap_pages = (HEAP_SIZE + 255) / 256;
  for (uint8_t i = 0; i < heap_pages; ++i) {
    page_res_type[i] = RES_TYPE_HEAP | RES_ACTIVE_MASK;
  }
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
 * the page of the resource in the resource memory. Use map_ds_resource() to
 * map the resource memory to the data segment.
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

  uint16_t save_cs = map_get_cs();

  map_cs_diskio();
  uint16_t chunk_size = diskio_start_resource_loading(type, id);
  //debug_out("Loading resource type %d id %d, size %d", type, id, chunk_size);
 
  map_cs_main_priv();
  uint8_t page = allocate(type, id, (chunk_size + 255) / 256);
  __auto_type dest = HUGE_U8_PTR(RESOURCE_BASE + (uint16_t)page * 256);
  
  map_cs_diskio();
  diskio_continue_resource_loading(dest);

#ifdef HEAP_DEBUG_OUT  
  map_cs_main_priv();
  print_heap();
#endif

  map_set_cs(save_cs);

  return page;
}

/**
 * @brief Deactivates all resources in memory
 *
 * This function deactivates all resources in memory, except for heap resources.
 *
 * Code section: code_main
 */
void res_deactivate_and_unlock_all(void)
{
  uint8_t i = 0;
  do {
    if ((page_res_type[i] & RES_TYPE_MASK) != RES_TYPE_HEAP) {
      page_res_type[i] &= ~(RES_ACTIVE_MASK | RES_LOCKED_MASK);
    }
  }
  while (++i != 0);
#ifdef HEAP_DEBUG_OUT
  uint16_t save_cs = map_cs_main_priv();
  print_heap();
  map_set_cs(save_cs);
#endif
}

/**
 * @brief Provides a 32 bit pointer to a resource in memory
 * 
 * @param slot The page of the resource in the resource memory
 * @return The 32 bit pointer to the resource
 */
uint8_t __huge *res_get_huge_ptr(uint8_t slot)
{
  return (uint8_t __huge *)(RESOURCE_BASE + (uint16_t)slot * 256);
}

/**
 * @brief Locks a resource in memory
 *
 * This function locks a resource in memory. The resource will not be overwritten
 * by other resources if possible. This is useful for resources that are used frequently and
 * should not be reloaded from disk. However, it is not guaranteed that the resource will
 * remain in memory. If memory allocations can't be satisfied by just freeing unlocked
 * resources, locked resources will be freed as well.
 *
 * @param type Resource type and flags
 * @param id Resource ID
 * @param hint The position in the page list to start searching for the resource
 *
 * Code section: code_main
 */
void res_lock(uint8_t type, uint8_t id, uint8_t hint)
{
  uint16_t save_cs = map_cs_main_priv();
  find_and_set_flags(type, id, hint, RES_LOCKED_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Locking resource type %d id %d", type, id);
  print_heap();
#endif
  map_set_cs(save_cs);
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
  uint16_t save_cs = map_cs_main_priv();
  uint16_t slot = find_and_clear_flags(type, id, hint, RES_LOCKED_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Unlocking resource type %d id %d slot %d", type, id, slot);
  print_heap();
#endif
  map_set_cs(save_cs);
}

/**
 * @brief Marks a resource as active, preventing it from being freed
 *
 * Activating a resource prevents it from being freed by the resource manager. This is useful
 * for resources that are currently in use and should not be freed. It is important to deactivate
 * the resource once it is no longer needed to allow the resource manager to free it.
 * 
 * @param type Type of the resource to activate
 * @param id ID of the resource to activate
 * @param hint The position in the page list to start searching for the resource
 *
 * Code section: code_main
 */
void res_activate(uint8_t type, uint8_t id, uint8_t hint)
{
  uint16_t save_cs = map_cs_main_priv();
  find_and_set_flags(type, id, hint, RES_ACTIVE_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Activating resource type %d id %d", type, id);
  print_heap();
#endif
  map_set_cs(save_cs);
}

/**
 * @brief Deactivates a resource, allowing it to be freed
 * 
 * Deactivating a resource allows the resource manager to free it if necessary. This is useful
 * for resources that are no longer in use and can be freed to make room for other resources.
 * 
 * @param type Type of the resource to deactivate
 * @param id ID of the resource to deactivate
 * @param hint The position in the page list to start searching for the resource
 *
 * Code section: code_main
 */
void res_deactivate(uint8_t type, uint8_t id, uint8_t hint)
{
  uint16_t save_cs = map_cs_main_priv();
  uint16_t slot = find_and_clear_flags(type, id, hint, RES_ACTIVE_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Deactivating resource type %d id %d slot %d", type, id, slot);
  print_heap();
#endif
  map_set_cs(save_cs);
}

/**
 * @brief Activates a resource slot, preventing it from being freed
 * 
 * Activating a resource slot prevents it from being freed by the resource manager. This is useful
 * for resources that are currently in use and should not be freed. It is important to deactivate
 * the resource once it is no longer needed to allow the resource manager to free it.
 * 
 * It is the whole resource starting at the provided slot that is activated. Thus, all pages
 * of the resource are getting marked as active.
 * 
 * @param slot Starting page of the resource to activate
 *
 * Code section: code_main
 */
void res_activate_slot(uint8_t slot)
{
  uint16_t save_cs = map_cs_main_priv();
  set_flags(slot, RES_ACTIVE_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Activating slot %d", slot);
  print_heap();
#endif
  map_set_cs(save_cs);
}

/**
 * @brief Deactivates a resource slot, allowing it to be freed
 * 
 * Deactivating a resource slot allows the resource manager to free it if necessary. This is useful
 * for resources that are no longer in use and can be freed to make room for other resources.
 * 
 * It is the whole resource starting at the provided slot that is deactivated. Thus, all pages
 * of the resource are getting marked as inactive.
 * 
 * @param slot Starting page of the resource to deactivate
 *
 * Code section: code_main
 */
void res_deactivate_slot(uint8_t slot)
{
  
  uint16_t save_cs = map_cs_main_priv();
  clear_flags(slot, RES_ACTIVE_MASK);
#ifdef HEAP_DEBUG_OUT
  //debug_out("Deactivating slot %d", slot);
  print_heap();
#endif
  map_set_cs(save_cs);
}

uint8_t res_reserve_heap(uint8_t size_blocks)
{
  uint16_t save_cs = map_cs_main_priv();
  uint8_t slot = allocate(RES_TYPE_HEAP, 0, size_blocks);
  set_flags(slot, RES_ACTIVE_MASK);
  map_set_cs(save_cs);
  return slot;
}

void res_free_heap(uint8_t slot)
{
  uint16_t save_cs = map_cs_main_priv();
  free_resource(slot);
  map_set_cs(save_cs);
}

/** @} */ // res_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup res_private Resource Private Functions
 * @{
 */

#pragma clang section text="code_main_private" rodata="cdata_main_private" data="data_main_private"

/**
 * @brief Sets flags for a resource
 *
 * This function sets the specified flags for a resource.
 *
 * @param slot Index of the first page of the resource
 * @param flags Flags to set
 *
 * Code section: code_main
 */
void set_flags(uint8_t slot, uint8_t flags)
{
  uint8_t current_type_and_flags = page_res_type[slot];
  reset_flags(slot, current_type_and_flags | flags);
}

/**
 * @brief Clears flags for a resource
 *
 * This function clears the specified flags for a resource.
 *
 * @param slot Index of the first page of the resource
 * @param flags Flags to clear
 *
 * Code section: code_main
 */
void clear_flags(uint8_t slot, uint8_t flags)
{
  uint8_t current_type_and_flags = page_res_type[slot];
  reset_flags(slot, current_type_and_flags & ~flags);
}

/**
 * @brief Resets flags for a resource
 *
 * This function resets all flags of a resource to the specified flags.
 *
 * @param slot Index of the first page of the resource
 * @param flags New flags to set
 *
 * Code section: code_main
 */
void reset_flags(uint8_t slot, uint8_t flags)
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

/**
 * @brief Finds a resource in memory
 *
 * This function finds a resource in memory. If hint is non-zero, the function
 * will start searching at the hint position. Be careful to make sure that the
 * hint is not pointing to the middle of a resource, as the function will not
 * check for that and might return a non-starting page of the resource.
 *
 * @param type Type of the resource
 * @param id ID of the resource
 * @param hint The position in the page list to start searching for the resource
 * @return uint16_t Index of the first page of the resource, or 0xffff if not found
 *
 * Code section: code_main
 */
uint16_t find_resource(uint8_t type, uint8_t id, uint8_t hint)
{
  uint8_t i = hint;
  type &= RES_TYPE_MASK;
  do {
    if(page_res_index[i] == id && (page_res_type[i] & RES_TYPE_MASK) == type) {
      return i;
    }
  }
  while(++i != hint);
  return 0xffff;
}

/**
 * @brief Finds a resource and sets flags
 *
 * This function finds a resource in memory and sets the specified flags. If hint is non-zero,
 * the function will start searching at the hint position. Be careful to make sure that the
 * hint is not pointing to the middle of a resource, as the function will not check for that
 * and might return a non-starting page of the resource.
 *
 * @param type Type of the resource
 * @param id ID of the resource
 * @param hint The position in the page list to start searching for the resource
 * @param flags Flags to set
 * @return uint16_t Index of the first page of the resource, or 0xffff if not found
 *
 * Code section: code_main
 */
uint16_t find_and_set_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags)
{
  uint16_t result = find_resource(type, id, hint);
  if (result == 0xffff) {
    return result;
  }
  uint8_t slot = (uint8_t) result;
  set_flags(slot, flags);
  return slot;
}

/**
 * @brief Finds a resource and clears flags
 *
 * This function finds a resource in memory and clears the specified flags. If hint is non-zero,
 * the function will start searching at the hint position. Be careful to make sure that the
 * hint is not pointing to the middle of a resource, as the function will not check for that
 * and might return a non-starting page of the resource.
 *
 * @param type Type of the resource
 * @param id ID of the resource
 * @param hint The position in the page list to start searching for the resource
 * @param flags Flags to clear
 * @return uint16_t Index of the first page of the resource, or 0xffff if not found
 *
 * Code section: code_main
 */
uint16_t find_and_clear_flags(uint8_t type, uint8_t id, uint8_t hint, uint8_t flags)
{
  uint16_t result = find_resource(type, id, hint);
  if (result == 0xffff) {
    return result;
  }
  uint8_t slot = (uint8_t) result;
  clear_flags(slot, flags);
  return slot;
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
/**
 * @brief Allocates memory for the specified resource
 * 
 * This function allocates memory for the specified resource.
 * If a free block of memory is available, the function will allocate
 * there. If not, it will try to free locked memory to make room.
 * If no unlocked memory can be freed, it will free locked memory.
 * Active resources will not be freed.
 *
 * If no memory can be freed, the function will call fatal_error().
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
  uint16_t result;

  result = find_free_block_range(num_pages, HEAP_STRATEGY_FREE_ONLY);
  if (result != 0xffff) {
    uint8_t slot = (uint8_t)result;
    for (uint8_t i = 0; i < num_pages; ++i) {
      page_res_type[slot + i] = type;
      page_res_index[slot + i] = id;
    }
    return slot;
  }

  // At this point, there is no contiguos free block available to allocate
  // Check whether there is enough free memory to if freeing locked memory 
  for (enum heap_strategy_t strategy = HEAP_STRATEGY_ALLOW_UNLOCKED; strategy <= HEAP_STRATEGY_ALLOW_LOCKED; ++strategy) {
    //debug_out("Trying heap allocation strategy %d", strategy);
    result = find_free_block_range(num_pages, strategy);
    if (result != 0xffff) {
      uint8_t slot = (uint8_t)result;
      for (uint8_t i = 0; i < num_pages; ++i) {
        if (page_res_type[slot + i] & RES_LOCKED_MASK) {
          debug_out("warning: freeing locked memory type %d id %d at slot %d", page_res_type[slot + i] & RES_TYPE_MASK, page_res_index[slot + i], slot + i);
        }
        // free the locked resource currently occupying the slot
        free_resource(slot + i);
        page_res_type[slot + i] = type;
        page_res_index[slot + i] = id;
      }
      return slot;
    }
  }

  fatal_error(ERR_OUT_OF_RESOURCE_MEMORY);
}
#pragma clang diagnostic pop

/**
 * @brief Finds a free block range in memory
 *
 * This function finds a free block range in memory with a best-fit strategy. The function
 * will search for a block of memory that is at least num_pages long. The strategy parameter
 * determines how the function will search for free memory. If the strategy is HEAP_STRATEGY_FREE_ONLY,
 * the function will only consider free memory pages. If the strategy is HEAP_STRATEGY_ALLOW_UNLOCKED,
 * the function will consider free and unlocked memory pages. If the strategy is HEAP_STRATEGY_ALLOW_LOCKED,
 * the function will consider free, unlocked and locked memory pages. Pages marked as active will not be
 * considered in any case.
 *
 * @param num_pages Number of pages to find
 * @param strategy Strategy to use when searching for free memory
 * @return uint16_t Index of the first page of the free block range, or 0xffff if not found
 *
 * Code section: code_main
 */
uint16_t find_free_block_range(uint8_t num_pages, enum heap_strategy_t strategy)
{
  uint8_t current_start = 0;
  uint8_t best_fit_start = 0;
  uint16_t best_fit_size = 0x7fff;
  uint8_t cur_size = 0;
  uint8_t block_idx = 0;

  //debug_out("Searching for %d pages of free memory", num_pages);
  
  do {
    uint8_t cur_type = page_res_type[block_idx];
    if(   (strategy == HEAP_STRATEGY_FREE_ONLY      && cur_type == RES_TYPE_NONE)
       || (strategy == HEAP_STRATEGY_ALLOW_UNLOCKED && (cur_type == RES_TYPE_NONE || (cur_type & (RES_LOCKED_MASK | RES_ACTIVE_MASK)) == 0))
       || (strategy == HEAP_STRATEGY_ALLOW_LOCKED   && (cur_type == RES_TYPE_NONE || (cur_type & RES_ACTIVE_MASK) == 0)))
    {
      if (cur_size == 0) {
        current_start = block_idx;
      }
      ++cur_size;
    }
    else {
      if (cur_size >= num_pages && cur_size < best_fit_size) {
        best_fit_start = current_start;
        best_fit_size = cur_size;
      }
      cur_size = 0;
    }

    if (cur_size == 255) {
      return current_start;
    }

    if (block_idx == 255 && cur_size >= num_pages && cur_size < best_fit_size) {
      best_fit_start = current_start;
      best_fit_size = cur_size;
    }

    //debug_out("  block %d, type %02x, index %d, free %d", block_idx, page_res_type[block_idx], page_res_index[block_idx], cur_size);
  }
  while (++block_idx != 0); // block_idx will wrap around to 0

  //debug_out("  result: %u, size %u", (uint16_t)best_fit_start, best_fit_size);

  if (best_fit_size == 0x7fff) {
    return 0xffff;
  }

  return best_fit_start;
}

/**
 * @brief Frees a resource in memory
 *
 * This function frees the resource in memory that covers the specified slot. The function
 * will free all pages of the resource, searching back for the start of the resource.
 *
 * If the page is marked as free already (RES_TYPE_NONE), the function will do nothing.
 *
 * @param slot Index of a page of the resource
 *
 * Code section: code_main
 */
static void free_resource(uint8_t slot)
{
  uint8_t type = page_res_type[slot] & RES_TYPE_MASK;
  if (type == RES_TYPE_NONE) {
    return;
  }

  uint8_t id = page_res_index[slot];

  // find first block of resource
  while (slot != 0) {
    --slot;
    if ((page_res_type[slot] & RES_TYPE_MASK) != type || page_res_index[slot] != id) {
      ++slot;
      break;
    }
  }

  //debug_out("Freeing resource type %d id %d at slot %d", type, id, slot);
  // free all blocks of resource
  while (1) {
    if ((page_res_type[slot] & RES_TYPE_MASK) == type && page_res_index[slot] == id) {
      page_res_type[slot] = RES_TYPE_NONE;
      page_res_index[slot] = 0;
      ++slot;
    }
    else {
      break;
    }
  }
#ifdef HEAP_DEBUG_OUT
  print_heap();
#endif
}

/**
 * @brief Prints out a summary of the current heap state
 *
 * This function prints out a summary of the current heap state. It will print out
 * the type, ID, number of pages, active flag and locked flag for each block range.
 *
 * This is a debug function and regular output of it can be activated by uncommenting
 * the HEAP_DEBUG_OUT define at the top of this file.
 *
 * Code section: code_main 
 */
static void print_heap(void)
{
  uint8_t num_pages = 0;
  uint8_t prev_type = RES_TYPE_NONE;
  uint8_t prev_id = 0xff;
  uint8_t idx = 0;
  debug_out("Heap:");
  do {
    if (page_res_type[idx] != prev_type || page_res_index[idx] != prev_id) {
      if (num_pages) {
        uint8_t type = prev_type & RES_TYPE_MASK;
        uint8_t active = prev_type & RES_ACTIVE_MASK;
        uint8_t locked = prev_type & RES_LOCKED_MASK;
        if (type == RES_TYPE_NONE) {
          debug_out(" [%03u] Free: %d", idx - num_pages, num_pages);
        }
        else {
          debug_out(" [%03u] Type: %d, ID: %03d, Pages: %02d, Active: %d, Locked: %d", idx - num_pages, type, prev_id, num_pages, active != 0, locked != 0);
        }
      }
      prev_type = page_res_type[idx];
      prev_id = page_res_index[idx];
      num_pages = 1;
    }
    else {
      ++num_pages;
    }
  }
  while (++idx != 0);
  debug_out(" [%03u] Free: %d", 256 - num_pages, num_pages);
  debug_out("-------");
}

/** @} */ // res_private

//-----------------------------------------------------------------------------------------------
