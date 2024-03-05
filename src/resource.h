#ifndef __RESOURCE_H
#define __RESOURCE_H

#include <stdint.h>

enum ResourceType {
    RES_TYPE_NONE = 0,
    RES_TYPE_ROOM,
    RES_TYPE_COSTUME,
    RES_TYPE_SCRIPT,
    RES_TYPE_SOUND
};

void res_init(void);

uint8_t res_provide(uint8_t type, uint8_t id);

void res_map(uint8_t slot);

#endif // __RESOURCE_H
