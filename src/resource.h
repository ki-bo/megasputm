#ifndef __RESOURCE_H
#define __RESOURCE_H

#include <stdint.h>

#define MAX_RESOURCE_SIZE        0x4000
#define DEFAULT_RESOURCE_ADDRESS 0x8000
#define RESOURCE_MEMORY          0x18000UL

enum res_type_t {
    RES_TYPE_NONE = 0,
    RES_TYPE_ROOM,
    RES_TYPE_COSTUME,
    RES_TYPE_SCRIPT,
    RES_TYPE_SOUND
};

enum res_type_flags_t {
  RES_TYPE_MASK   = 0x07,
  RES_LOCKED_MASK = 0x80,
};

void res_init(void);
uint8_t res_provide(uint8_t type_and_flags, uint8_t id, uint8_t hint);
void res_lock(uint8_t type, uint8_t id, uint8_t hint);
void res_unlock(uint8_t type, uint8_t id, uint8_t hint);

#endif // __RESOURCE_H
