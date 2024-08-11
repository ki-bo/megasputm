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

#include "vm.h"
#include "actor.h"
#include "costume.h"
#include "diskio.h"
#include "error.h"
#include "gfx.h"
#include "input.h"
#include "inventory.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "script.h"
#include "sound.h"
#include "util.h"
#include "walk_box.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <mega65.h>

struct room_header {
  uint16_t chunk_size;
  uint16_t unused1;
  uint16_t bg_width;
  uint16_t bg_height;
  uint16_t unused2;
  uint16_t bg_data_offset;
  uint16_t bg_attr_offset;
  uint16_t bg_mask_flag_offset;
  uint16_t unused3;
  uint16_t unused4;
  uint8_t  num_objects;
  uint8_t  walk_boxes_offset;
  uint8_t  num_sounds;
  uint8_t  num_scripts;
  uint16_t exit_script_offset;
  uint16_t entry_script_offset;  
};

#pragma clang section bss="zdata"

struct vm vm_state;
uint8_t reset_game;
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint8_t proc_exec_count[NUM_SCRIPT_SLOTS];

volatile uint8_t script_watchdog;
uint8_t ui_state;
uint16_t camera_x;
uint16_t camera_target;
uint8_t camera_state;
uint8_t camera_follow_actor_id;

int8_t   proc_slot_table_idx;
uint8_t  proc_slot_table_exec;
uint8_t  proc_table_cleanup_needed;
uint8_t  active_script_slot;
uint16_t message_timer;
uint8_t  actor_talking;

uint8_t message_color;
char message_buffer[256];
char *message_ptr;
char *print_message_ptr;
uint8_t print_message_num_chars;

// room and object data
uint8_t  room_res_slot;
uint16_t room_width;
uint8_t  num_objects;
uint8_t  obj_page[MAX_OBJECTS];
uint8_t  obj_offset[MAX_OBJECTS];
uint16_t obj_id[MAX_OBJECTS];
uint8_t  screen_update_needed;

// sentence queue
struct sentence_stack_t sentence_stack;
char sentence_text[41];
uint8_t prev_sentence_highlighted;
uint8_t prev_verb_highlighted;
uint8_t prev_inventory_highlighted;
uint8_t sentence_length;

uint8_t inventory_pos;
uint8_t last_selected_actor;

static const uint8_t savegame_magic[] = {'M', '6', '5', 'M', 'C', 'M', 'N'};

// Private functions
static void reset_game_state(void);
static void set_proc_state(uint8_t slot, uint8_t state);
static void process_dialog(uint8_t jiffies_elapsed);
static void stop_current_actor_talking(void);
static void stop_all_dialog(void);
static uint8_t wait_for_jiffy(void);
static void read_objects(void);
static void redraw_screen(void);
static void handle_input(void);
static uint8_t match_parent_object_state(uint8_t parent, uint8_t expected_state);
static void update_script_timers(uint8_t elapsed_jiffies);
static const char *get_object_name(uint16_t global_object_id);
static uint8_t start_child_script_at_address(uint8_t script_slot, uint8_t res_slot, uint16_t offset);
static void execute_sentence_stack(void);
static void load_room(uint8_t room_no);
static uint16_t clamp_camera_x(uint16_t x);
static void update_actors(void);
static void animate_actors(void);
static void override_cutscene(void);
static void update_sentence_line(void);
static void update_sentence_highlighting(void);
static void add_string_to_sentence(const char *str, uint8_t prepend_space);
static void update_verb_interface(void);
static void update_verb_highlighting(void);
static uint8_t get_verb_slot_by_id(uint8_t verb_id);
static void select_verb(uint8_t verb_id);
static const char *get_preposition_name(uint8_t preposition);
static void update_inventory_interface();
static void update_inventory_highlighting(void);
static uint8_t get_hovered_inventory_slot(void);
static uint8_t inventory_ui_pos_to_x(uint8_t pos);
static uint8_t inventory_ui_pos_to_y(uint8_t pos);
static void inventory_scroll_up(void);
static void inventory_scroll_down(void);

// Private functions (code_main_private)
static void cleanup_slot_table();
static void read_walk_boxes(void);
static void clear_all_other_object_states(uint8_t local_object_id);
static void update_camera(void);
static uint8_t get_hovered_verb_slot(void);
static void verb_new(uint8_t slot, uint8_t verb_id, uint8_t x, uint8_t y, const char* name);
static void verb_delete(uint8_t slot);
static void freeze_non_active_scripts(void);
static void unfreeze_scripts(void);
static void add_string_to_sentence_priv(const char *str, uint8_t prepend_space);
static uint8_t resolve_position(uint16_t object_or_actor_id, uint8_t *x, uint8_t *y);


/**
  * @defgroup vm_init VM Init Functions
  * @{
  */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
  * @brief Initializes the virtual machine
  * 
  * Initializes the virtual machine by setting all script slots to free.
  *
  * Code section: code_init
  */
void vm_init(void)
{
  diskio_load_index();

  for (active_script_slot = 0; active_script_slot < NUM_SCRIPT_SLOTS; ++active_script_slot)
  {
    vm_state.proc_state[active_script_slot] = PROC_STATE_FREE;
  }

  camera_x = 20;
  camera_state = 0;
  camera_follow_actor_id = 0xff;
  actor_talking = 0xff;
  ui_state = 0;
  vm_write_var(VAR_CURSOR_STATE, 3);
  vm_state.message_speed = 6;
  prev_verb_highlighted = 0xff;
  prev_inventory_highlighted = 0xff;

  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    vm_state.verbs.id[i] = 0xff;
  }
}

/** @} */ // vm_init

//-----------------------------------------------------------------------------------------------

/**
  * @defgroup vm_public VM Public Functions
  * @{
  */
#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

/**
  * @brief Main loop of the virtual machine
  * 
  * The main loop of the virtual machine. This function will never return and runs an infinite
  * loop processing all game logic. It is waiting for at least one jiffy to pass before
  * processing the next cycle of the active scripts.
  *
  * Graphics update is triggered if at least one script has requested a screen update by setting
  * the screen_update_needed flag.
  *
  * Code section: code_main
  */
__task void vm_mainloop(void)
{
  // We will never return, so reset the stack pointer to the top of the stack
  __asm(" ldx #0xff\n"
        " txs"
        : /* no output operands */
        : /* no input operands */
        : "x" /* clobber list */);

  MAP_CS_GFX
  gfx_start();
  UNMAP_CS

  reset_game = RESET_RESTART;

  while (1) {
    if (reset_game == RESET_RESTART) {
      reset_game = 0;
      UNMAP_ALL
      MAP_CS_GFX
      gfx_fade_out();
      gfx_clear_bg_image();
      gfx_reset_actor_drawing();
      gfx_reset_palettes();
      MAP_CS_DISKIO
      diskio_load_game_objects();
      reset_game_state();
      UNMAP_CS

      script_schedule_init_script();
      wait_for_jiffy(); // this resets the elapsed jiffies timer
    }
  
    script_watchdog = 0;

    uint8_t elapsed_jiffies = 0;
    uint8_t jiffy_threshold = vm_read_var(VAR_TIMER_NEXT);
    do {
      elapsed_jiffies += wait_for_jiffy();
    }
    while (jiffy_threshold && elapsed_jiffies < jiffy_threshold);

    if (elapsed_jiffies > 15) {
      elapsed_jiffies = 15;
    }

    //VICIV.bordercol = 0x01;

    MAP_CS_DISKIO
    diskio_check_motor_off(elapsed_jiffies);
    MAP_CS_SOUND
    sound_stop_finished_slots();
    UNMAP_CS

    proc_table_cleanup_needed = 0;
    proc_slot_table_idx = -1;
    handle_input();

    update_script_timers(elapsed_jiffies);

    //debug_out("New cycle, %d scripts active", vm_state.num_active_proc_slots);
    memset(proc_exec_count, 0, NUM_SCRIPT_SLOTS);
    proc_slot_table_exec = 0;
    for (proc_slot_table_idx = 0; 
         proc_slot_table_idx < vm_state.num_active_proc_slots;
         ++proc_slot_table_idx)
    {
      // skip slots that have already been executed this cycle
      if (proc_slot_table_exec > proc_slot_table_idx) {
        continue;
      }
      active_script_slot = vm_state.proc_slot_table[proc_slot_table_idx];
      if (active_script_slot != 0xff && vm_state.proc_state[active_script_slot] == PROC_STATE_RUNNING) {
        script_execute_slot(active_script_slot);
      }
      if (reset_game == RESET_LOADED_GAME) {
        break;
      }
      ++proc_slot_table_exec;
    }

    if (reset_game == RESET_LOADED_GAME) {
      wait_for_jiffy(); // this resets the elapsed jiffies timer
      reset_game = 0;
      continue; // restart game loop if new game was loaded
    }

    if (proc_table_cleanup_needed) {
      MAP_CS_MAIN_PRIV
      cleanup_slot_table();
      UNMAP_CS
      //script_print_slot_table();
    }
    active_script_slot = 0xff;

    // executes the sentence script if any sentences are in the queue
    proc_slot_table_idx = -1;
    execute_sentence_stack();

    process_dialog(elapsed_jiffies);
    update_actors();
    animate_actors();
    MAP_CS_MAIN_PRIV
    update_camera();
    UNMAP_CS

    if (last_selected_actor != vm_read_var8(VAR_SELECTED_ACTOR)) {
      if (last_selected_actor != 0xff) { // makes sure inventory pos is kept when loading a savegame
        inventory_pos = 0;
      }
      last_selected_actor = vm_read_var8(VAR_SELECTED_ACTOR);
    }


    uint8_t flashlight_on = vm_read_var8(VAR_CURRENT_LIGHTS) == 12;
    //debug_out("Flashlight on: %d", flashlight_on);

    if (screen_update_needed) {
      //VICIV.bordercol = 0x01;
      MAP_CS_GFX
      if (screen_update_needed & SCREEN_UPDATE_FLASHLIGHT) {
        if (flashlight_on) {
          gfx_enable_flashlight();
        }
        else {
          gfx_disable_flashlight();
        }
        // whenever flashlight is toggled, we need to redraw the screen
        // as the rrb layout will change
        vm_update_bg();
        vm_update_actors();
      }
      if (screen_update_needed & SCREEN_UPDATE_BG) {
        redraw_screen();
      }
      if (screen_update_needed & SCREEN_UPDATE_ACTORS) {
        actor_sort_and_draw_all();
      }

      //VICIV.bordercol = 0x00;
      gfx_wait_vsync();
      //VICIV.bordercol = 0x03;
      if (screen_update_needed & SCREEN_UPDATE_DIALOG) {
        if (print_message_ptr) {
          gfx_print_dialog(message_color, print_message_ptr, print_message_num_chars);
        }
        else {
          gfx_clear_dialog();
        }
      }

      if (screen_update_needed & (SCREEN_UPDATE_BG | SCREEN_UPDATE_ACTORS)) {
        gfx_update_main_screen();
      }

      if (screen_update_needed & SCREEN_UPDATE_VERBS) {
        update_verb_interface();
      }

      if (screen_update_needed & SCREEN_UPDATE_INVENTORY) {
        update_inventory_interface();
      }

      if (screen_update_needed & SCREEN_UPDATE_SENTENCE) {
        update_sentence_line();
      }

      UNMAP_CS

      screen_update_needed = 0;
    }

    if (flashlight_on) {
      //VICIV.bordercol = 0x01;
      MAP_CS_GFX2
      gfx_update_flashlight();
      UNMAP_CS
      //VICIV.bordercol = 0x00;
      //gfx_flashlight_irq_update(1);
    }

    update_sentence_highlighting();
    update_verb_highlighting();
    update_inventory_highlighting();

    //VICIV.bordercol = 0x00;
  }
}

