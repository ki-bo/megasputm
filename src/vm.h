#ifndef __VM_H
#define __VM_H

#include "util.h"
#include <stdint.h>

#define NUM_SCRIPT_SLOTS     32
#define MAX_OBJECTS          57
#define MAX_VERBS            22
#define CMD_STACK_SIZE        6

enum {
  // bits 0-2 reserved for process state
  PROC_STATE_FREE = 0,
  PROC_STATE_RUNNING = 1,
  PROC_STATE_WAITING_FOR_TIMER = 2,
  PROC_STATE_WAITING_FOR_CHILD = 3,
  // flags (bits 3-7)
  PROC_FLAGS_FROZEN = 0x80
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
  VAR_SELECTED_ACTOR = 0,
  VAR_CAMERA_X = 2,
  VAR_MESSAGE_GOING = 3,
  VAR_SELECTED_ROOM = 4,
  VAR_MACHINE_SPEED = 6,
  VAR_MSGLEN = 7,
  VAR_COMMAND_VERB = 8,
  VAR_COMMAND_NOUN1 = 9,
  VAR_COMMAND_NOUN2 = 10,
  VAR_NUMBER_OF_ACTORS = 11,
  VAR_VALID_VERB = 18,
  VAR_CURSOR_STATE = 21,
  VAR_TIMER_NEXT = 25,
  VAR_SENTENCE_VERB = 26,
  VAR_SENTENCE_NOUN1 = 27,
  VAR_SENTENCE_NOUN2 = 28,
  VAR_SENTENCE_PREPOSITION = 29,
  VAR_SCENE_CURSOR_X = 30,
  VAR_SCENE_CURSOR_Y = 31,
  VAR_INPUT_EVENT = 32,
  VAR_DEFAULT_VERB = 38,
  VAR_CUTSCENEEXIT_KEY = 40,
};

enum {
  OBJ_CLASS_PICKUPABLE  = 0x10,
  OBJ_CLASS_DONT_SELECT = 0x20,
  OBJ_CLASS_LOCKED      = 0x40,
  // SCUMM defines different meanings for the state bit
  // 0: HERE, CLOSED, R-OPEN,   OFF, R_GONE
  // 1: GONE, OPEN,   R-CLOSED, ON,  R_HERE
  OBJ_STATE             = 0x80
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

enum {
  SCREEN_UPDATE_BG     = 1,
  SCREEN_UPDATE_ACTORS = 2,
  SCREEN_UPDATE_DIALOG = 4,
  SCREEN_UPDATE_VERBS  = 8
};

enum {
  UI_FLAGS_APPLY_FREEZE     = 0x01,
	UI_FLAGS_APPLY_CURSOR     = 0x02,
	UI_FLAGS_APPLY_INTERFACE  = 0x04,
	UI_FLAGS_ENABLE_FREEZE    = 0x08,
	UI_FLAGS_ENABLE_CURSOR    = 0x10,
	UI_FLAGS_ENABLE_SENTENCE  = 0x20,
	UI_FLAGS_ENABLE_INVENTORY = 0x40,
	UI_FLAGS_ENABLE_VERBS     = 0x80
};

enum {
  VERB_STATE_DELETED = 0x80
};

struct walk_box {
  uint8_t top_y;
  uint8_t bottom_y;
  uint8_t topleft_x;
  uint8_t topright_x;
  uint8_t bottomleft_x;
  uint8_t bottomright_x;
  uint8_t mask;
  uint8_t flags;
};

extern uint8_t global_game_objects[780];
extern uint8_t variables_lo[256];
extern uint8_t variables_hi[256];
extern char message_buffer[256];

extern volatile uint8_t script_watchdog;
extern uint8_t ui_state;
extern uint16_t camera_x;

extern uint8_t active_script_slot;
extern uint8_t proc_script_id[NUM_SCRIPT_SLOTS];
extern uint8_t proc_state[NUM_SCRIPT_SLOTS];
extern uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint16_t proc_pc[NUM_SCRIPT_SLOTS];

extern uint8_t room_res_slot;

extern uint8_t          num_walk_boxes;
extern struct walk_box *walk_boxes;
extern uint8_t         *walk_box_matrix;


struct cmd_stack_t {
  uint8_t  num_entries;
  uint8_t  verb[CMD_STACK_SIZE];
  uint16_t noun1[CMD_STACK_SIZE];
  uint16_t noun2[CMD_STACK_SIZE];
};
extern struct cmd_stack_t cmd_stack;

void vm_init(void);
__task void vm_mainloop(void);
uint8_t vm_get_active_proc_state_and_flags(void);
void vm_change_ui_flags(uint8_t flags);
void vm_set_current_room(uint8_t room_no);
void vm_set_script_wait_timer(int32_t negative_ticks);
void vm_cut_scene_begin(void);
void vm_cut_scene_end(void);
void vm_begin_override(void);
void vm_costume_init();
void vm_say_line(uint8_t actor_id);
uint8_t vm_start_script(uint8_t script_id);
uint8_t vm_start_room_script(uint16_t room_script_offset);
uint8_t vm_start_child_script(uint8_t script_id);
uint8_t vm_start_object_script(uint8_t verb, uint16_t object);
void vm_stop_active_script(void);
void vm_stop_script(uint8_t script_id);
uint8_t vm_is_script_running(uint8_t script_id);
void vm_update_bg(void);
void vm_update_actors(void);
uint16_t vm_get_object_at(uint8_t x, uint8_t y);
void vm_clear_all_other_object_states(uint16_t global_object_id);
void vm_set_camera_follow_actor(uint8_t actor_id);
void vm_camera_pan_to(uint8_t x);
void vm_verb_new(uint8_t slot, uint8_t verb_id, uint8_t x, uint8_t y, const char* name);
void vm_verb_delete(uint8_t slot);
void vm_verb_set_state(uint8_t slot, uint8_t state);
char *vm_verb_get_name(uint8_t slot);

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
  variables_lo[var] = LSB(value);
  variables_hi[var] = MSB(value);
  /*__asm volatile(" lda %[val]\n"
                 " sta variables_lo, x\n"
                 " lda %[val]+1\n"
                 " sta variables_hi, x"
                 :
                 : "Kx" (var), [val]"Kzp16" (value)
                 : "a");*/
}

#endif // __VM_H
