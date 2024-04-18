#include "map.h"
#include "diskio.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

// private code functions
static inline void apply_map(void);

//-----------------------------------------------------------------------------------------------

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

union map_t {
  struct {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t z;
  };
  uint32_t quad;
};


union map_t __attribute__((zpage)) map_regs; // need to init with non-zero to force it into data section

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

/**
 * @brief Get the current map register (32 bit)
 * 
 * @return uint32_t The current map register
 *
 * Code section: code
 */
uint32_t map_get(void)
{
  return map_regs.quad;    
}

/**
 * @brief Get the current Code Segment (CS) map register
 * 
 * @return uint16_t The current CS map register
 *
 * Code section: code
 */
uint16_t map_get_cs(void)
{
  return (map_regs.x << 8) | map_regs.a;
}

/**
 * @brief Get the current Data Segment (DS) map register
 * 
 * @return uint16_t The current DS map register
 *
 * Code section: code
 */
uint16_t map_get_ds(void)
{
  return (map_regs.z << 8) | map_regs.y;
}

/**
 * @brief Set the map register (32 bit)
 * 
 * Sets the complete mapping register to the given value. It is usually done to restore a 
 * previously saved mapping (via map_get()).
 * 
 * @param map_reg The new map register
 *
 * Code section: code
 */
void map_set(uint32_t map_reg)
{
  map_regs.quad = map_reg;
  apply_map();
}

/**
 * @brief Set the Code Segment (CS) map register
 * 
 * Sets the Code Segment (CS) part of the mapping register to the given value. It is usually 
 * done to restore a previously saved CS mapping (via map_get_cs()).
 * 
 * @param map_reg The new CS map register
 *
 * Code section: code
 */
void map_set_cs(uint16_t map_reg)
{
  map_regs.a = LSB(map_reg);
  map_regs.x = MSB(map_reg);
  apply_map();
}

/**
 * @brief Set the Data Segment (DS) map register
 *
 * Sets the Data Segment (DS) part of the mapping register to the given value. It is usually
 * done to restore a previously saved DS mapping (via map_get_ds()).
 * 
 * @param map_reg The new DS map register
 *
 * Code section: code
 */
void map_set_ds(uint16_t map_reg)
{
  map_regs.y = LSB(map_reg);
  map_regs.z = MSB(map_reg);
  apply_map();
}

/**
 * @brief Unmap all segments
 * 
 * Disables all memory mappings (CS and DS).
 *
 * Code section: code
 */
void unmap_all(void)
{
  map_regs.quad = 0;
  apply_map();
}

/**
 * @brief Unmap the Code Segment (CS)
 * 
 * Disables the Code Segment (CS) memory mapping.
 *
 * Code section: code
 */
void unmap_cs(void)
{
  map_regs.a = 0;
  map_regs.x = 0;
  apply_map();
}

/**
 * @brief Unmap the Data Segment (DS)
 * 
 * Disables the Data Segment (DS) memory mapping.
 *
 * Code section: code
 */
void unmap_ds(void)
{
  map_regs.y = 0;
  map_regs.z = 0;
  apply_map();
}

/**
 * @brief Maps the disk I/O module to CS (0x2000-0x3fff)
 * 
 * Disk I/O module originally is stored at 0x12000-0x13fff. This function maps
 * it to 0x2000-0x3fff.
 *
 * Code section: code
 */
void map_cs_diskio(void)
{
  map_regs.a = 0x00;
  map_regs.x = 0x21;
  apply_map();
}

/**
 * @brief Maps the graphics module to CS (0x2000-0x3fff)
 * 
 * Graphics module originally is stored at 0x14000-0x15fff. This function maps
 * it to 0x2000-0x3fff.
 *
 * Code section: code
 */
void map_cs_gfx(void)
{
  map_regs.a = 0x20;
  map_regs.x = 0x21;
  apply_map();}

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
  uint16_t offset = 0x3000 + (HEAP_BASE / 256) + res_page - 0x80;
  map_regs.y = LSB(offset);
  map_regs.z = MSB(offset);
  apply_map();
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
 * @return uint8_t* The pointer to the mapped room offset (is between 0x8000 and 0x80ff)
 */
uint8_t *map_ds_room_offset(uint16_t room_offset)
{
  uint8_t res_slot = room_res_slot + MSB(room_offset);
  uint8_t new_offset = LSB(room_offset);
  map_ds_resource(res_slot);
  return NEAR_U8_PTR(RES_MAPPED + new_offset);
}

/** @} */ // map_private

//-----------------------------------------------------------------------------------------------