uint8_t vm_get_proc_state(uint8_t slot)
{
  return vm_state.proc_state[slot] & 0x07;
}

/**
  * @brief Returns the current state of the active process
  * 
  * The returned value is the complete state variable of the currently active process.
  * This includes the state in the lower bits and the additional state flags in the
  * upper bits.
  *
  * @return The current state of the active process
  *
  * Code section: code_main
  */
uint8_t vm_get_active_proc_state_and_flags(void)
{
  return vm_state.proc_state[active_script_slot];
}

void vm_change_ui_flags(uint8_t flags)
{
  if (flags & UI_FLAGS_APPLY_FREEZE) {
    SAVE_CS
    MAP_CS_MAIN_PRIV
    if (flags & UI_FLAGS_ENABLE_FREEZE) {
      freeze_non_active_scripts();
    }
    else {
      unfreeze_scripts();
    }
    RESTORE_CS
  }

  if (flags & UI_FLAGS_APPLY_CURSOR) {
    ui_state = (ui_state & ~UI_FLAGS_ENABLE_CURSOR) | (flags & UI_FLAGS_ENABLE_CURSOR);
  }

  if (flags & UI_FLAGS_APPLY_INTERFACE) {
    ui_state = (ui_state & ~(UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS)) |
                (flags & (UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS));
    screen_update_needed |= SCREEN_UPDATE_SENTENCE | SCREEN_UPDATE_VERBS | SCREEN_UPDATE_INVENTORY;
    MAP_CS_GFX
    if (ui_state & UI_FLAGS_ENABLE_SENTENCE) {
      vm_print_sentence();
    } else {
      gfx_clear_sentence();
    }
    if (!(ui_state & UI_FLAGS_ENABLE_VERBS)) {
      gfx_clear_verbs();
    }
    if (!(ui_state & UI_FLAGS_ENABLE_INVENTORY)) {
      gfx_clear_inventory();
    }
    UNMAP_CS
  }

  //debug_out("UI state enable-cursor: %d", ui_state & UI_FLAGS_ENABLE_CURSOR);
  //debug_out("cursor state: %d", vm_read_var8(VAR_CURSOR_STATE));
}

/**
  * @brief Switches the scene to another room
  * 
  * Switches the scene to another room by deactivating the current room and activating the new
  * room. The new room number is stored in VAR_ROOM_NO. The function will also decode the new
  * background image and redraw the screen, including drawing all objects currently visible.
  *
  * Code section: code_main
  */
void vm_set_current_room(uint8_t room_no)
{
  // save DS
  SAVE_DS_AUTO_RESTORE

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;

  if (script_is_room_object_script(active_script_slot)) {
    script_stop_slot(active_script_slot);
  }

  // exit and free old room
  //debug_out("Deactivating old room %d", vm_read_var8(VAR_SELECTED_ROOM));
  stop_all_dialog();
  vm_revert_sentence();

  MAP_CS_GFX
  gfx_clear_dialog();
  gfx_fade_out();
  UNMAP_CS

  if (vm_read_var(VAR_SELECTED_ROOM) != 0) {
    map_ds_resource(room_res_slot);
    uint16_t exit_script_offset = room_hdr->exit_script_offset;
    if (exit_script_offset) {
      UNMAP_CS
      script_execute_room_script(exit_script_offset);
    }

    // stop all room scripts before deactivating the room
    for (uint8_t i = 0; i < NUM_SCRIPT_SLOTS; ++i) {
      if (vm_state.proc_state[i] != PROC_STATE_FREE && script_is_room_object_script(i)) {
        script_stop_slot(i);
      }
    }

    res_deactivate_slot(room_res_slot);
  }

  vm_write_var(VAR_SELECTED_ROOM, room_no);

  if (room_no == 0) {
    MAP_CS_GFX
    gfx_clear_bg_image();
    actor_room_changed();
    num_objects = 0;
  }
  else {
    debug_out("Activating new room %d", room_no);
    // activate new room data
    load_room(room_no);
    camera_x = 20;
    vm_write_var(VAR_CAMERA_X, camera_x);
    actor_room_changed();

    // run entry script
    uint16_t entry_script_offset = room_hdr->entry_script_offset;
    if (entry_script_offset) {
      UNMAP_CS
      script_execute_room_script(entry_script_offset);
    }
  }

  redraw_screen();
  vm_update_bg();
  vm_print_sentence();
  UNMAP_CS
}

uint8_t vm_get_room_object_script_offset(uint8_t verb, uint8_t local_object_id, uint8_t is_inventory)
{
  SAVE_DS_AUTO_RESTORE

  uint8_t *ptr;

  if (is_inventory) {
    UNMAP_DS
    ptr = NEAR_U8_PTR(vm_state.inv_objects[local_object_id]);
  }
  else {
    map_ds_resource(obj_page[local_object_id]);
    ptr = NEAR_U8_PTR(RES_MAPPED + obj_offset[local_object_id]);
  }
  ptr += 15;
  
  uint8_t script_offset = 0;

  //debug_out("Searching for verb %x in object %d", verb, local_object_id);

  while (*ptr != 0) {
    //debug_out("Verb %x, offset %x", *ptr, *(ptr + 1));
    if (*ptr == verb || *ptr == 0xff) {
      script_offset = *(ptr + 1);
      break;
    }
    ptr += 2;
  }

  return script_offset;
}

/**
  * @brief Delays the currently active script
  *
  * Initializes the delay timer for the currently active script. Note that the timer
  * is actually counting up, so the script will provide a negative number of ticks
  * as parameter to this function.
  *
  * The scheduler will suspend execution of the currently active process until the
  * timer becomes zero or positive.
  *
  * The process state of the currently active process is set to PROC_STATE_WAITING_FOR_TIMER
  * when this function returns.
  * 
  * @param timer_value The negative amount of ticks to suspend
  *
  * Code section: code_main
  */
void vm_set_script_wait_timer(int32_t negative_ticks)
{
  vm_state.proc_state[active_script_slot] = PROC_STATE_WAITING_FOR_TIMER;
  vm_state.proc_wait_timer[active_script_slot] = negative_ticks;
}

/**
  * @brief Starts a cutscene
  *
  * Starts a cutscene by saving the current game state and setting the cursor and interface
  * state to the cutscene state. The cutscene state is stored in the cutscene variables.
  *
  * Code section: code_main
  */
void vm_cut_scene_begin(void)
{
  vm_state.cs_room = vm_read_var8(VAR_SELECTED_ROOM);
  vm_state.cs_cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  vm_state.cs_proc_slot = 0xff;
  vm_state.cs_override_pc = 0;
  vm_state.cs_camera_state = camera_state;
  vm_state.cs_ui_state = ui_state;
  vm_write_var(VAR_CURSOR_STATE, 0x80);
  vm_change_ui_flags(UI_FLAGS_APPLY_FREEZE | UI_FLAGS_ENABLE_FREEZE |
                     UI_FLAGS_APPLY_CURSOR |
                     UI_FLAGS_APPLY_INTERFACE);
  // make sure current activity is stopped when entering cutscene
  vm_revert_sentence();
  sentence_stack.num_entries = 0;
  script_stop(SCRIPT_ID_SENTENCE);
}

void vm_cut_scene_end(void)
{
  //debug_out("cut-scene ended");
  vm_write_var(VAR_OVERRIDE_HIT, 0);
  camera_state = vm_state.cs_camera_state;
  if (camera_state & CAMERA_STATE_FOLLOW_ACTOR) {
    vm_set_camera_follow_actor(vm_read_var8(VAR_SELECTED_ACTOR));
  }
  else if (vm_state.cs_room != 0 && vm_read_var8(VAR_SELECTED_ROOM) != vm_state.cs_room) {
    vm_set_current_room(vm_state.cs_room);
  }
  vm_write_var(VAR_CURSOR_STATE, vm_state.cs_cursor_state);
  vm_change_ui_flags(vm_state.cs_ui_state | UI_FLAGS_APPLY_CURSOR | UI_FLAGS_APPLY_FREEZE | UI_FLAGS_APPLY_INTERFACE);
  vm_state.cs_proc_slot = 0xff;
  vm_state.cs_override_pc = 0;

  //debug_out("  cursor state %d, ui state %d", vm_read_var8(VAR_CURSOR_STATE), ui_state);
}

