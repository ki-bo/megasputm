/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "util.h"
#include <stdint.h>

#define NUM_SCRIPT_SLOTS     20
#define MAX_OBJECTS          55
#define MAX_VERBS            22
#define MAX_INVENTORY        80
#define CMD_STACK_SIZE        6
#define WATCHDOG_TIMEOUT     30

enum {
  // bits 0-2 reserved for process state
  PROC_STATE_FREE              = 0,
  PROC_STATE_RUNNING           = 1,
  PROC_STATE_WAITING_FOR_TIMER = 2,
  PROC_STATE_WAITING_FOR_CHILD = 3,
  // flags (bits 3-7)
  PROC_FLAGS_FROZEN            = 0x80
};

enum {
  PROC_TYPE_GLOBAL       = 0x01,
  PROC_TYPE_BACKGROUND   = 0x02,
  PROC_TYPE_REGULAR_VERB = 0x04,
  PROC_TYPE_INVENTORY    = 0x08
};

enum {
  INPUT_EVENT_VERB_SELECT     = 1,
  INPUT_EVENT_SCENE_CLICK     = 2,
  INPUT_EVENT_INVENTORY_CLICK = 3,
  INPUT_EVENT_KEYPRESS        = 4,
  INPUT_EVENT_SENTENCE_CLICK  = 5   // seems to be only relevant for Zak McKracken
};

enum {
  VAR_SELECTED_ACTOR = 0,
  VAR_OVERRIDE_HIT = 1,
  VAR_CAMERA_X = 2,
  VAR_MESSAGE_GOING = 3,
  VAR_SELECTED_ROOM = 4,
  VAR_MACHINE_SPEED = 6,
  VAR_MSGLEN = 7,
  VAR_CURRENT_VERB = 8,
  VAR_CURRENT_NOUN1 = 9,
  VAR_CURRENT_NOUN2 = 10,
  VAR_NUMBER_OF_ACTORS = 11,
  VAR_CURRENT_LIGHTS = 12,
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
  VAR_SELECTED_VERB = 33,
  VAR_CLICKED_NOUN = 35,
  VAR_DEFAULT_VERB = 38,
  VAR_CURRENT_KEY = 39,
  VAR_OVERRIDE_KEY = 40,
};

enum {
  OBJ_CLASS_PICKUPABLE  = 0x10,
  OBJ_CLASS_UNTOUCHABLE = 0x20,
  OBJ_CLASS_LOCKED      = 0x40,
  // SCUMM defines different meanings for the state bit
  // 0: HERE, CLOSED, R-OPEN,   OFF, R-GONE
  // 1: GONE, OPEN,   R-CLOSED, ON,  R-HERE
  OBJ_STATE             = 0x80
};

enum {
  SCRIPT_ID_SENTENCE = 2,
  SCRIPT_ID_INPUT_EVENT = 4
};

enum {
  CAMERA_STATE_FOLLOW_ACTOR = 1,
  CAMERA_STATE_MOVE_TO_TARGET_POS = 2,
  CAMERA_STATE_MOVING = 4
};

enum {
  SCREEN_UPDATE_BG         = 0x01,
  SCREEN_UPDATE_FLASHLIGHT = 0x02,
  SCREEN_UPDATE_ACTORS     = 0x04,
  SCREEN_UPDATE_DIALOG     = 0x08,
  SCREEN_UPDATE_VERBS      = 0x10,
  SCREEN_UPDATE_SENTENCE   = 0x20,
  SCREEN_UPDATE_INVENTORY  = 0x40
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

enum {
  RESET_RESTART     = 0x01,
  RESET_LOADED_GAME = 0x02
};

struct verb {
  uint8_t  id[MAX_VERBS];
  uint8_t  state[MAX_VERBS];
  uint8_t  x[MAX_VERBS];
  uint8_t  y[MAX_VERBS];
  uint8_t  len[MAX_VERBS];
  char    *name[MAX_VERBS];
};

struct object_code {
  uint16_t chunk_size;
  uint8_t  unused1;
  uint8_t  unused2;
  uint16_t id;
  uint8_t  unused3;
  uint8_t  pos_x;
  uint8_t  pos_y_and_parent_state;
  uint8_t  width;
  uint8_t  parent;
  uint8_t  walk_to_x;
  uint8_t  walk_to_y_and_preposition;
  uint8_t  height_and_actor_dir;
  uint8_t  name_offset;
};

struct vm
{
  uint8_t global_game_objects[780];
  uint8_t variables_lo[256];
  uint8_t variables_hi[256];

  uint8_t message_speed;

  uint8_t num_actor_palettes;

  uint8_t  num_active_proc_slots;
  uint8_t  proc_slot_table[NUM_SCRIPT_SLOTS];
  uint8_t  proc_script_or_object_id[NUM_SCRIPT_SLOTS];
  uint8_t  proc_object_id_msb[NUM_SCRIPT_SLOTS];
  uint8_t  proc_state[NUM_SCRIPT_SLOTS];
  uint8_t  proc_parent[NUM_SCRIPT_SLOTS];
  uint8_t  proc_type[NUM_SCRIPT_SLOTS];
  uint16_t proc_pc[NUM_SCRIPT_SLOTS];
  int32_t  proc_wait_timer[NUM_SCRIPT_SLOTS];

