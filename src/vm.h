#ifndef __VM_H
#define __VM_H

#include <stdint.h>

#define NUM_SCRIPT_SLOTS   32
#define MAX_OBJECTS        57
#define CMD_STACK_SIZE      6

enum {
  PROC_STATE_FREE,
  PROC_STATE_RUNNING,
  PROC_STATE_WAITING_FOR_TIMER,
  PROC_STATE_WAITING_FOR_CHILD
};

enum {
  PROC_TYPE_GLOBAL = 1
};

enum {
  INPUT_EVENT_VERB_SELECT = 1,
  INPUT_EVENT_SCENE_CLICK = 2,
  INPUT_EVENT_KEYPRESS    = 4
};

enum {
  VAR_EGO = 0,
  VAR_CAMERA_CURRENT_X = 2,
  VAR_DIALOG_ACTIVE = 3,
  VAR_ROOM_NO = 4,
  VAR_MACHINE_SPEED = 6,
  VAR_COMMAND_VERB = 8,
  VAR_COMMAND_OBJECT_LEFT = 9,
  VAR_COMMAND_OBJECT_RIGHT = 10,
  VAR_NUM_ACTORS = 11,
  VAR_COMMAND_VERB_AVAILABLE = 18,
  VAR_CURSOR_STATE = 21,
  VAR_TIMER_NEXT = 25,
  VAR_NEXT_COMMAND_VERB = 26,
  VAR_NEXT_COMMAND_OBJECT_LEFT = 27,
  VAR_NEXT_COMMAND_OBJECT_RIGHT = 28,
  VAR_NEXT_COMMAND_PREPOSITION = 29,
  VAR_SCENE_CURSOR_X = 30,
  VAR_SCENE_CURSOR_Y = 31,
  VAR_INPUT_EVENT = 32,
  VAR_DEFAULT_VERB = 38,
  VAR_CUTSCENEEXIT_KEY = 40,
};

enum {
  OBJ_STATE_CAN_PICKUP  = 0x10,
  OBJ_STATE_DONT_SELECT = 0x20,
  OBJ_STATE_4           = 0x40,
  OBJ_STATE_ACTIVE      = 0x80
};

enum {
  SCRIPT_ID_COMMAND = 2,
  SCRIPT_ID_INPUT_EVENT = 4
};

enum {
  CAMERA_STATE_FOLLOW_ACTOR = 1,
  CAMERA_STATE_MOVE_TO_TARGET_POS = 2,
  CAMERA_STATE_MOVING = 4
};

extern uint8_t global_game_objects[780];
extern uint8_t variables_lo[256];
extern uint8_t variables_hi[256];
extern char dialog_buffer[256];

extern volatile uint8_t script_watchdog;
extern uint8_t state_iface;
extern uint16_t camera_x;

extern uint8_t active_script_slot;
extern uint8_t proc_state[NUM_SCRIPT_SLOTS];
extern uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint16_t proc_pc[NUM_SCRIPT_SLOTS];

extern uint8_t room_res_slot;
extern uint8_t current_room;

struct cmd_stack_t {
  uint8_t  num_entries;
  uint8_t  verb[CMD_STACK_SIZE];
  uint16_t object_left[CMD_STACK_SIZE];
  uint16_t object_right[CMD_STACK_SIZE];
};
extern struct cmd_stack_t cmd_stack;

void vm_init(void);
__task void vm_mainloop(void);
uint8_t vm_get_active_proc_state(void);
void vm_switch_room(uint8_t room_no);
void vm_set_script_wait_timer(int32_t negative_ticks);
void vm_start_cutscene(void);
void vm_costume_init();
void vm_actor_start_talking(uint8_t actor_id);
void vm_actor_place_in_room(uint8_t actor_id, uint8_t room_no);
uint8_t vm_start_script(uint8_t script_id);
uint8_t vm_start_room_script(uint16_t room_script_offset);
uint8_t vm_start_child_script(uint8_t script_id);
uint8_t vm_start_object_script(uint8_t verb, uint16_t object);
void vm_stop_active_script(void);
void vm_stop_script(uint8_t script_id);
uint8_t vm_is_script_running(uint8_t script_id);
void vm_update_screen(void);
uint16_t vm_get_object_at(uint8_t x, uint8_t y);
void vm_set_camera_follow_actor(uint8_t actor_id);
void vm_set_camera_target(uint8_t x);

static inline uint16_t vm_read_var(uint8_t var)
{
  volatile uint16_t value;
  value = variables_lo[var] | (variables_hi[var] << 8);
  return value;
}

static inline uint8_t vm_read_var8(uint8_t var)
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
