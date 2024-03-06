#ifndef __SCRIPT_H
#define __SCRIPT_H

#include <stdint.h>

enum {
    VAR_EGO = 0,
    VAR_MACHINE_SPEED = 6,
    VAR_NUM_ACTOR = 11,
    VAR_TIMER_NEXT = 25,
    VAR_BACKUP_VERB = 38,
    VAR_CUSCENEEXIT_KEY = 40,
};

extern uint8_t global_game_objects[780];

void script_run(uint8_t script_id);

#endif // __SCRIPT_H