void vm_begin_override(void)
{
  vm_state.cs_proc_slot = active_script_slot;
  vm_state.cs_override_pc = script_get_current_pc();
}

/**
  * @brief Outputs a text message on screen for the specified actor
  * 
  * Outputs dialog on screen for the specified actor. The text message is stored in the 
  * text message buffer and the text timer is set to the number of jiffies needed to display
  * the message. The variable VAR_MESSAGE_GOING is set to 1 while the message is presented on
  * screen. It will be reset back to 0 in the main loop once the message timer expires.
  * 
  * For general text outputs, actor_id 0xff can be used. No talking animation will be triggered
  * in that case.
  *
  * The text message is directly printed on screen without waiting for a redraw event.

  * @param actor_id ID of the actor talking, will be used to select the configured text color
  *                 for the text output on screen.
  *
  * Code section: code_main
  */
void vm_say_line(uint8_t actor_id)
{
  // check if there is already a message going on
  if (vm_read_var(VAR_MESSAGE_GOING)) {
    stop_current_actor_talking();
  }

  actor_talking = actor_id;
 
  if (actor_id == 0xff) {
    // print-line " " will shut everybody's mouth
    if (message_buffer[0] == ' ' && message_buffer[1] == '\0') {
      stop_all_dialog();
      return;
    }

    message_color = actors.talk_color[0];
  } else {
    actor_start_talking(actor_id);
    message_color = actors.talk_color[actor_id];
  }

  message_ptr = message_buffer;
  message_timer = 0;
  vm_write_var(VAR_MESSAGE_GOING, 1);
  vm_write_var(VAR_MSGLEN, 0);
}

uint8_t vm_get_first_script_slot_by_script_id(uint8_t script_id)
{
  for (uint8_t table_idx = 0; table_idx < vm_state.num_active_proc_slots; ++table_idx)
  {
    uint8_t slot = vm_state.proc_slot_table[table_idx];
    if (slot != 0xff &&
        vm_state.proc_type[slot] == PROC_TYPE_GLOBAL &&
        vm_state.proc_script_or_object_id[slot] == script_id)
    {
      return slot;
    }
  }

  return 0xff;
}

uint8_t vm_is_script_running(uint8_t script_id)
{
  return vm_get_first_script_slot_by_script_id(script_id) != 0xff;
}

/**
  * @brief Requests that the screen is updated
  *
  * The screen update flag is set to 1, which will trigger a screen update in the main loop.
  * The redraw will happen after all active scripts have finished their current execution cycle.
  *
  * Code section: code_main
  */
void vm_update_bg(void)
{
  screen_update_needed |= SCREEN_UPDATE_BG;
}

void vm_update_flashlight(void)
{
  screen_update_needed |= SCREEN_UPDATE_FLASHLIGHT;
}

void vm_update_dialog(void)
{
  screen_update_needed |= SCREEN_UPDATE_DIALOG;
}

void vm_update_actors(void)
{
  screen_update_needed |= SCREEN_UPDATE_ACTORS;
}

void vm_update_sentence(void)
{
  screen_update_needed |= SCREEN_UPDATE_SENTENCE;
}

void vm_update_verbs(void)
{
  screen_update_needed |= SCREEN_UPDATE_VERBS;
}

void vm_update_inventory(void)
{
  screen_update_needed |= SCREEN_UPDATE_INVENTORY;
}

struct object_code *vm_get_room_object_hdr(uint16_t global_object_id)
{
  uint8_t id = vm_get_local_object_id(global_object_id);
  if (id == 0xff) {
    return NULL;
  }

  map_ds_resource(obj_page[id]);
  return (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[id]);
}

/**
  * @brief Returns the object id at the given position
  *
  * Returns the object id of the object at the given position. The function will iterate over
  * all objects and check if the object is currently active and if the object position matches
  * the one provided. If a match is found, the object id is returned.
  *
  * @param x The x position of the object in scene coordinates
  * @param y The y position of the object in scene coordinates
  * @return The object id of the object at the given position
  *
  * Code section: code_main
  */
uint16_t vm_get_object_at(uint8_t x, uint8_t y)
{
  SAVE_DS_AUTO_RESTORE

  uint16_t found_obj_id = 0;
  y >>= 2;


  for (uint8_t i = 0; i < num_objects; ++i) {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)(RES_MAPPED + obj_offset[i]);
    //debug_out("Checking object %d at %d, %d state %d - parent_state %d", obj_hdr->id, obj_hdr->pos_x, obj_hdr->pos_y_and_parent_state & 0x7f, vm_state.global_game_objects[obj_hdr->id] & OBJ_STATE, obj_hdr->pos_y_and_parent_state & 0x80);
    //debug_out("  obj_state %x", vm_state.global_game_objects[obj_hdr->id]);
    if (vm_state.global_game_objects[obj_hdr->id] & OBJ_CLASS_UNTOUCHABLE) {
      continue;
    }
    if (obj_hdr->parent != 0) {
      if (!match_parent_object_state(obj_hdr->parent - 1, obj_hdr->pos_y_and_parent_state & 0x80)) {
        continue;
      }
    }

    uint8_t width  = obj_hdr->width;
    uint8_t height = obj_hdr->height_and_actor_dir >> 3;
    uint8_t obj_x  = obj_hdr->pos_x;
    uint8_t obj_y  = obj_hdr->pos_y_and_parent_state & 0x7f;

    if (x >= obj_x && x < obj_x + width && y >= obj_y && y < obj_y + height)
    {
      found_obj_id = obj_hdr->id;
      break;
    }
  }

  return found_obj_id;
}

uint8_t vm_get_object_position(uint16_t global_object_id, uint8_t *x, uint8_t *y)
{
  SAVE_DS_AUTO_RESTORE

  // check if object is an actor
  if (global_object_id <= NUM_ACTORS) {
    // in current room?
    if (actors.local_id[global_object_id] == 0xff) {
      return 0;
    }
    *x = actors.x[global_object_id];
    *y = actors.y[global_object_id] - actors.elevation[global_object_id];
    return 1;
  }

  // check if object is in current room
  __auto_type obj_hdr = vm_get_room_object_hdr(global_object_id);
  if (!obj_hdr) {
    return 0;
  }
  *x = obj_hdr->pos_x;
  *y = obj_hdr->pos_y_and_parent_state & 0x7f;
  return 2;
}

void vm_set_object_name(uint16_t global_object_id, const char *name)
{
  SAVE_DS_AUTO_RESTORE

  char *name_ptr = (char *)get_object_name(global_object_id);
  if (name_ptr) {
    strcpy(name_ptr, name);
  }
}

uint8_t vm_get_local_object_id(uint16_t global_object_id)
{
  for (uint8_t i = 0; i < num_objects; ++i)
  {
    if (obj_id[i] == global_object_id)
    {
      return i;
    }
  }

  return 0xff;
}

uint8_t vm_calc_proximity(uint16_t actor_or_object_id1, uint16_t actor_or_object_id2)
{
  SAVE_DS_AUTO_RESTORE

  uint8_t x1, y1, x2, y2;
  uint8_t is_actor1 = 0;
  uint8_t is_actor2 = 0;

  // resolve the positions of the objects (or actors if id < number of actors)
  SAVE_CS_AUTO_RESTORE
  MAP_CS_MAIN_PRIV
  is_actor1 = resolve_position(actor_or_object_id1, &x1, &y1);
  is_actor2 = resolve_position(actor_or_object_id2, &x2, &y2);

  // if one of the objects is not found, we set the distance to 0xff (maximum)
  if (x1 == 0xff || x2 == 0xff) {
    return 0xff;
  }

  if (is_actor1 && !is_actor2) {
    // if object1 is an actor and object2 is an object, we need to correct the walk-to position
    // of the object to be in the closest box (seems walk-to positions are often outside of
    // walk boxes, eg. the doorbell in maniac mansion)
    walkbox_correct_position_to_closest_box(&x2, &y2);
  }

  // calculate the distance between the two objects
  uint8_t dx = abs8((int8_t)x1 - (int8_t)x2);
  uint8_t dy = abs8((int8_t)y1 - (int8_t)y2);

  return max(dx, dy);
}

void vm_draw_object(uint8_t local_id, uint8_t x, uint8_t y)
{
  SAVE_DS_AUTO_RESTORE

  SAVE_CS_AUTO_RESTORE
  MAP_CS_MAIN_PRIV
  clear_all_other_object_states(local_id);

  map_ds_resource(obj_page[local_id]);
  __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[local_id]);
  
  uint8_t width  = obj_hdr->width;
  uint8_t height = obj_hdr->height_and_actor_dir >> 3;

  if (x == 0xff) {
    x = obj_hdr->pos_x;
  }
  if (y == 0xff) {
    y = obj_hdr->pos_y_and_parent_state & 0x7f;
  }

  int8_t screen_x = (int8_t)x - camera_x + 20;

  if (screen_x >= 40 || screen_x + width <= 0 || y >= 16) {
    return;
  }

  MAP_CS_GFX
  UNMAP_DS
  gfx_draw_object(local_id, screen_x, y);

  vm_update_actors();
}

void vm_set_camera_follow_actor(uint8_t actor_id)
{
  //debug_out("set-camera-follow-actor %d", actor_id);
  uint8_t room_of_actor = actors.room[actor_id];
  if (room_of_actor != vm_read_var8(VAR_SELECTED_ROOM)) {
    vm_set_current_room(room_of_actor);
    vm_set_camera_to(actors.x[actor_id]);
  }
  else {
    vm_camera_at(actors.x[actor_id]);
  }

  camera_follow_actor_id = actor_id;
  camera_state           = CAMERA_STATE_FOLLOW_ACTOR;
}

