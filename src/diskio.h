#ifndef __DISKIO_H
#define __DISKIO_H

#include <stdint.h>

/**
 * @brief Initialises the diskio module.
 *
 * This function must be called before any other diskio function.
 * It reads the start tracks and sectors of each room from the disk
 * directory and caches them in memory.
 * 
 * @note Code section code_init
 */
void diskio_init_entry(void);

/**
 * @brief Loads a file from disk into memory.
 * 
 * @param filename Null-terminated string containing the filename to load.
 * @param address Far pointer address in memory to load the file to.
 *
 * @note Code section code_diskio
 */
void diskio_load_file(const char *filename, uint8_t __far *address);

uint16_t diskio_start_resource_loading(uint8_t type, uint8_t id);

void diskio_continue_resource_loading(void);

/**
 * @brief Loads a room from disk into memory.
 *
 * @param room The room number to load.
 * @param address The address in memory to load the room to.
 */
void diskio_load_room(uint8_t room, __far uint8_t *address);

#endif // __DISKIO_H
