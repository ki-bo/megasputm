#pragma once

#include <stdint.h>

extern uint8_t __attribute__((zpage)) parallel_script_count;

// code_init functions
void script_init(void);

// code_main functions
uint8_t script_run_active_slot(void);
uint8_t script_run_slot_stacked(uint8_t slot);
uint16_t script_get_current_pc(void);
void script_break(void);