void vm_camera_at(uint8_t x)
{
  if (abs((int16_t)camera_x - (int16_t)x) > 20) {
    vm_set_camera_to(x);
  }
  else {
    vm_camera_pan_to(x);
  }
}

void vm_set_camera_to(uint8_t x)
{
  camera_x = clamp_camera_x(x);
  vm_write_var(VAR_CAMERA_X, camera_x);
  vm_update_bg();
  vm_update_actors();
  camera_state = 0;
}

void vm_camera_pan_to(uint8_t x)
{
  camera_target          = clamp_camera_x(x);
  camera_follow_actor_id = 0xff;
  camera_state           = CAMERA_STATE_MOVE_TO_TARGET_POS;
}

void vm_print_sentence(void)
{
  SAVE_DS_AUTO_RESTORE

  sentence_length = 0;

  //debug_out("Printing sentence");
  uint8_t verb_slot = get_verb_slot_by_id(vm_read_var8(VAR_SENTENCE_VERB));
  //debug_out("  Verb id %d slot %d", vm_read_var8(VAR_SENTENCE_VERB), verb_slot);
  if (verb_slot != 0xff) {
    UNMAP_DS
    add_string_to_sentence(vm_state.verbs.name[verb_slot], 0);
  }

  uint16_t noun1 = vm_read_var(VAR_SENTENCE_NOUN1);
  if (noun1) {
    const char *noun1_name = get_object_name(noun1);
    if (noun1_name) {
      //debug_out("  Noun1 name: %s", noun1_name);
      add_string_to_sentence(noun1_name, 1);
    }
  }
  
  uint8_t preposition = vm_read_var8(VAR_SENTENCE_PREPOSITION);
  if (preposition) {
    const char *preposition_name = get_preposition_name(preposition);
    if (preposition_name) {
      //debug_out("  Preposition name: %s", preposition_name);
      add_string_to_sentence(preposition_name, 1);
    }
  }

  uint16_t noun2 = vm_read_var(VAR_SENTENCE_NOUN2);
  if (noun2) {
    const char *noun2_name = get_object_name(noun2);
    if (noun2_name) {
      //debug_out("  Noun2 name: %s", noun2_name);
      add_string_to_sentence(noun2_name, 1);
    }
  }

  uint8_t num_chars = sentence_length;
  while (num_chars < 40) {
    sentence_text[num_chars] = '@';
    ++num_chars;
  }  
  sentence_text[40] = '\0';

  vm_update_sentence();
}

void vm_revert_sentence(void)
{
  //debug_out("revert-sentence");
  vm_write_var(VAR_SENTENCE_VERB, vm_read_var(VAR_DEFAULT_VERB));
  vm_write_var(VAR_SENTENCE_NOUN1, 0);
  vm_write_var(VAR_SENTENCE_NOUN2, 0);
  vm_write_var(VAR_SENTENCE_PREPOSITION, 0);
}

void vm_verb_new(uint8_t slot, uint8_t verb_id, uint8_t x, uint8_t y, const char* name)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_MAIN_PRIV
  verb_new(slot, verb_id, x, y, name);
}

void vm_verb_delete(uint8_t slot)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_MAIN_PRIV
  verb_delete(slot);
}

void vm_verb_set_state(uint8_t slot, uint8_t state)
{
  vm_state.verbs.state[slot] = state;
}

char *vm_verb_get_name(uint8_t slot)
{
  return vm_state.verbs.name[slot];
}

uint8_t vm_savegame_exists(uint8_t slot)
{
  char filename[11];
  sprintf(filename, "MM.SAV.%u", slot);
  MAP_CS_DISKIO
  uint8_t exists = diskio_file_exists(filename);
  UNMAP_CS
  return exists;
}

uint8_t vm_save_game(uint8_t slot)
{
  char    filename[11];
  uint8_t version = 0;

  uint8_t   heap_slot;
  uint16_t *locked_resources;
  uint8_t   num_locked_resources;

  SAVE_DS_AUTO_RESTORE
  SAVE_CS_AUTO_RESTORE
  MAP_CS_DISKIO

  heap_slot            = res_reserve_heap(5);
  map_ds_resource(heap_slot);
  locked_resources     = NEAR_U16_PTR(RES_MAPPED);
  num_locked_resources = res_get_locked_resources(locked_resources, 255);

  uint8_t *pal_ptr = NEAR_U8_PTR(RES_MAPPED + 0x200);
  memcpy(pal_ptr, (const void *)0xd100, 0x300);

  sprintf(filename, "MM.SAV.%u", slot);
  diskio_open_for_writing();
  diskio_write((uint8_t __huge *)savegame_magic, sizeof(savegame_magic));
  diskio_write((uint8_t __huge *)&version, 1);

  // write global vm state
  diskio_write((uint8_t __huge *)&vm_state, sizeof(vm_state));

  // write inventory objects
  UNMAP_DS
  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    __auto_type object = vm_state.inv_objects[i];
    if (object) {
      diskio_write((uint8_t __huge *)object, object->chunk_size);
    } 
  }

  // write actors
  diskio_write((uint8_t __huge *)&actors, sizeof(actors));

  // write locked resources states
  diskio_write((uint8_t __huge *)&num_locked_resources, 1);
  diskio_write(res_get_huge_ptr(heap_slot), num_locked_resources * 2);

  // write palettes
  diskio_write(res_get_huge_ptr(heap_slot + 2), 0x300);

  diskio_close_for_writing(filename, FILE_TYPE_SEQ);

  res_free_heap(heap_slot);

  return 0;
}

uint8_t vm_load_game(uint8_t slot)
{
  char     filename[11];
  uint8_t  magic_hdr[8];
  uint8_t  num_locked_resources;

  uint8_t      cur_pc           = script_get_current_pc();
  uint8_t      cur_script_id    = vm_state.proc_script_or_object_id[active_script_slot];
  uint8_t      heap_slot        = res_reserve_heap(5);
  __auto_type  heap_obj_huge    = (struct object_code __huge *)res_get_huge_ptr(heap_slot);
  __auto_type  heap_obj_near    = (struct object_code *)RES_MAPPED;
  uint16_t    *locked_resources = NEAR_U16_PTR(RES_MAPPED);
  uint8_t     *pal_ptr          = NEAR_U8_PTR(RES_MAPPED + 0x200);

  SAVE_CS_AUTO_RESTORE
  MAP_CS_DISKIO

  sprintf(filename, "MM.SAV.%u", slot);
  diskio_open_for_reading(filename, FILE_TYPE_SEQ);
  diskio_read(magic_hdr, sizeof(magic_hdr));
  for (uint8_t i = 0; i < 7; ++i) {
    if (magic_hdr[i] != savegame_magic[i]) {
      diskio_close_for_reading();
      return 1;
    }
  }
  // check version
  if (magic_hdr[7] != 0) {
    diskio_close_for_reading();
    return 1;
  }

  reset_game_state();

  // read data from disk
  diskio_read((uint8_t *)&vm_state, sizeof(vm_state));

  // read inventory objects
  map_ds_resource(heap_slot);
  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    if (!vm_state.inv_objects[i]) {
      continue;
    }
    diskio_read((uint8_t *)heap_obj_near, 2);
    diskio_read(((uint8_t *)heap_obj_near) + 2, heap_obj_near->chunk_size - 2);
    // This will overwrite the inventory pointer vm_state.inv_objects[i] as it will
    // allocate new memory for the object via malloc.
    inv_copy_object_data(i, heap_obj_huge);
  }

  // read actors
  diskio_read((uint8_t *)&actors, sizeof(actors));

  // read locked resources states
  diskio_read((uint8_t *)&num_locked_resources, 1);
  map_ds_resource(heap_slot);
  diskio_read((uint8_t *)locked_resources, num_locked_resources * 2);

  // read palettes
  diskio_read(pal_ptr, 0x300);

  diskio_close_for_reading();

  // restore palettes
  memcpy((void *)0xd100, pal_ptr, 0x300);

  // actor-in-the-dark palette (only needed to fix older broken savegames that don't have it)
  for (uint8_t i = 0xf0; i != 0; ++i) {
    uint8_t src_index = (i == 0xfc) ? 0 : 8;
    PALETTE.red[i]   = PALETTE.red[src_index];
    PALETTE.green[i] = PALETTE.green[src_index];
    PALETTE.blue[i]  = PALETTE.blue[src_index];
  }

  // ensure active scripts are in memory and update their res_slot
  for (uint8_t i = 0; i < vm_state.num_active_proc_slots; ++i) {
    uint8_t slot = vm_state.proc_slot_table[i];
    if (slot != 0xff) {
      if (vm_state.proc_script_or_object_id[slot] == cur_script_id) {
        // found the slot where the active script is running
        active_script_slot = slot;
      }
      uint8_t script_id = vm_state.proc_script_or_object_id[slot];
      uint8_t script_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
      res_activate_slot(script_page);
      proc_res_slot[slot] = script_page;
    }
  }

  // lock resources
  for (uint8_t i = 0; i < num_locked_resources; ++i, ++locked_resources) {
    uint8_t  type  = MSB(*locked_resources);
    uint8_t  index = LSB(*locked_resources);
    res_provide(type, index, 0);
    res_lock(type, index, 0);
  }
  
  res_free_heap(heap_slot);

  load_room(vm_read_var8(VAR_SELECTED_ROOM));

  UNMAP_CS
  script_break();

  reset_game = RESET_LOADED_GAME;

  return 0;
}

