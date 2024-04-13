#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <stdint.h>

extern uint8_t __attribute__((zpage)) parallel_script_count;

// code_init functions
void script_init(void);

// code_main functions
void script_run_active_slot(void);
void script_run_slot_stacked(uint8_t slot);

#endif // __SCRIPT_H
