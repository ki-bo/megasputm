#ifndef __VM_H
#define __VM_H

#include <stdint.h>

#define NUM_SCRIPT_SLOTS 32
#define NUM_ACTORS       25
#define MAX_OBJECTS      57
#define ACTOR_NAME_LEN   16

enum {
  PROC_STATE_FREE,
  PROC_STATE_RUNNING,
  PROC_STATE_WAITING,
  PROC_STATE_DEAD
};

enum {
  PROC_TYPE_GLOBAL = 1
};

enum {
  VAR_EGO = 0,
  VAR_DIALOG_ACTIVE = 3,
  VAR_ROOM_NO = 4,
  VAR_MACHINE_SPEED = 6,
  VAR_NUM_ACTORS = 11,
  VAR_CURSOR_STATE = 21,
  VAR_TIMER_NEXT = 25,
  VAR_BACKUP_VERB = 38,
  VAR_CUTSCENEEXIT_KEY = 40,
};

enum {
  OBJ_STATE_1 = 0x10,
  OBJ_STATE_2 = 0x20,
  OBJ_STATE_4 = 0x40,
  OBJ_STATE_ACTIVE = 0x80,
};


extern uint8_t global_game_objects[780];
extern uint8_t variables_lo[256];
extern uint8_t variables_hi[256];
extern char dialog_buffer[256];

extern uint8_t actor_sounds[NUM_ACTORS];
extern uint8_t actor_palette_idx[NUM_ACTORS];
extern uint8_t actor_palette_colors[NUM_ACTORS];
extern char    actor_names[NUM_ACTORS][ACTOR_NAME_LEN];
extern uint8_t actor_costumes[NUM_ACTORS];
extern uint8_t actor_talk_colors[NUM_ACTORS];
extern uint8_t actor_talking;

extern uint8_t state_iface;
extern uint16_t camera_x;

extern uint8_t jiffy_counter;
extern uint8_t proc_state[NUM_SCRIPT_SLOTS];
extern uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint16_t proc_pc[NUM_SCRIPT_SLOTS];

void vm_init(void);
__task void vm_mainloop(void);
uint8_t vm_get_active_proc_state(void);
void vm_switch_room(uint8_t room_no);
void vm_set_script_wait_timer(int32_t negative_ticks);
void vm_start_cutscene(void);
void vm_actor_start_talking(uint8_t actor_id);
void vm_start_script(uint8_t script_id);
void vm_stop_active_script(void);
void vm_update_screen(void);

inline uint16_t vm_read_var(uint8_t var)
{
  volatile uint16_t value;
  value = variables_lo[var] | (variables_hi[var] << 8);
  return value;
}

inline uint8_t vm_read_var8(uint8_t var)
{
  return variables_lo[var];
}


static inline void vm_write_var(uint8_t var, uint16_t value)
{
  // variables_lo[var] = LSB(value);
  // variables_hi[var] = MSB(value);
  __asm volatile(" lda %[val]\n"
                 " sta variables_lo, x\n"
                 " lda %[val]+1\n"
                 " sta variables_hi, x"
                 :
                 : "Kx" (var), [val]"Kzp16" (value)
                 : "a");
}

#endif // __VM_H
