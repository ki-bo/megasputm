#ifndef __RESOURCE_H
#define __RESOURCE_H

#include <stdint.h>

#define MAX_RESOURCE_SIZE        0x4000

enum res_type_t {
    RES_TYPE_NONE = 0,
    RES_TYPE_ROOM,
    RES_TYPE_COSTUME,
    RES_TYPE_SCRIPT,
    RES_TYPE_SOUND,
    RES_TYPE_HEAP
};

enum res_type_flags_t {
  RES_TYPE_MASK   = 0x07,
  RES_LOCKED_MASK = 0x80,
  RES_ACTIVE_MASK = 0x40
};

extern uint8_t res_pages_invalidated;

// code_init functions
void res_init(void);

// code_main functions
uint8_t res_provide(uint8_t type_and_flags, uint8_t id, uint8_t hint);
uint8_t __huge *res_get_huge_ptr(uint8_t slot);
void res_lock(uint8_t type, uint8_t id, uint8_t hint);
void res_unlock(uint8_t type, uint8_t id, uint8_t hint);
void res_activate(uint8_t type, uint8_t id, uint8_t hint);
void res_deactivate(uint8_t type, uint8_t id, uint8_t hint);
void res_activate_slot(uint8_t slot);
void res_deactivate_slot(uint8_t slot);
void res_set_flags(uint8_t slot, uint8_t flags);
void res_clear_flags(uint8_t slot, uint8_t flags);
void res_reset_flags(uint8_t slot, uint8_t flags);
uint8_t res_reserve_heap(uint8_t size_blocks);
void res_free_heap(uint8_t slot);

#endif // __RESOURCE_H
