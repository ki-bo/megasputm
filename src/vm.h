#ifndef __VM_H
#define __VM_H

#include <stdint.h>

#define NUM_SCRIPT_SLOTS  8
#define NUM_ACTORS       25
#define ACTOR_NAME_LEN   16

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

extern uint8_t actor_sounds[NUM_ACTORS];
extern uint8_t actor_palette_idx[NUM_ACTORS];
extern uint8_t actor_palette_colors[NUM_ACTORS];
extern char    actor_names[NUM_ACTORS][ACTOR_NAME_LEN];
extern uint8_t actor_costumes[NUM_ACTORS];
extern uint8_t actor_talk_colors[NUM_ACTORS];

extern uint8_t state_cursor;
extern uint8_t state_iface;

extern uint8_t jiffy_counter;
extern uint8_t proc_state[NUM_SCRIPT_SLOTS];
extern uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint16_t proc_pc[NUM_SCRIPT_SLOTS];

void vm_init(void);
__task void vm_mainloop(void);
void vm_switch_room(uint8_t res_slot);

#endif // __VM_H
