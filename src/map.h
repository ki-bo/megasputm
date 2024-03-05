#ifndef __MAP_H
#define __MAP_H

#include <stdint.h>

/**
 * @brief Initializes the memory mapping system
 * 
 * This function must be called before any other memory mapping functions are
 * used. It sets up the memory mapping register memory.
 *
 * @note Code section code_init
 */
void map_init(void);

/**
 * @defgroup map Memory mapping functions
 * The below functions are used to map and unmap code segments and data segments
 * Code pages are mapped to 0x2000-0x3fff.
 * Data pages are mapped starting at 0x8000.
 * CS and DS can be controlled individually. Mapping CS will not affect DS and 
 * vice versa.
 * @{
 */

/**
 * @brief Unmaps both CS and DS
 * @note Code section code
 */
void unmap_all(void);

/**
 * @brief Unmaps CS only, keeping DS mapped
 * @note Code section code
 */
void unmap_cs(void);

/**
 * @brief Unmaps DS only, keeping CS mapped
 * @note Code section code
 */
void unmap_ds(void);

/**
 * @brief Maps the disk I/O module to CS (0x2000-0x3fff)
 * @note Code section code
 * 
 * Disk I/O module originally is stored at 0x12000-0x13fff. This function maps
 * it to 0x2000-0x3fff.
 */
void map_cs_diskio(void);

/**
 * @brief Maps the graphics module to CS (0x2000-0x3fff)
 * 
 * Graphics module originally is stored at 0x14000-0x15fff. This function maps
 * it to 0x2000-0x3fff.
 *
 * @note Code section code
 */
void map_cs_gfx(void);

/**
 * @brief Maps a resource slot to DS (0x8000-0xbfff)
 * 
 * Resource slots are located at 0x50000-0x5ffff. This function maps the memory
 * starting at 0x50000 + (res_page * 256) to 0x8000. We are always mapping
 * 16KB of memory.
 *
 * @param res_page The resource slot to map. The slot is 0-indexed.
 *
 * @note Code section code
 */
void map_ds_resource(uint8_t res_page);

/** @} */ // end of map

#endif // __MAP_H
