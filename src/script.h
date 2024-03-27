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
uint8_t script_run(uint8_t script_id);
void script_run_as_function(uint8_t offset);

#endif // __SCRIPT_H
