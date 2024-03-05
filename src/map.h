#ifndef __MAP_H
#define __MAP_H

#include <stdint.h>

void map_init(void);

void unmap_all(void);
void unmap_cs(void);
void unmap_ds(void);

void map_diskio(void);
void map_gfx(void);
void map_resource(uint8_t res_page);

#endif // __MAP_H
