#pragma once

#include <stdint.h>

struct inventory_display {
    uint8_t prev_id;
    uint8_t displayed_ids[4];
    uint8_t next_id;
};

// code_init functions
void inv_init();

//code_main functions
void inv_add_object(uint8_t local_object_id);
struct object_code *inv_get_object_by_id(uint8_t global_object_id);
uint8_t inv_object_available(uint16_t id);
const char *inv_get_object_name(uint8_t position);
uint8_t inv_get_object_id(uint8_t position);
uint8_t inv_get_position_by_id(uint8_t global_object_id);
uint8_t inv_get_displayed_inventory(struct inventory_display *entries, uint8_t start_position, uint8_t owner_id);
