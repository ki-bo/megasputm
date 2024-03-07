#ifndef __VM_H
#define __VM_H

#include <stdint.h>

#define NUM_SCRIPT_SLOTS 8

enum {
    VAR_EGO = 0,
    VAR_MACHINE_SPEED = 6,
    VAR_NUM_ACTOR = 11,
    VAR_TIMER_NEXT = 25,
    VAR_BACKUP_VERB = 38,
    VAR_CUTSCENEEXIT_KEY = 40,
};

extern uint8_t global_game_objects[780];
extern uint8_t variables_lo[256];
extern uint8_t variables_hi[256];
extern uint8_t jiffy_counter;
extern uint8_t proc_state[NUM_SCRIPT_SLOTS];
extern uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint16_t proc_pc[NUM_SCRIPT_SLOTS];

void vm_init(void);
__task void vm_mainloop(void);

#endif // __VM_H