void vm_handle_error_wrong_disk(uint8_t expected_disk)
{
  SAVE_CS_AUTO_RESTORE

  char error_str[41];
  sprintf(error_str, "Please Insert Disk %d.  Press RETURN", expected_disk);

  input_key_pressed = 0; // ack the space key
  MAP_CS_GFX
  gfx_clear_dialog();
  gfx_print_interface_text(0, 0, error_str, TEXT_STYLE_SENTENCE);
  script_watchdog = WATCHDOG_TIMEOUT;

  MAP_CS_DISKIO
  while (1) {
    if (input_key_pressed) {
      if (input_key_pressed == 0x0d) {
        wait_for_jiffy();  // this resets the elapsed jiffies timer
        break;
      }
      else {
        // ack all other key presses
        input_key_pressed = 0;
      }
    }
    uint8_t elapsed_jiffies = wait_for_jiffy();
    diskio_check_motor_off(elapsed_jiffies);
  }

  MAP_CS_GFX
  gfx_clear_dialog();
  UNMAP_CS
}

/// @} // vm_public

//-----------------------------------------------------------------------------------------------

/**
  * @defgroup vm_private VM Private Functions
  * @{
  */

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"
static void reset_game_state(void)
{
  for (uint8_t i = 0; i < NUM_SCRIPT_SLOTS; ++i) {
    vm_state.proc_state[i]               = PROC_STATE_FREE;
    vm_state.proc_script_or_object_id[i] = 0xff;
    vm_state.proc_parent[i]              = 0xff;
    vm_state.proc_wait_timer[i]          = 0;
  }

  UNMAP_DS
  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    vm_state.verbs.id[i] = 0xff;
    free(vm_state.verbs.name[i]);
  }
  memset(&vm_state.verbs.name, 0, sizeof(vm_state.verbs.name));
  for (uint8_t i = 0; i < MAX_INVENTORY; ++i) {
    free(vm_state.inv_objects[i]);
  }
  memset(&vm_state.inv_objects, 0, sizeof(vm_state.inv_objects));
  res_deactivate_and_unlock_all();

  uint8_t var_idx = 0;
  do {
    vm_write_var(var_idx, 0);
  }
  while (++var_idx != 0);

  vm_state.num_actor_palettes = 1;
  memset(&actors, 0, sizeof(actors));
  for (uint8_t i = 0; i < NUM_ACTORS; ++i) {
    actors.local_id[i]    = 0xff;
    actors.palette_idx[i] = 1; // default actor palette is index 1
    if (i < MAX_LOCAL_ACTORS) {
      local_actors.global_id[i] = 0xff;
    }
  }

  vm_state.inv_num_objects   = 0;
  vm_state.flashlight_width  = 6;
  vm_state.flashlight_height = 4;

  active_script_slot         = 0xff;
  camera_x                   = 20;
  camera_state               = 0;
  camera_follow_actor_id     = 0xff;
  actor_talking              = 0xff;
  // this ensures that inventory_pos won't get reset to 0
  // (which should not happen when loading a savegame)
  last_selected_actor        = 0xff;
  vm_state.message_speed     = 6;
  message_timer              = 0;
  message_ptr                = NULL;
  print_message_ptr          = NULL;
  prev_verb_highlighted      = 0xff;
  prev_inventory_highlighted = 0xff;

  ui_state = UI_FLAGS_ENABLE_CURSOR | UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS;
  vm_write_var(VAR_CURSOR_STATE, 3);
}

static void set_proc_state(uint8_t slot, uint8_t state)
{
  vm_state.proc_state[slot] &= ~0x07; // clear state without changing flags
  vm_state.proc_state[slot] |= state;
}

static uint8_t is_room_object_script(uint8_t slot)
{
  return (vm_state.proc_type[slot] & PROC_TYPE_GLOBAL) == 0;
}

/**
  * @brief Processes the dialog timer
  *
  * Decreases the dialog timer by the number of jiffies elapsed since the last call. Clears
  * the dialog area on screen and resets the dialog active variable when the timer reaches
  * zero. Note that the dialog timer is counting down.
  *
  * Parsing handles special control characters:
  * - 0x01: Next line
  * - 0x02: Keep dialog - no dialog timer will elapse, new messages will be appended
  * - 0x03: Break dialog - break dialog into multiple parts, each part to be displayed
  *                        one after another and with its own dialog timer
  * 
  * @param jiffies_elapsed The number of jiffies elapsed since the last call
  *
  * Code section: code_main
  */
static void process_dialog(uint8_t jiffies_elapsed)
{
  if (message_ptr == NULL && message_timer == 0)
  {
    // no new dialog to process and no recent one to handle
    return;
  }

  if (message_ptr && message_timer == 0) {
    print_message_num_chars = 0;
    uint8_t timer_chars = 0;
    while (1) {
      char c = message_ptr[print_message_num_chars];
      if (c == '\0') {
        break;
      }
      if (c == 0x02 || c == 0x03) {
        break;
      }
      ++print_message_num_chars;
      if (c == 0x01 || c == 0x20) {
        continue;
      }
      ++timer_chars;
    }

    vm_write_var(VAR_MSGLEN, vm_read_var8(VAR_MSGLEN) + print_message_num_chars);

    print_message_ptr = message_ptr;
    vm_update_dialog();
    message_timer = 60 + (uint16_t)timer_chars * (uint16_t)vm_state.message_speed;
    message_ptr += print_message_num_chars;
    if (*message_ptr == '\0') {
      message_ptr = NULL;
    }
    else if (*message_ptr == 0x03) {
      ++message_ptr;
    }
    return;
  }

  if (!message_ptr || *message_ptr != 0x02) {
    if (message_timer >= jiffies_elapsed)
    {
      message_timer -= jiffies_elapsed;
    }
    else
    {
      message_timer = 0;
    }
  }

  if (!message_timer && !message_ptr) {
    stop_current_actor_talking();
  }
}

static void stop_current_actor_talking(void)
{
  if (actor_talking != 0xff) {
    actor_stop_talking(actor_talking);
  }
  print_message_ptr = NULL;
  vm_update_dialog();
  vm_write_var(VAR_MESSAGE_GOING, 0);
  vm_write_var(VAR_MSGLEN, 0);
}

static void stop_all_dialog(void)
{
  print_message_ptr = NULL;
  message_ptr = NULL;
  message_timer = 0;
  vm_update_dialog();
  // stop talking animation for all actors
  actor_stop_talking(0xff);
  vm_write_var(VAR_MESSAGE_GOING, 0);
  vm_write_var(VAR_MSGLEN, 0);
}

/**
  * @brief Waits for at least one jiffy to have passed since the last call
  *
  * Waits until at least one jiffy has passed since the last call. The function will return
  * the number of jiffies elapsed since the last call. If at least one jiffy has passed
  * already since the last call, the function will return immediately.
  * 
  * @return The number of jiffies elapsed since the last call
  *
  * Code section: code_main
  */
static uint8_t wait_for_jiffy(void)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_GFX
  uint8_t num_jiffies_elapsed = gfx_wait_for_jiffy_timer();
  return num_jiffies_elapsed;
}

/**
  * @brief Reads object data for the current room
  *
  * Reads all offsets for object images and object data from the current room resource. The
  * function will read the number of objects from the room header and then read the object
  * offsets and image offsets from the room resource. The function will then read the object
  * metadata and image data for each object and store it in the object data arrays.
  *
  * The images are decoded and kept in gfx memory for fast drawing on screen.
  *
  * Needs cs_gfx mapped and the room resource mapped to DS.
  *
  * Code section: code_main
  */
static void read_objects(void)
{
  struct offset {
    uint8_t lsb;
    uint8_t msb;
  };

  uint8_t __huge* room_ptr = res_get_huge_ptr(room_res_slot);

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;
  num_objects = room_hdr->num_objects;

  uint16_t *image_offset = NEAR_U16_PTR(RES_MAPPED + sizeof(struct room_header));
  struct offset *obj_hdr_offset = (struct offset *)(image_offset + num_objects);

  for (uint8_t i = 0; i < num_objects; ++i)
  {
    // read object and image offsets
    uint8_t cur_obj_offset = obj_hdr_offset->lsb;
    uint8_t cur_obj_page = room_res_slot + obj_hdr_offset->msb;
    uint16_t cur_image_offset = *image_offset;
    ++obj_hdr_offset;
    ++image_offset;
    obj_offset[i] = cur_obj_offset;
    obj_page[i] = cur_obj_page;

    // read object metadata from header
    map_ds_resource(cur_obj_page);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + cur_obj_offset);
    obj_id[i] = obj_hdr->id;

    // read object image
    gfx_set_object_image(room_ptr + cur_image_offset, 
                         obj_hdr->pos_x, 
                         obj_hdr->pos_y_and_parent_state & 0x7f, 
                         obj_hdr->width, 
                         obj_hdr->height_and_actor_dir >> 3);

    // reset ds back to room header
    map_ds_resource(room_res_slot);
  }
}

/**
  * @brief Redraws the screen
  *
  * Redraws the screen by redrawing the background and all objects currently visible on screen.
  * The function will read the object data from the object data arrays and draw the objects
  * on screen at their current position.
  *
  * @note This function will change CS and DS but won't restore them.
  *
  * Code section: code_main
  */
static void redraw_screen(void)
{
  MAP_CS_GFX

  // draw background
  gfx_draw_bg(vm_read_var8(VAR_CURRENT_LIGHTS) == 11);

  // draw all visible room objects
  for (int8_t i = num_objects - 1; i >= 0; --i)
  {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
    // OBJ_STATE is used to determine visibility of object in this room
    if (!(vm_state.global_game_objects[obj_hdr->id] & OBJ_STATE)) {
      continue;
    }
    if (obj_hdr->parent != 0) {
      if (!match_parent_object_state(obj_hdr->parent - 1, obj_hdr->pos_y_and_parent_state & 0x80)) {
        continue;
      }
    }
    int8_t screen_x = obj_hdr->pos_x - camera_x + 20;
    if (screen_x >= 40 || screen_x + obj_hdr->width <= 0) {
      continue;
    }
    int8_t screen_y = obj_hdr->pos_y_and_parent_state & 0x7f;
    UNMAP_DS

    gfx_draw_object(i, screen_x, screen_y);
  }
}

