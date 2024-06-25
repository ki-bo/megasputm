#pragma once

#include <stdint.h>
#include "vm.h"

extern uint8_t __attribute__((zpage)) parallel_script_count;

// code_init functions
void script_init(void);

// code_main functions
void script_schedule_init_script(void);
uint8_t script_execute_slot(uint8_t slot);
uint16_t script_get_current_pc(void);
void script_break(void);
uint8_t script_start(uint8_t script_id);
void script_execute_room_script(uint16_t room_script_offset);
void script_execute_object_script(uint8_t verb, uint16_t object, uint8_t background);
void script_stop_slot(uint8_t slot);
void script_stop(uint8_t script_id);
void script_print_slot_table(void);
uint8_t script_is_room_object_script(uint8_t slot);
