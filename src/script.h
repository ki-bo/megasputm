#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <stdint.h>

enum script_return_t {
    SCRIPT_RUNNING = 0,
    SCRIPT_FINISHED = 1,
    SCRIPT_BLOCKED = 2,
};

// code_init functions
void script_init(void);

// code_main functions
uint8_t script_run_active_slot(void);
void script_save_state(void);
void script_restore_state(void);

#endif // __SCRIPT_H