static void handle_input(void)
{
  // mouse cursor handling
  static uint8_t last_input_button_pressed = 0;

  uint8_t camera_offset = camera_x - 20;

  vm_write_var(VAR_SCENE_CURSOR_X, (input_cursor_x >> 2) + camera_offset);
  vm_write_var(VAR_SCENE_CURSOR_Y, (input_cursor_y >> 1) - 8);

  if (input_button_pressed != last_input_button_pressed)
  {
    last_input_button_pressed = input_button_pressed;

    if (input_button_pressed == INPUT_BUTTON_LEFT)
    {
      if (input_cursor_y >= 16 && input_cursor_y < 144) {
        // clicked in gfx scene
        vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_SCENE_CLICK);
        script_start(SCRIPT_ID_INPUT_EVENT);
        return;
      }
      else if (input_cursor_y >= 18 * 8 && input_cursor_y < 19 * 8) {
        // clicked on sentence line
        if (ui_state & UI_FLAGS_ENABLE_SENTENCE) {
          vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_SENTENCE_CLICK);
          script_start(SCRIPT_ID_INPUT_EVENT);
          return;
        }
      }
      else if (input_cursor_y >= 19 * 8 && input_cursor_y < 22 * 8) {
        // clicked in verb zone
        if (ui_state & UI_FLAGS_ENABLE_VERBS) {
          MAP_CS_MAIN_PRIV
          uint8_t verb_slot = get_hovered_verb_slot();
          UNMAP_CS
          if (verb_slot != 0xff) {
            select_verb(vm_state.verbs.id[verb_slot]);
          }
          return;
        }
      }
      else if (input_cursor_y >= 22 * 8 && input_cursor_y < 24 * 8) {
        // clicked in inventory zone
        if (ui_state & UI_FLAGS_ENABLE_INVENTORY) {
          uint8_t inventory_ui_slot = get_hovered_inventory_slot();
          if (inventory_ui_slot < 4 && 
              inventory_ui_slot < inv_ui_entries.num_entries) {
            uint8_t inventory_item_id = inv_ui_entries.displayed_ids[inventory_ui_slot];
            if (inventory_item_id < vm_state.inv_num_objects) {
              vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_INVENTORY_CLICK);
              vm_write_var(VAR_CLICKED_NOUN, inv_get_global_object_id(inventory_item_id));
              script_start(SCRIPT_ID_INPUT_EVENT);
              return;
            }
          }
          else if (inventory_ui_slot == 4) {
            inventory_scroll_up();
            return;
          }
          else if (inventory_ui_slot == 5) {
            inventory_scroll_down();
            return;
          }
        }
      }
    }
  }

  // keyboard handling
  if (input_key_pressed) {
    if (input_key_pressed == vm_read_var8(VAR_OVERRIDE_KEY)) {
      override_cutscene();
    }
    else if (input_key_pressed == 0x20) {
      // handle space key, pause
      static const char pause_str[] = "Game paused, press SPACE to continue.";

      input_key_pressed = 0; // ack the space key
      MAP_CS_GFX
      gfx_print_interface_text(0, 18, pause_str, prev_sentence_highlighted ? TEXT_STYLE_HIGHLIGHTED : TEXT_STYLE_SENTENCE);
      script_watchdog = WATCHDOG_TIMEOUT;

      while (1) {
        if (input_key_pressed) {
          if (input_key_pressed == 0x20) {
            wait_for_jiffy();  // this resets the elapsed jiffies timer
            break;
          }
          else {
            // ack all other key presses
            input_key_pressed = 0;
          }
        }
      }
      
      vm_print_sentence();
      UNMAP_CS
    }
    else if (input_key_pressed == 8) {
      // handle restart key, ask user confirmation
      static const char restart_str[] = "Are you sure you want to restart? (y/n)";

      input_key_pressed = 0; // ack the F8 restart key

      if (actor_talking != 0xff) {
        actor_stop_talking(actor_talking);
      }
      MAP_CS_GFX
      gfx_print_dialog(2, restart_str, sizeof(restart_str) - 1);
      script_watchdog = WATCHDOG_TIMEOUT;
      
      while (1) {
        if (input_key_pressed) {
          if (input_key_pressed == 'y') {
            reset_game = RESET_RESTART;
            break;
          }
          else if (input_key_pressed == 'n') {
            wait_for_jiffy();  // this resets the elapsed jiffies timer
            break;
          }
          else {
            input_key_pressed = 0;
          }
        }
      }
      
      gfx_clear_dialog();
      UNMAP_CS
    }
    else {
      vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_KEYPRESS);
      vm_write_var(VAR_CURRENT_KEY, input_key_pressed);
      script_start(SCRIPT_ID_INPUT_EVENT);
    }
    // ack the key press
    input_key_pressed = 0;
  }
}

static uint8_t match_parent_object_state(uint8_t parent, uint8_t expected_state)
{
  SAVE_DS_AUTO_RESTORE
  map_ds_resource(obj_page[parent]);

  __auto_type obj_hdr  = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[parent]);
  uint8_t new_parent   = obj_hdr->parent;
  uint8_t cur_state    = vm_state.global_game_objects[obj_hdr->id] & OBJ_STATE;
  uint8_t parent_state = obj_hdr->pos_y_and_parent_state & 0x80;

  //debug_out("  check parent %d state %d - expected_state %d", obj_hdr->id, cur_state, expected_state);

  if (cur_state != expected_state) {
    return 0;
  }
  if (new_parent == 0) {
    //debug_out("  no further parent, match");
    return 1;
  }

  return match_parent_object_state(new_parent - 1, parent_state);
}

static void update_script_timers(uint8_t elapsed_jiffies)
{
  ++elapsed_jiffies;
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    // checking for equality will also make sure we won't update slots that are frozen
    if (vm_state.proc_state[slot] == PROC_STATE_WAITING_FOR_TIMER)
    {
      vm_state.proc_wait_timer[slot] += elapsed_jiffies;
      //uint8_t timer_msb = (uint8_t)((uintptr_t)(vm_state.proc_wait_timer[slot]) >> 24);
      //if (timer_msb == 0)
      if (vm_state.proc_wait_timer[slot] > 0)
      {
        set_proc_state(slot, PROC_STATE_RUNNING);
      }
    }
  }
}

static const char *get_object_name(uint16_t global_object_id)
{
  if (global_object_id <= NUM_ACTORS) {
    return actors.name[global_object_id];
  }

  uint8_t owner = vm_state.global_game_objects[global_object_id] & 0x0f;
  
  if (owner == 0x0f) {
    // is room object
    struct object_code *obj_hdr = obj_hdr = vm_get_room_object_hdr(global_object_id);
    if (obj_hdr) {
      uint16_t name_offset = obj_hdr->name_offset;
      return ((const char *)obj_hdr) + name_offset;
    }
  }
  else if (owner == vm_read_var8(VAR_SELECTED_ACTOR)) {
    // is inventory object
    uint8_t inv_pos = inv_get_position_by_id(global_object_id);
    if (inv_pos != 0xff) {
      return inv_get_object_name(inv_pos);
    }
  }

  return NULL;
}

static void execute_sentence_stack(void)
{
  if (sentence_stack.num_entries == 0 || vm_is_script_running(SCRIPT_ID_SENTENCE))
  {
    return;
  }

  --sentence_stack.num_entries;
  uint8_t  verb  = sentence_stack.verb[sentence_stack.num_entries];
  uint16_t noun1 = sentence_stack.noun1[sentence_stack.num_entries];
  uint16_t noun2 = sentence_stack.noun2[sentence_stack.num_entries];

  vm_write_var(VAR_CURRENT_VERB, verb);
  vm_write_var(VAR_CURRENT_NOUN1, noun1);
  vm_write_var(VAR_CURRENT_NOUN2, noun2);
  
  uint8_t script_offset   = 0;
  uint8_t local_object_id = vm_get_local_object_id(noun1);
  uint8_t is_inventory;

  if (local_object_id != 0xff) {
    is_inventory = 0;
  }
  else {
    is_inventory = 1;
    local_object_id = inv_get_position_by_id(noun1);
  }

  if (local_object_id != 0xff) {
    script_offset = vm_get_room_object_script_offset(verb, local_object_id, is_inventory);
  }
  
  vm_write_var(VAR_VALID_VERB, script_offset != 0);

  //debug_out(" verb %d, noun1 %d, noun2 %d, valid_verb %d", verb, noun1, noun2, script_offset != 0);

  uint8_t slot = script_start(SCRIPT_ID_SENTENCE);
  //debug_out("Sentence script verb %d noun1 %d noun2 %d valid-verb %d", verb, noun1, noun2, script_offset != 0);
}

static void load_room(uint8_t room_no)
{
  __auto_type room_hdr = (struct room_header *)RES_MAPPED;

  room_res_slot = res_provide(RES_TYPE_ROOM, room_no, 0);
  res_activate_slot(room_res_slot);
  map_ds_resource(room_res_slot);
  room_width = room_hdr->bg_width;
  uint16_t bg_data_offset = room_hdr->bg_data_offset;
  uint16_t bg_masking_offset = room_hdr->bg_attr_offset;

  MAP_CS_GFX
  uint8_t __huge* bg_data = res_get_huge_ptr(room_res_slot) + bg_data_offset;
  gfx_decode_bg_image(bg_data, room_width);
  gfx_decode_masking_buffer(bg_masking_offset, room_width);

  map_ds_resource(room_res_slot);

  read_objects();
  MAP_CS_MAIN_PRIV
  read_walk_boxes();
}

static uint16_t clamp_camera_x(uint16_t x)
{
  uint8_t max_camera_x = room_width / 8 - 20;
  if (x > max_camera_x) {
    return max_camera_x;
  }
  else if (x < 20) {
    return 20;
  }
  return x;
}

static void update_actors(void)
{
  for (uint8_t i = 0; i < MAX_LOCAL_ACTORS; ++i)
  {
    if (local_actors.global_id[i] != 0xff) {
      actor_next_step(i);
    }
  }
}

