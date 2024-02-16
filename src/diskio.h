#ifndef __DISKIO_H
#define __DISKIO_H

#include <stdint.h>

/**
 * @brief Initialises the diskio module.
 *
 * This function must be called before any other diskio function.
 * It reads the start tracks and sectors of each room from the disk
 * directory and caches them in memory.
 */
void diskio_init_entry(void);

void diskio_map(void);

void diskio_unmap(void);

void diskio_load_file(const char *filename, uint8_t __far *address);

/**
 * @brief Loads a room from disk into memory.
 *
 * @param room The room number to load.
 * @param address The address in memory to load the room to.
 */
void diskio_load_room(uint8_t room, __far uint8_t *address);

void diskio_load();

#endif // __DISKIO_H
