#ifndef __MAP_H
#define __MAP_H

#include <stdint.h>

// code functions
void map_init(void);
uint32_t map_get(void);
uint16_t map_get_cs(void);
uint16_t map_get_ds(void);
void map_set(uint32_t map_reg);
void map_set_cs(uint16_t map_reg);
void map_set_ds(uint16_t map_reg);
void unmap_all(void);
void unmap_cs(void);
void unmap_ds(void);
void map_cs_diskio(void);
void map_cs_gfx(void);
uint8_t *map_ds_ptr(void __huge *ptr);
void map_ds_resource(uint8_t res_page);
void map_ds_heap(void);
uint8_t *map_ds_room_offset(uint16_t room_offset);

#endif // __MAP_H