static void animate_actors(void)
{
  uint8_t redraw_needed = 0;
  for (uint8_t local_id = 0; local_id < MAX_LOCAL_ACTORS; ++local_id)
  {
    if (local_actors.global_id[local_id] != 0xff) {
      actor_update_animation(local_id);
    }
  }
}

static void override_cutscene(void)
{
  if (vm_state.cs_override_pc) {
    vm_state.proc_pc[vm_state.cs_proc_slot] = vm_state.cs_override_pc;
    vm_state.cs_override_pc = 0;
    vm_state.proc_state[vm_state.cs_proc_slot] &= ~PROC_FLAGS_FROZEN;
    if (vm_state.proc_state[vm_state.cs_proc_slot] == PROC_STATE_WAITING_FOR_TIMER) {
      vm_state.proc_state[vm_state.cs_proc_slot] = PROC_STATE_RUNNING;
    }
    vm_write_var(VAR_OVERRIDE_HIT, 1);
  }
}

static void update_sentence_line(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_SENTENCE)) {
    gfx_clear_sentence();
    return;
  }

  gfx_print_interface_text(0, 18, sentence_text, TEXT_STYLE_SENTENCE);

  prev_sentence_highlighted = 0;
}

static void update_sentence_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_CURSOR)) {
    return;
  }

  if (input_cursor_y >= 18 * 8 && input_cursor_y < 19 * 8) {
    if (!prev_sentence_highlighted) {
      MAP_CS_GFX
      gfx_change_interface_text_style(0, 18, 40, TEXT_STYLE_HIGHLIGHTED);
      UNMAP_CS
      prev_sentence_highlighted = 1;
    }
  }
  else {
    if (prev_sentence_highlighted) {
      MAP_CS_GFX
      gfx_change_interface_text_style(0, 18, 40, TEXT_STYLE_SENTENCE);
      UNMAP_CS
      prev_sentence_highlighted = 0;
    }
  }
}

static void add_string_to_sentence(const char *str, uint8_t prepend_space)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_MAIN_PRIV
  add_string_to_sentence_priv(str, prepend_space);
}

static void update_verb_interface(void)
{
  gfx_clear_verbs();
  if (ui_state & UI_FLAGS_ENABLE_VERBS) {
    //debug_out("Updating verbs");
    UNMAP_DS
    for (uint8_t i = 0; i < MAX_VERBS; ++i) {
      if (vm_state.verbs.id[i] != 0xff) {
        gfx_print_interface_text(vm_state.verbs.x[i], vm_state.verbs.y[i], vm_state.verbs.name[i], TEXT_STYLE_NORMAL);
      }
    }
    prev_verb_highlighted = 0xff;
  }
}

static void update_verb_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_VERBS)) {
    return;
  }

  uint8_t cur_verb = 0xff;
  if (input_cursor_y >= 19 * 8 && input_cursor_y < 22 * 8) {
    MAP_CS_MAIN_PRIV
    cur_verb = get_hovered_verb_slot();
  }

  if (cur_verb != prev_verb_highlighted) {
    MAP_CS_GFX
    if (prev_verb_highlighted != 0xff) {
      gfx_change_interface_text_style(vm_state.verbs.x[prev_verb_highlighted], 
                                      vm_state.verbs.y[prev_verb_highlighted], 
                                      vm_state.verbs.len[prev_verb_highlighted], 
                                      TEXT_STYLE_NORMAL);
    }
    if (cur_verb != 0xff) {
      gfx_change_interface_text_style(vm_state.verbs.x[cur_verb], 
                                      vm_state.verbs.y[cur_verb], 
                                      vm_state.verbs.len[cur_verb], 
                                      TEXT_STYLE_HIGHLIGHTED);
    }
    prev_verb_highlighted = cur_verb;
  }

  UNMAP_CS
}

static uint8_t get_verb_slot_by_id(uint8_t verb_id)
{
  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    if (vm_state.verbs.id[i] == verb_id) {
      return i;
    }
  }
  return 0xff;
}

static void select_verb(uint8_t verb_id)
{
  vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_VERB_SELECT);
  vm_write_var(VAR_SELECTED_VERB, verb_id);
  script_start(SCRIPT_ID_INPUT_EVENT);
}

static const char *get_preposition_name(uint8_t preposition)
{
  static const char *prepositions_english[] = { NULL, "in", "with", "on", "to" };
  return prepositions_english[preposition];
}

static void update_inventory_interface()
{
  UNMAP_DS

  struct inventory_display entries;

  gfx_clear_inventory();

  if (ui_state & UI_FLAGS_ENABLE_INVENTORY) {
    inv_update_displayed_inventory();

    char buf[19];
    buf[18] = '\0';
    for (uint8_t ui_pos = 0; ui_pos < inv_ui_entries.num_entries; ++ui_pos) {
      const char *name = inv_get_object_name(inv_ui_entries.displayed_ids[ui_pos]);
      for (uint8_t j = 0; j < 18; ++j) {
        buf[j] = name[j];
        if (buf[j] == '\0') {
          break;
        }
      }
      gfx_print_interface_text(inventory_ui_pos_to_x(ui_pos), 
                               inventory_ui_pos_to_y(ui_pos), 
                               buf, 
                               TEXT_STYLE_INVENTORY);
    }
    
    if (inv_ui_entries.prev_id != 0xff) {
      gfx_print_interface_text(19, 22, "\xfc\xfd", TEXT_STYLE_INVENTORY_ARROW);
    }

    if (inv_ui_entries.next_id != 0xff) {
      gfx_print_interface_text(19, 23, "\xfe\xff", TEXT_STYLE_INVENTORY_ARROW);
    }
  }
}

static void update_inventory_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_INVENTORY)) {
    return;
  }

  uint8_t cur_inventory = get_hovered_inventory_slot();

  if (cur_inventory != prev_inventory_highlighted) {
    MAP_CS_GFX
    if (prev_inventory_highlighted != 0xff) {
      uint8_t style = (prev_inventory_highlighted & 4) ? TEXT_STYLE_INVENTORY_ARROW : TEXT_STYLE_INVENTORY;
      gfx_change_interface_text_style(inventory_ui_pos_to_x(prev_inventory_highlighted), 
                                      inventory_ui_pos_to_y(prev_inventory_highlighted), 
                                      prev_inventory_highlighted & 4 ? 4 : 18, 
                                      style);
    }
    if (cur_inventory != 0xff) {
      gfx_change_interface_text_style(inventory_ui_pos_to_x(cur_inventory), 
                                      inventory_ui_pos_to_y(cur_inventory), 
                                      cur_inventory & 4 ? 4 : 18, 
                                      TEXT_STYLE_HIGHLIGHTED);
    }
    UNMAP_CS
    prev_inventory_highlighted = cur_inventory;
  }
}

/**
  * @brief Returns the inventory slot currently hovered by the cursor
  *
  * @return The inventory slot currently hovered by the cursor
  *
  * The following return codes are possible:
  * - 0xff: Cursor is not hovering over any inventory slot
  * - 0-3: Cursor is hovering over an inventory slot
  *   - 0: Top-left slot
  *   - 1: Top-right slot
  *   - 2: Bottom-left slot
  *   - 3: Bottom-right slot 
  * - 4: Cursor is hovering over the scroll-up button
  * - 5: Cursor is hovering over the scroll-down button
  *
  * Code section: code_main
  */
static uint8_t get_hovered_inventory_slot(void)
{
  uint8_t cur_inventory = 0xff;
  
  if (input_cursor_y >= 22 * 8 && input_cursor_y < 24 * 8) {
    if (input_cursor_x >= 22 * 4) {
      cur_inventory = 1;
    }
    else if (input_cursor_x < 18 * 4) {
      cur_inventory = 0;
    }
    else {
      cur_inventory = 4;
    }
    if (cur_inventory != 0xff && input_cursor_y >= 23 * 8) {
      cur_inventory += cur_inventory < 4 ? 2 : 1;
    }
  }

  return cur_inventory;
}

static uint8_t inventory_ui_pos_to_x(uint8_t pos)
{
  if (pos & 4) {
    // arrow buttons
    return 18;
  }
  // even slots are on the left, odd slots are on the right
  return (pos & 1) ? 22 : 0;
}

static uint8_t inventory_ui_pos_to_y(uint8_t pos)
{
  if (pos & 4) {
    // arrow buttons
    return 22 + (pos & 1);
  }
  // top slots are at 22, bottom slots are at 23
  return 22 + (pos >> 1);
}

static void inventory_scroll_up(void)
{
  if (inv_ui_entries.prev_id != 0xff) {
    inventory_pos -= 2;
    vm_update_inventory();
    prev_inventory_highlighted = 0xff;
  }
}

static void inventory_scroll_down(void)
{
  if (inv_ui_entries.next_id != 0xff) {
    inventory_pos += 2;
    vm_update_inventory();
    prev_inventory_highlighted = 0xff;
  }
}

/// @} // vm_private

#pragma clang section text="code_main_private"
/**
  * @brief Removes all orphan entries from slot table
  *
  * All slots with value 0xff are removed from the table, moving all other entries to the
  * beginning of the table. The number of active slots is updated accordingly.
  *
  * Code section: code_main_private
  */
static void cleanup_slot_table()
{
  uint8_t write_ptr = 0;

  for (uint8_t read_ptr = 0; read_ptr < vm_state.num_active_proc_slots; ++read_ptr) {
    uint8_t slot = vm_state.proc_slot_table[read_ptr];
    if (slot != 0xff) {
      if (vm_state.proc_parent[slot] != 0xff) {
        // in theory, all slots in the table should be without any parent
        VICIV.bordercol = 1;
        fatal_error(ERR_SLOT_WITH_PARENT_IN_SLOT_TABLE);
      }
      if (write_ptr != read_ptr) {
        vm_state.proc_slot_table[write_ptr] = slot;
      }
      ++write_ptr;
    }
  }

  vm_state.num_active_proc_slots = write_ptr;
}