  // cutscene backup data
  uint8_t cs_room;
  int8_t cs_cursor_state;
  uint8_t cs_ui_state;
  uint8_t cs_camera_state;
  uint8_t cs_proc_slot;
  uint16_t cs_override_pc;

  // verb data
  struct verb verbs;
  
  uint8_t             inv_num_objects;
  struct object_code *inv_objects[MAX_INVENTORY];
  uint8_t             reserved1;
  uint8_t             reserved2;

  uint8_t             flashlight_width;
  uint8_t             flashlight_height;
};

extern struct vm        vm_state;
extern uint8_t          reset_game;
extern uint8_t          proc_slot_table_exec;
extern char             message_buffer[256];
extern volatile uint8_t script_watchdog;
extern uint8_t          ui_state;
extern uint16_t         camera_x;
extern int8_t           proc_slot_table_idx;
extern uint8_t          proc_table_cleanup_needed;
extern uint8_t          active_script_slot;
extern uint8_t          proc_res_slot[NUM_SCRIPT_SLOTS];
extern uint8_t          proc_exec_count[NUM_SCRIPT_SLOTS];
extern uint8_t          room_res_slot;
extern uint8_t          obj_page[MAX_OBJECTS];
extern uint8_t          obj_offset[MAX_OBJECTS];
extern uint16_t         obj_id[MAX_OBJECTS];
extern uint8_t          inventory_pos;
extern uint8_t          last_selected_actor;

struct sentence_stack_t {
  uint8_t  num_entries;
  uint8_t  verb[CMD_STACK_SIZE];
  uint16_t noun1[CMD_STACK_SIZE];
  uint16_t noun2[CMD_STACK_SIZE];
};
extern struct sentence_stack_t sentence_stack;

void vm_init(void);
__task void vm_mainloop(void);
uint8_t vm_get_proc_state(uint8_t slot);
uint8_t vm_get_active_proc_state_and_flags(void);
void vm_change_ui_flags(uint8_t flags);
void vm_set_current_room(uint8_t room_no);
uint8_t vm_get_room_object_script_offset(uint8_t verb, uint8_t local_object_id, uint8_t is_inventory);
void vm_set_script_wait_timer(int32_t negative_ticks);
void vm_cut_scene_begin(void);
void vm_cut_scene_end(void);
void vm_begin_override(void);
void vm_costume_init();
void vm_say_line(uint8_t actor_id);
uint8_t vm_get_first_script_slot_by_script_id(uint8_t script_id);
uint8_t vm_is_script_running(uint8_t script_id);
void vm_update_bg(void);
void vm_update_flashlight(void);
void vm_update_actors(void);
void vm_update_sentence(void);
void vm_update_inventory(void);
struct object_code *vm_get_room_object_hdr(uint16_t global_object_id);
uint16_t vm_get_object_at(uint8_t x, uint8_t y);
uint8_t vm_get_object_position(uint16_t global_object_id, uint8_t *x, uint8_t *y);
void vm_set_object_name(uint16_t global_object_id, const char *name);
uint8_t vm_get_local_object_id(uint16_t global_object_id);
uint8_t vm_calc_proximity(uint16_t actor_or_object_id1, uint16_t actor_or_object_id2);
void vm_draw_object(uint8_t local_object_id, uint8_t x, uint8_t y);
void vm_set_camera_follow_actor(uint8_t actor_id);
void vm_camera_at(uint8_t x);
void vm_set_camera_to(uint8_t x);
void vm_camera_pan_to(uint8_t x);
void vm_print_sentence(void);
void vm_revert_sentence(void);
void vm_verb_new(uint8_t slot, uint8_t verb_id, uint8_t x, uint8_t y, const char* name);
void vm_verb_delete(uint8_t slot);
void vm_verb_set_state(uint8_t slot, uint8_t state);
char *vm_verb_get_name(uint8_t slot);
uint8_t vm_savegame_exists(uint8_t slot);
uint8_t vm_save_game(uint8_t slot);
uint8_t vm_load_game(uint8_t slot);
void vm_handle_error_wrong_disk(uint8_t expected_disk);

static inline uint16_t vm_read_var(uint8_t var)
{
  volatile uint16_t value;
  value = make16(vm_state.variables_lo[var], vm_state.variables_hi[var]);
  return value;
}

static inline uint8_t vm_read_var8(uint8_t var)
{
  return vm_state.variables_lo[var];
}

static inline void vm_write_var(uint8_t var, uint16_t value)
{
  vm_state.variables_lo[var] = LSB(value);
  vm_state.variables_hi[var] = MSB(value);
  /*__asm volatile(" lda %[val]\n"
                 " sta variables_lo, x\n"
                 " lda %[val]+1\n"
                 " sta variables_hi, x"
                 :
                 : "Kx" (var), [val]"Kzp16" (value)
                 : "a");*/
}
