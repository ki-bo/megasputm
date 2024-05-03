#ifndef __INVENTORY_H
#define __INVENTORY_H

#include <stdint.h>

// code_init functions
void inv_init();

//code_main functions
void inv_add_object(uint8_t local_object_id);
uint8_t inv_object_available(uint16_t id);

#endif // __INVENTORY_H