static void read_walk_boxes(void)
{
  __auto_type room_hdr = (struct room_header *)RES_MAPPED;
  __auto_type box_ptr = NEAR_U8_PTR(RES_MAPPED + room_hdr->walk_boxes_offset);
  num_walk_boxes = *box_ptr++;
  walk_boxes = (struct walk_box *)box_ptr;
  //debug_out("Reading %d walk boxes", num_walk_boxes);
  for (uint8_t i = 0; i < num_walk_boxes; ++i) {
    __auto_type box = (struct walk_box *)box_ptr;
    box_ptr += sizeof(struct walk_box);
    //debug_out("Walk box %d:", i);
    //debug_out("  uy:  %d", box->top_y);
    //debug_out("  ly:  %d", box->bottom_y);
    //debug_out("  ulx: %d", box->topleft_x);
    //debug_out("  urx: %d", box->topright_x);
    //debug_out("  llx: %d", box->bottomleft_x);
    //debug_out("  lrx: %d", box->bottomright_x);
    //debug_out("  mask: %x", box->mask);
    //debug_out("  classes: %x", box->classes);
  }
  walk_box_matrix = box_ptr;
  /*
  for (uint8_t i = 0; i < num_walk_boxes; ++i) {
    debug_out("  row offset %d = %d", i, *box_ptr++);
  }
  for (uint8_t src_box = 0; src_box < num_walk_boxes; ++src_box) {
    for (uint8_t dst_box = 0; dst_box < num_walk_boxes; ++dst_box) {
      uint8_t next_box = *box_ptr++;
      debug_out("  box matrix[%d][%d] = %d", src_box, dst_box, next_box);
    }
  }*/
}

/**
  * @brief Clears state of objects matching position and size
  *
  * Clears the state of all objects that match the given position and size. The function will
  * iterate over all objects and check if the object is currently active and if the object
  * position and size matches the one provided. If a match is found, the object state is
  * cleared.
  * 
  * @param local_object_id Object in current room to compare with
  */
static void clear_all_other_object_states(uint8_t local_object_id)
{
  SAVE_DS_AUTO_RESTORE

  map_ds_resource(obj_page[local_object_id]);
  __auto_type obj_hdr = (struct object_code *)(RES_MAPPED + obj_offset[local_object_id]);
  uint8_t width = obj_hdr->width;
  uint8_t height = obj_hdr->height_and_actor_dir >> 3;
  uint8_t x = obj_hdr->pos_x;
  uint8_t y = obj_hdr->pos_y_and_parent_state & 0x7f;
  uint16_t global_object_id = obj_hdr->id;

  for (uint8_t i = 0; i < num_objects; ++i)
  {
    if (obj_id[i] == global_object_id)
    {
      continue;
    }

    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)(RES_MAPPED + obj_offset[i]);
    if (obj_hdr->width == width && 
       (obj_hdr->height_and_actor_dir >> 3) == height &&
        obj_hdr->pos_x == x && 
       (obj_hdr->pos_y_and_parent_state & 0x7f) == y)
    {
      vm_state.global_game_objects[obj_hdr->id] &= ~OBJ_STATE;
      // since actors could potentially be affected, we need to redraw those as well
      vm_update_actors();
      //debug_out("Cleared state of object %d due to identical position and size", obj_hdr->id);
    }
  }
}

static void update_camera(void)
{
  if (camera_state == CAMERA_STATE_FOLLOW_ACTOR && camera_follow_actor_id == 0xff)
  {
    camera_state = 0;
    return;
  }

  uint16_t old_camera_x = camera_x;
  uint16_t max_camera_x = room_width / 8 - 20;

  if (camera_state & CAMERA_STATE_FOLLOW_ACTOR)
  {
    camera_target = actors.x[camera_follow_actor_id];
    //debug_out("Camera following actor %d at x %d, cur camera: %d", camera_follow_actor_id, camera_target, camera_x);
  }
  else if (camera_state & CAMERA_STATE_MOVE_TO_TARGET_POS)
  {
    //debug_out("Camera moving to target %d", camera_target);
  }
  else
  {
    return;
  }

  if (camera_state & CAMERA_STATE_MOVING) {
    if (camera_target > camera_x) {
      //debug_msg("Camera continue moving right");
      if (camera_x < max_camera_x) {
        camera_x += 1;
      }
      else {
        camera_state &= ~CAMERA_STATE_MOVING;
        return;
      }
    }
    else if (camera_target < camera_x) {
      //debug_msg("Camera continue moving left");
      if (camera_x > 20) {
        camera_x -= 1;
      }
      else {
        camera_state &= ~CAMERA_STATE_MOVING;
        return;
      }
    }
    else {
      //debug_msg("Camera reached target");
      camera_state &= ~CAMERA_STATE_MOVING;
      if (camera_state & CAMERA_STATE_MOVE_TO_TARGET_POS) {
        camera_state &= ~CAMERA_STATE_MOVE_TO_TARGET_POS;
      }
      return;
    }
  }
  else {
    if (camera_target < camera_x - 10 && camera_x > 20)
    {
      //debug_msg("Camera start moving left");
      --camera_x;
      camera_state |= CAMERA_STATE_MOVING;
    }
    else if (camera_target > camera_x + 10 && camera_x < max_camera_x)
    {
      //debug_msg("Camera start moving right");
      ++camera_x;
      camera_state |= CAMERA_STATE_MOVING;
    }
    else {
      return;
    }
  }

  if (camera_x != old_camera_x) {
    vm_write_var(VAR_CAMERA_X, camera_x);
    //debug_out("camera_x: %d", camera_x);
    vm_update_bg();
    vm_update_actors();
  }
}

static uint8_t get_hovered_verb_slot(void)
{
  uint8_t row = input_cursor_y >> 3;
  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    uint8_t col = input_cursor_x >> 2;
    if (vm_state.verbs.id[i] != 0xff) {
      if (row == vm_state.verbs.y[i] && col >= vm_state.verbs.x[i] && col < vm_state.verbs.x[i] + vm_state.verbs.len[i]) {
        return i;
      }
    }
  }
  return 0xff;
}

/**
  * @brief Creates a new verb in the verb table
  *
  * Creates a new verb in the verb table at the specified slot. The verb will have the given
  * verb id, position and name. The verb will be added to the verb table and the verb interface
  * will be updated.
  *
  * @param slot The slot in the verb table to create the verb in
  * @param verb_id The verb id of the verb
  * @param x The x position of the verb
  * @param y The y position of the verb
  * @param name The name of the verb
  *
  * Code section: code_main_private
  */
static void verb_new(uint8_t slot, uint8_t verb_id, uint8_t x, uint8_t y, const char* name)
{
  SAVE_DS_AUTO_RESTORE
  UNMAP_DS

  vm_state.verbs.id[slot] = verb_id;
  vm_state.verbs.x[slot]  = x;
  vm_state.verbs.y[slot]  = y;

  uint8_t len = strlen(name);
  vm_state.verbs.len[slot] = len;
  ++len; // include null terminator
  vm_state.verbs.name[slot] = (char *)malloc(len);
  strcpy(vm_state.verbs.name[slot], name);

  vm_update_verbs();
}

/**
  * @brief Deletes a verb from the verb table
  *
  * Deletes the verb at the specified slot from the verb table. The verb will be removed from
  * the verb table and the verb interface will be updated.
  *
  * @param slot The slot in the verb table to delete the verb from
  *
  * Code section: code_main_private
  */
static void verb_delete(uint8_t slot)
{
  SAVE_DS_AUTO_RESTORE

  vm_state.verbs.id[slot] = 0xff;
  UNMAP_DS
  free(vm_state.verbs.name[slot]);
  vm_state.verbs.name[slot] = NULL;

  vm_update_verbs();
}

static void freeze_non_active_scripts(void)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot) {
    if (slot != active_script_slot && vm_state.proc_state[slot] != PROC_STATE_FREE) {
      vm_state.proc_state[slot] |= PROC_FLAGS_FROZEN;
    }
  }
}

static void unfreeze_scripts(void)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot) {
    vm_state.proc_state[slot] &= ~PROC_FLAGS_FROZEN;
  }
}

static void add_string_to_sentence_priv(const char *str, uint8_t prepend_space)
{
  if (!str) {
    return;
  }

  if (prepend_space && sentence_length < 40) {
    sentence_text[sentence_length++] = ' ';
  }
  while (*str != '\0' && sentence_length < 40) {
    if (*str == '@') {
      ++str;
      continue;
    }
    sentence_text[sentence_length++] = *str++;
  }
}

static uint8_t resolve_position(uint16_t object_or_actor_id, uint8_t *x, uint8_t *y)
{
  if (object_or_actor_id < vm_read_var(VAR_NUMBER_OF_ACTORS)) {
    // is an actor
    if (actors.room[object_or_actor_id] == vm_read_var(VAR_SELECTED_ROOM)) {
      *x = actors.x[object_or_actor_id];
      *y = actors.y[object_or_actor_id];
    }
    else {
      *x = 0xff;
    }
    return 1;
  }
  else {
    // is an object
    
    if (inv_object_available(object_or_actor_id)) {
      // is inventory, return position of owner actor if in current room
      uint8_t owner_actor_id = vm_state.global_game_objects[object_or_actor_id] & 0x0f;
      if (actor_is_in_current_room(owner_actor_id)) {
        *x = actors.x[owner_actor_id];
        *y = actors.y[owner_actor_id];
      }
      else {
        *x = 0xff;
      }
      return 0;
    }

    // is room object
    __auto_type obj_hdr = vm_get_room_object_hdr(object_or_actor_id);
    if (!obj_hdr) {
      *x = 0xff;
    } 
    else {
      *x = obj_hdr->walk_to_x;
      *y = (obj_hdr->walk_to_y_and_preposition & 0x1f) << 2;
    }
    return 0;
  }
}

//-----------------------------------------------------------------------------------------------
