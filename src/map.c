#include "map.h"
#include "diskio.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

// private code functions
static inline void apply_map(void);

#define MAP_OFFSET_HEAP 0x1000 + (RESOURCE_BASE / 256) - 0x80

/**
  * @defgroup map_public Map Public Functions
  *
  * The below functions are used to map and unmap Code Segments (CS) and Data Segments (DS)
  * Code pages are mapped to 0x2000-0x3fff.
  * Data pages are mapped starting at 0x8000.
  * CS and DS can be controlled individually. Mapping CS will not affect DS and 
  * vice versa.
  *
  * @{
  */
#pragma clang section text="code" rodata="cdata" data="data" bss="zzpage"

union map_t __attribute__((zpage)) map_regs;

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
  * @brief Initializes the memory mapping system
  * 
  * This function must be called before any other memory mapping functions are
  * used. It sets up the memory mapping register memory.
  *
  * Code section: code_init
  */
void map_init(void)
{
  map_regs.quad = 0;
}

#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

/**
  * @brief Maps a resource slot to DS (0x8000-0xbfff)
  * 
  * Resource slots are located at RESOURCE_MEMORY (0x18000-0x27fff). 
  * This function maps the memory starting at 0x18000 + (res_page * 256) to 0x8000.
  * We are always mapping 16KB of memory.
  *
  * @param res_page The resource slot to map. The slot is 0-indexed.
  *
  * Code section: code
  */
void map_ds_resource(uint8_t res_page)
{
  // map offset: RESOURCE_MEMORY + page*256 - 0x8000
  map_regs.ds = 0x3000 + (RESOURCE_BASE / 256) + res_page - 0x80;
  apply_map();
}

/**
  * @brief Maps the DS to the heap memory
  * 
  * Maps the DS to the default heap memory. The heap memory is located at the beginning of
  * the resource memory (0x18000-0x187ff). The heap is always 1 page (256 bytes) in size.
  *
  * Code section: code
  */
void map_ds_heap(void)
{
  // assuming heap will always be at the beginning of the resource memory
  // and the map window is a single 8kb block.
  map_regs.ds = 0x1000 + (RESOURCE_BASE / 256) - 0x80;
  apply_map();
}

/**
  * @brief Maps a room offset to DS
  *
  * Maps a room offset to DS. The room offset is a 16-bit value that points to a location
  * in the room data. The room data is stored in the resource memory.
  *
  * The room data is mapped so that the specified offset will be between 0x8000 and 0x80ff.
  * The mapped block is 16kb in size (mapped to 0x8000 - 0xbfff).
  * 
  * @param room_offset The room offset to map
  * @return* The pointer to the mapped room offset (is between 0x8000 and 0x80ff)
  *
  * Code section: code
  */
uint8_t *map_ds_room_offset(uint16_t room_offset)
{
  uint8_t res_slot = room_res_slot + MSB(room_offset);
  uint8_t new_offset = LSB(room_offset);
  map_ds_resource(res_slot);
  return NEAR_U8_PTR(RES_MAPPED + new_offset);
}

/** @} */ // map_public

//-----------------------------------------------------------------------------------------------

/**
  * @defgroup map_private Map Private Functions
  * @{
  */

/**
  * @brief Applies the current map register
  * 
  * Code section: code
  */
static inline void apply_map(void)
{
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

/** @} */ // map_private

//-----------------------------------------------------------------------------------------------
