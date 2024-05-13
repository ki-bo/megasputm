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

volatile uint8_t script_watchdog;
uint8_t ui_state;
uint16_t camera_x;
uint16_t camera_target;
uint8_t camera_state;
uint8_t camera_follow_actor_id;

uint8_t active_script_slot;
uint16_t message_timer;
uint8_t actor_talking;

uint8_t message_color;
char message_buffer[256];
char *message_ptr;
char *print_message_ptr;
uint8_t print_message_num_chars;

// cutscene backup data
uint8_t cs_room;
int8_t cs_cursor_state;
uint8_t cs_ui_state;
uint8_t cs_proc_slot;
uint16_t cs_override_pc;

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

// Private functions
static void reset_game_state(void);
static void set_proc_state(uint8_t slot, uint8_t state);
static uint8_t get_proc_state(uint8_t slot);
static uint8_t is_room_object_script(uint8_t slot);
static void freeze_non_active_scripts(void);
static void unfreeze_scripts(void);
static void process_dialog(uint8_t jiffies_elapsed);
static void stop_current_actor_talking(void);
static void stop_all_dialog(void);
static uint8_t wait_for_jiffy(void);
static void read_objects(void);
static void read_walk_boxes(void);
static void redraw_screen(void);
static void handle_input(void);
static void clear_all_other_object_states(uint8_t local_object_id);
static uint8_t match_parent_object_state(uint8_t parent, uint8_t expected_state);
static uint8_t find_free_script_slot(void);
static void update_script_timers(uint8_t elapsed_jiffies);
static void execute_script_slot(uint8_t slot);
static const char *get_object_name(uint16_t global_object_id);
static uint8_t start_child_script_at_address(uint8_t res_slot, uint16_t offset);
static void execute_sentence_stack(void);
static uint8_t get_room_object_script_offset(uint8_t verb, uint8_t local_object_id);
static void update_actors(void);
static void animate_actors(void);
static void update_camera(void);
static void override_cutscene(void);
static void update_sentence_line(void);
static void update_sentence_highlighting(void);
static void add_string_to_sentence(const char *str, uint8_t prepend_space);
static void update_verb_interface(void);
static void update_verb_highlighting(void);
static uint8_t get_hovered_verb_slot(void);
static uint8_t get_verb_slot_by_id(uint8_t verb_id);
static void select_verb(uint8_t verb_id);
static const char *get_preposition_name(uint8_t preposition);
static void update_inventory_interface();
static void update_inventory_highlighting(void);
static uint8_t get_hovered_inventory_slot(void);
static uint8_t inventory_pos_to_x(uint8_t pos);
static uint8_t inventory_pos_to_y(uint8_t pos);
static void inventory_scroll_up(void);
static void inventory_scroll_down(void);
static void update_inventory_scroll_buttons(void);

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
  for (active_script_slot = 0; active_script_slot < NUM_SCRIPT_SLOTS; ++active_script_slot)
  {
    vm_state.proc_state[active_script_slot] = PROC_STATE_FREE;
  }

  camera_x = 20;
  camera_state = 0;
  camera_follow_actor_id = 0xff;
  actor_talking = 0xff;
  ui_state = UI_FLAGS_ENABLE_CURSOR | UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS;
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

void vm_restart_game(void)
{
  static const char restart_str[] = "Are you sure you want to restart? (y/n)";
  map_cs_gfx();
  gfx_print_dialog(2, restart_str, sizeof(restart_str) - 1);
  unmap_cs();

  while (1) {
    if (input_key_pressed) {
      debug_out("key %d", input_key_pressed);
      if (input_key_pressed == 'y') {
        reset_game = 1;
        break;
      }
      else if (input_key_pressed == 'n') {
        break;
      }
      else {
        input_key_pressed = 0;
      }
    }
  }
  
  map_cs_gfx();
  gfx_clear_dialog();
  unmap_cs();
}

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

  map_cs_gfx();
  gfx_start();
  unmap_cs();

  reset_game = 1;

  while (1) {
    if (reset_game) {
      reset_game = 0;
      unmap_all();
      map_cs_gfx();
      gfx_fade_out();
      gfx_clear_bg_image();
      gfx_reset_actor_drawing();
      map_cs_diskio();
      reset_game_state();
      unmap_cs();
    }
  
    script_watchdog = 0;

    uint8_t elapsed_jiffies = 0;
    uint8_t jiffy_threshold = vm_read_var(VAR_TIMER_NEXT);
    do {
      elapsed_jiffies += wait_for_jiffy();
    }
    while (jiffy_threshold && elapsed_jiffies < jiffy_threshold);

    //VICIV.bordercol = 0x01;

    map_cs_diskio();
    diskio_check_motor_off(elapsed_jiffies);
    unmap_cs();

    handle_input();

    update_script_timers(elapsed_jiffies);

    for (active_script_slot = 0; 
         active_script_slot < NUM_SCRIPT_SLOTS;
         ++active_script_slot)
    {
      // non-top-level scripts will be executed as childs in other scripts, so skip them here
      if (vm_state.proc_parent[active_script_slot] != 0xff) {
        continue;
      }
      execute_script_slot(active_script_slot);
    }

    // executes the sentence script if any sentences are in the queue
    execute_sentence_stack();

    process_dialog(elapsed_jiffies);
    update_actors();
    animate_actors();
    update_camera();

    if (screen_update_needed) {
      //VICIV.bordercol = 0x01;
      if (screen_update_needed & SCREEN_UPDATE_BG) {
        redraw_screen();
      }
      if (screen_update_needed & SCREEN_UPDATE_ACTORS) {
        actor_sort_and_draw_all();
      }

      //VICIV.bordercol = 0x00;
      map_cs_gfx();
      gfx_wait_vsync();
      //VICIV.bordercol = 0x03;
      if (screen_update_needed & SCREEN_UPDATE_DIALOG) {
        gfx_print_dialog(message_color, print_message_ptr, print_message_num_chars);
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

      unmap_cs();

      screen_update_needed = 0;
    }

    update_sentence_highlighting();
    update_verb_highlighting();
    update_inventory_highlighting();

    //VICIV.bordercol = 0x00;
  }
}

/**
 * @brief Returns the current state of the active process
 * 
 * The returned value is the complete state variable of the currently active process.
 * This includes the state in the lower bits and the additional state flags in the
 * upper bits.
 *
 * @return uint8_t The current state of the active process
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
    if (flags & UI_FLAGS_ENABLE_FREEZE) {
      freeze_non_active_scripts();
    }
    else {
      unfreeze_scripts();
    }
  }

  if (flags & UI_FLAGS_APPLY_CURSOR) {
    ui_state = (ui_state & ~UI_FLAGS_ENABLE_CURSOR) | (flags & UI_FLAGS_ENABLE_CURSOR);
  }

  if (flags & UI_FLAGS_APPLY_INTERFACE) {
    ui_state = (ui_state & ~(UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS)) |
                (flags & (UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS));
    screen_update_needed |= SCREEN_UPDATE_SENTENCE | SCREEN_UPDATE_VERBS | SCREEN_UPDATE_INVENTORY;
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
  uint16_t ds_save = map_get_ds();

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;

  // exit and free old room
  //debug_out("Deactivating old room %d", vm_read_var8(VAR_SELECTED_ROOM));
  stop_all_dialog();
  if (vm_read_var(VAR_SELECTED_ROOM) != 0) {
    map_ds_resource(room_res_slot);
    uint16_t exit_script_offset = room_hdr->exit_script_offset;
    if (exit_script_offset) {
      vm_start_room_script(exit_script_offset);
      execute_script_slot(active_script_slot);
    }

    res_deactivate_slot(room_res_slot);
  }

  map_cs_gfx();
  gfx_clear_dialog();
  gfx_fade_out();

  vm_write_var(VAR_SELECTED_ROOM, room_no);

  camera_follow_actor_id = 0xff;
  camera_state = 0;
  camera_x = 20;

  if (room_no == 0) {
    gfx_clear_bg_image();
    num_objects = 0;
  }
  else {
    //debug_out("Activating new room %d", room_no);
    // activate new room data
    room_res_slot = res_provide(RES_TYPE_ROOM, room_no, 0);
    res_activate_slot(room_res_slot);
    map_ds_resource(room_res_slot);
    room_width = room_hdr->bg_width;
    uint16_t bg_data_offset = room_hdr->bg_data_offset;
    uint16_t bg_masking_offset = room_hdr->bg_attr_offset;

    map_cs_gfx();
    gfx_decode_bg_image(map_ds_room_offset(bg_data_offset), room_width);
    gfx_decode_masking_buffer(bg_masking_offset, room_width);

    map_ds_resource(room_res_slot);

    read_objects();
    map_cs_main_priv();
    read_walk_boxes();
    map_cs_gfx();
    actor_room_changed();

    // run entry script
    uint16_t entry_script_offset = room_hdr->entry_script_offset;
    if (entry_script_offset) {
      vm_start_room_script(entry_script_offset);
      execute_script_slot(active_script_slot);
    }
  }

  redraw_screen();
  screen_update_needed |= SCREEN_UPDATE_BG;
  unmap_cs();

  // restore DS
  map_set_ds(ds_save);
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
  cs_room = vm_read_var8(VAR_SELECTED_ROOM);
  cs_cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  cs_proc_slot = 0xff;
  cs_override_pc = 0;
  cs_ui_state = ui_state;
  vm_change_ui_flags(UI_FLAGS_APPLY_FREEZE | UI_FLAGS_ENABLE_FREEZE |
                     UI_FLAGS_APPLY_CURSOR |
                     UI_FLAGS_APPLY_INTERFACE);
}

void vm_cut_scene_end(void)
{
  //debug_out("cut-scene ended");
  if (vm_read_var8(VAR_SELECTED_ROOM) != cs_room) {
    //debug_out("switching to room %d", cs_room);
    vm_set_current_room(cs_room);
  }
  vm_write_var(VAR_CURSOR_STATE, cs_cursor_state);
  vm_change_ui_flags(cs_ui_state | UI_FLAGS_APPLY_CURSOR | UI_FLAGS_APPLY_FREEZE | UI_FLAGS_APPLY_INTERFACE);
  cs_proc_slot = 0xff;
  cs_override_pc = 0;

  //debug_out("  cursor state %d, ui state %d", vm_read_var8(VAR_CURSOR_STATE), ui_state);
}

void vm_begin_override(void)
{
  cs_proc_slot = active_script_slot;
  cs_override_pc = script_get_current_pc();
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

    message_color = 0x09;
  } else {
    actor_start_talking(actor_id);
    message_color = actors.talk_color[actor_id];
  }

  message_ptr = message_buffer;
  message_timer = 0;
  vm_write_var(VAR_MESSAGE_GOING, 1);
  vm_write_var(VAR_MSGLEN, 0);
}

/**
 * @brief Starts a global script
 * 
 * The script with the given id is added to a free process slot and marked with state
 * PROC_STATE_RUNNING. The script will start execution at the next cycle of the main loop.
 *
 * @param script_id 
 *
 * Code section: code_main
 */
uint8_t vm_start_script(uint8_t script_id)
{
  //debug_out("Starting new top-level script %d", script_id);

  uint8_t slot = vm_get_script_slot_by_script_id(script_id);
  if (slot != 0xff) {
    vm_state.proc_state[slot] = PROC_STATE_RUNNING;
    vm_state.proc_pc[slot] = 4; // skipping script header directly to first opcode
    return slot;
  }

  slot = find_free_script_slot();
  vm_state.proc_script_id[slot] = script_id;
  vm_state.proc_state[slot] = PROC_STATE_RUNNING;
  vm_state.proc_parent[slot] = 0xff;
  vm_state.proc_child[slot] = 0xff;
  vm_state.proc_pc[slot] = 4; // skipping script header directly to first opcode

  uint8_t new_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_activate_slot(new_page);
  proc_res_slot[slot] = new_page;

  return slot;
}

uint8_t vm_start_room_script(uint16_t room_script_offset)
{
  debug_out("Starting room script at offset %04x", room_script_offset);

  uint8_t res_slot = room_res_slot + MSB(room_script_offset);
  uint16_t offset = LSB(room_script_offset);
  return start_child_script_at_address(res_slot, offset);
}

uint8_t vm_start_child_script(uint8_t script_id)
{
  //debug_out("Starting script %d as child of slot %d", script_id, active_script_slot);

  uint8_t slot = vm_start_script(script_id);
  vm_state.proc_parent[slot] = active_script_slot;
  
  // update the calling script to wait for the child script
  vm_state.proc_child[active_script_slot] = slot;
  vm_state.proc_state[active_script_slot] = PROC_STATE_WAITING_FOR_CHILD;

  return slot;
}

uint8_t vm_start_object_script(uint8_t verb, uint16_t global_object_id)
{
  uint8_t id = vm_get_local_object_id(global_object_id);
  if (id == 0xff) {
    return 0xff;
  }

  uint16_t script_offset = get_room_object_script_offset(verb, id);
  if (script_offset == 0) {
    return 0xff;
  }

  uint8_t res_slot = obj_page[id];
  script_offset += obj_offset[id];

  //debug_out("Starting object script %d for verb %d at slot %02x offset %04x", global_object_id, verb, res_slot, script_offset);

  return start_child_script_at_address(res_slot, script_offset);
}

/**
 * @brief Chains a script to the currently active script
 *
 * Chains a script to the currently active script. The currently active script will be
 * stopped and the new script will be executed from start in the same slot. The old script 
 * resource is deactivated and marked free to override if needed.
 *
 * @param script_id The id of the script to execute in stead of the currently active script
 *
 * Code section: code_main
 */
void vm_chain_script(uint8_t script_id)
{
  //debug_out("Chaining script %d from script %d", script_id, vm.proc_script_id[active_script_slot]);

  if (!is_room_object_script(active_script_slot)) {
    res_deactivate_slot(proc_res_slot[active_script_slot]);
  }

  uint8_t new_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_activate_slot(new_page);
  map_ds_resource(new_page);

  proc_res_slot[active_script_slot] = new_page;
  vm_state.proc_pc[active_script_slot] = 4; // skipping script header directly to first opcode
  vm_state.proc_script_id[active_script_slot] = script_id;
}

/**
 * @brief Stops the currently active script
 * 
 * Deactivates the currently active script and sets the process state to PROC_STATE_FREE.
 * The associated script resource is deactivated and thus marked free to override if needed.
 *
 * Code section: code_main
 */
void vm_stop_active_script()
{
  //debug_out("Script %d ended", vm.proc_script_id[active_script_slot]);
  if (!is_room_object_script(active_script_slot)) {
    res_deactivate_slot(proc_res_slot[active_script_slot]);
  }
  vm_state.proc_state[active_script_slot] = PROC_STATE_FREE;
  uint8_t parent = vm_state.proc_parent[active_script_slot];
  if (parent != 0xff)
  {
    vm_state.proc_state[parent] = PROC_STATE_RUNNING;
    vm_state.proc_child[parent] = 0xff;
  }
}

void vm_stop_script(uint8_t script_id)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (vm_state.proc_state[slot] != PROC_STATE_FREE && vm_state.proc_script_id[slot] == script_id)
    {
      //debug_out("Stopping script %d", script_id);
      res_deactivate_slot(proc_res_slot[slot]);
      vm_state.proc_state[slot] = PROC_STATE_FREE;
      // stop all children
      while (vm_state.proc_child[slot] != 0xff)
      {
        vm_state.proc_child[slot] = 0xff;
        slot = vm_state.proc_child[slot];
        //debug_out("Stopped child script %d", vm.proc_script_id[slot]);
        res_deactivate_slot(proc_res_slot[slot]);
        vm_state.proc_state[slot] = PROC_STATE_FREE;
      }
    }
  }
}

uint8_t vm_get_script_slot_by_script_id(uint8_t script_id)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (vm_state.proc_state[slot] != PROC_STATE_FREE && vm_state.proc_script_id[slot] == script_id)
    {
      return slot;
    }
  }

  return 0xff;
}

uint8_t vm_is_script_running(uint8_t script_id)
{
  return vm_get_script_slot_by_script_id(script_id) != 0xff;
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

void vm_update_actors(void)
{
  screen_update_needed |= SCREEN_UPDATE_ACTORS;
}

void vm_update_sentence(void)
{
  screen_update_needed |= SCREEN_UPDATE_SENTENCE;
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
 * @return uint16_t The object id of the object at the given position
 *
 * Code section: code_main
 */
uint16_t vm_get_object_at(uint8_t x, uint8_t y)
{
  // save DS
  uint16_t ds_save = map_get_ds();

  uint16_t found_obj_id = 0;
  y >>= 2;


  for (uint8_t i = 0; i < num_objects; ++i) {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
    //debug_out("Checking object %d at %d, %d state %d - parent_state %d", obj_hdr->id, obj_hdr->pos_x, obj_hdr->pos_y_and_parent_state & 0x7f, global_game_objects[obj_hdr->id] & OBJ_STATE, obj_hdr->pos_y_and_parent_state & 0x80);
    //debug_out("  obj_state %02x", global_game_objects[obj_hdr->id]);
    if (vm_state.global_game_objects[obj_hdr->id] & OBJ_CLASS_UNTOUCHABLE) {
      continue;
    }
    if (obj_hdr->parent != 0) {
      if (!match_parent_object_state(obj_hdr->parent - 1, obj_hdr->pos_y_and_parent_state & 0x80)) {
        continue;
      }
    }

    uint8_t width = obj_hdr->width;
    uint8_t height = obj_hdr->height_and_actor_dir >> 3;
    uint8_t obj_x = obj_hdr->pos_x;
    uint8_t obj_y = obj_hdr->pos_y_and_parent_state & 0x7f;

    if (x >= obj_x && x < obj_x + width && y >= obj_y && y < obj_y + height)
    {
      found_obj_id = obj_hdr->id;
      break;
    }
  }

  // restore DS
  map_set_ds(ds_save);

  return found_obj_id;
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

void vm_draw_object(uint8_t local_id, uint8_t x, uint8_t y)
{
  uint16_t save_ds = map_get_ds();

  clear_all_other_object_states(local_id);

  map_ds_resource(obj_page[local_id]);
  __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[local_id]);
  
  uint8_t width    = obj_hdr->width;
  uint8_t height   = obj_hdr->height_and_actor_dir >> 3;

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

  map_cs_gfx();
  unmap_ds();
  gfx_draw_object(local_id, screen_x, y);
  unmap_cs();

  screen_update_needed |= SCREEN_UPDATE_ACTORS;

  map_set_ds(save_ds);
}

void vm_set_camera_follow_actor(uint8_t actor_id)
{
  uint8_t room_of_actor = actors.room[actor_id];
  if (room_of_actor != vm_read_var8(VAR_SELECTED_ROOM)) {
    vm_set_current_room(room_of_actor);
  }
  camera_follow_actor_id = actor_id;
  camera_state = CAMERA_STATE_FOLLOW_ACTOR;
}

void vm_set_camera_to(uint8_t x)
{
  uint8_t max_camera_x = room_width / 8 - 20;
  if (x > max_camera_x) {
    x = max_camera_x;
  }
  else if (x < 20) {
    x = 20;
  }
  camera_x = x;
}

void vm_camera_pan_to(uint8_t x)
{
  camera_target = x;
  camera_follow_actor_id = 0xff;
  camera_state = CAMERA_STATE_MOVE_TO_TARGET_POS;
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
  uint16_t save_ds = map_get_ds();
  map_ds_heap();

  vm_state.verbs.id[slot] = verb_id;
  vm_state.verbs.x[slot] = x;
  vm_state.verbs.y[slot] = y;

  uint8_t len = strlen(name);
  vm_state.verbs.len[slot] = len;
  ++len; // include null terminator
  vm_state.verbs.name[slot] = (char *)malloc(len);
  strcpy(vm_state.verbs.name[slot], name);

  screen_update_needed |= SCREEN_UPDATE_VERBS;

  map_set_ds(save_ds);
}

void vm_verb_delete(uint8_t slot)
{
  uint16_t save_ds = map_get_ds();

  vm_state.verbs.id[slot] = 0xff;
  map_ds_heap();
  free(vm_state.verbs.name[slot]);
  vm_state.verbs.name[slot] = NULL;

  screen_update_needed |= SCREEN_UPDATE_VERBS;

  map_set_ds(save_ds);
}

void vm_verb_set_state(uint8_t slot, uint8_t state)
{
  vm_state.verbs.state[slot] = state;
}

char *vm_verb_get_name(uint8_t slot)
{
  return vm_state.verbs.name[slot];
}

/// @} // vm_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup vm_private VM Private Functions
 * @{
 */

#pragma clang section text="code_diskio" rodata="cdata_diskio" data="data_diskio" bss="bss_diskio"
static void reset_game_state(void)
{
  diskio_load_game_objects();
  for (uint8_t i = 0; i < NUM_SCRIPT_SLOTS; ++i) {
    vm_state.proc_state[i] = PROC_STATE_FREE;
    vm_state.proc_script_id[i] = 0xff;
    vm_state.proc_parent[i] = 0xff;
    vm_state.proc_child[i] = 0xff;
    vm_state.proc_wait_timer[i] = 0;
  }

  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    vm_state.verbs.id[i] = 0xff;
    map_ds_heap();
    if (vm_state.verbs.name[i]) {
      free(vm_state.verbs.name[i]);
      vm_state.verbs.name[i] = NULL;
    }
    unmap_ds();
  }
  res_deactivate_and_unlock_all();

  uint8_t var_idx = 0;
  do {
    vm_write_var(var_idx, 0);
  }
  while (++var_idx != 0);

  for (uint8_t i = 0; i < NUM_ACTORS; ++i) {
    actors.local_id[i] = 0xff;
    actors.room[i] = 0;
    if (i < MAX_LOCAL_ACTORS) {
      local_actors.global_id[i] = 0xff;
    }
  }

  vm_state.inv_num_objects = 0;
  vm_state.inv_objects[0]  = NULL;
  vm_state.inv_next_free   = (void *)INVENTORY_BASE;

  camera_x = 20;
  camera_state = 0;
  camera_follow_actor_id = 0xff;
  actor_talking = 0xff;
  ui_state = UI_FLAGS_ENABLE_CURSOR | UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS;
  vm_write_var(VAR_CURSOR_STATE, 3);
  vm_state.message_speed = 6;
  message_timer = 0;
  message_ptr = NULL;
  print_message_ptr = NULL;
  prev_verb_highlighted = 0xff;
  prev_inventory_highlighted = 0xff;

  vm_start_script(1);

  wait_for_jiffy(); // this resets the elapsed jiffies timer
}

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"
static void set_proc_state(uint8_t slot, uint8_t state)
{
  vm_state.proc_state[slot] &= ~0x07; // clear state without changing flags
  vm_state.proc_state[slot] |= state;
}

static uint8_t get_proc_state(uint8_t slot)
{
  return vm_state.proc_state[slot] & 0x07;
}

static uint8_t is_room_object_script(uint8_t slot)
{
  return vm_state.proc_script_id[slot] == 0xff;
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

/**
 * @brief Processes the dialog timer
 *
 * Decreases the dialog timer by the number of jiffies elapsed since the last call. Clears
 * the dialog area on screen and resets the dialog active variable when the timer reaches
 * zero. Note that the dialog timer is counting down.
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
      if (c == 0x03) {
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
    screen_update_needed |= SCREEN_UPDATE_DIALOG;
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

  if (message_timer >= jiffies_elapsed)
  {
    message_timer -= jiffies_elapsed;
  }
  else
  {
    message_timer = 0;
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
  map_cs_gfx();
  gfx_clear_dialog();
  unmap_cs();
  vm_write_var(VAR_MESSAGE_GOING, 0);
  vm_write_var(VAR_MSGLEN, 0);
}

static void stop_all_dialog(void)
{
  map_cs_gfx();
  gfx_clear_dialog();
  unmap_cs();
  message_ptr = NULL;
  message_timer = 0;
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
 * @return uint8_t The number of jiffies elapsed since the last call
 *
 * Code section: code_main
 */
static uint8_t wait_for_jiffy(void)
{
  uint16_t save_cs = map_get_cs();
  map_cs_gfx();
  uint8_t num_jiffies_elapsed = gfx_wait_for_jiffy_timer();
  map_set_cs(save_cs);
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

#pragma clang section text="code_main_private"
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
    // debug_out("Walk box %d:", i);
    // debug_out("  uy:  %d", box->top_y);
    // debug_out("  ly:  %d", box->bottom_y);
    // debug_out("  ulx: %d", box->topleft_x);
    // debug_out("  urx: %d", box->topright_x);
    // debug_out("  llx: %d", box->bottomleft_x);
    // debug_out("  lrx: %d", box->bottomright_x);
    // debug_out("  mask: %02x", box->mask);
    // debug_out("  flags: %02x", box->flags);
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
 * @brief Redraws the screen
 *
 * Redraws the screen by redrawing the background and all objects currently visible on screen.
 * The function will read the object data from the object data arrays and draw the objects
 * on screen at their current position.
 *
 * Code section: code_main
 */
#pragma clang section text="code_main"
static void redraw_screen(void)
{
  uint32_t map_save = map_get();
  map_cs_gfx();

  // draw background
  gfx_draw_bg();

  // draw all visible room objects
  for (int8_t i = num_objects - 1; i >= 0; --i)
  {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
    // OBJ_STATE is used to determine visibility of object in this room
    if (!(vm_state.global_game_objects[obj_hdr->id] & OBJ_STATE)) {
      continue;
    }
    int8_t screen_x = obj_hdr->pos_x - camera_x + 20;
    if (screen_x >= 40 || screen_x + obj_hdr->width <= 0) {
      continue;
    }
    int8_t screen_y = obj_hdr->pos_y_and_parent_state & 0x7f;
    unmap_ds();

    gfx_draw_object(i, screen_x, screen_y);
  }

  map_set(map_save);
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
        vm_start_script(SCRIPT_ID_INPUT_EVENT);
      }
      else if (input_cursor_y >= 18 * 8 && input_cursor_y < 19 * 8) {
        // clicked on sentence line
        vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_SENTENCE_CLICK);
        vm_start_script(SCRIPT_ID_INPUT_EVENT);
      }
      else if (input_cursor_y >= 19 * 8 && input_cursor_y < 22 * 8) {
        // clicked in verb zone
        uint8_t verb_slot = get_hovered_verb_slot();
        if (verb_slot != 0xff) {
          select_verb(vm_state.verbs.id[verb_slot]);
        }
      }
      else if (input_cursor_y >= 22 * 8 && input_cursor_y < 24 * 8) {
        // clicked in inventory zone
        uint8_t inventory_ui_slot = get_hovered_inventory_slot();
        if (inventory_ui_slot < 4) {
          uint8_t inventory_item_id = inventory_pos + inventory_ui_slot;
          if (inventory_item_id < vm_state.inv_num_objects) {
            vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_INVENTORY_CLICK);
            vm_write_var(VAR_CLICKED_NOUN, inv_get_object_id(inventory_item_id));
            vm_start_script(SCRIPT_ID_INPUT_EVENT);
            screen_update_needed |= SCREEN_UPDATE_SENTENCE;
          }
        }
        else if (inventory_ui_slot == 4) {
          inventory_scroll_up();
        }
        else if (inventory_ui_slot == 5) {
          inventory_scroll_down();
        }
      }
    }
  }

  // keyboard handling
  if (input_key_pressed) {
    if (input_key_pressed == vm_read_var8(VAR_CUTSCENEEXIT_KEY)) {
      override_cutscene();
    }
    else {
      vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_KEYPRESS);
      vm_write_var(VAR_CURRENT_KEY, input_key_pressed);
      vm_start_script(SCRIPT_ID_INPUT_EVENT);
    }
    // ack the key press
    input_key_pressed = 0;
  }
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
  uint16_t ds_save = map_get_ds();

  map_ds_resource(obj_page[local_object_id]);
  __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[local_object_id]);
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
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
    if (obj_hdr->width == width && 
       (obj_hdr->height_and_actor_dir >> 3) == height &&
        obj_hdr->pos_x == x && 
       (obj_hdr->pos_y_and_parent_state & 0x7f) == y)
    {
      vm_state.global_game_objects[obj_hdr->id] &= ~OBJ_STATE;
      // since actors could potentially be affected, we need to redraw those as well
      screen_update_needed |= SCREEN_UPDATE_ACTORS;
      //debug_out("Cleared state of object %d due to identical position and size", obj_hdr->id);
    }
  }

  map_set_ds(ds_save);
}

static uint8_t match_parent_object_state(uint8_t parent, uint8_t expected_state)
{
  uint16_t ds_save = map_get_ds();
  map_ds_resource(obj_page[parent]);

  __auto_type obj_hdr  = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[parent]);
  uint8_t new_parent   = obj_hdr->parent;
  uint8_t cur_state    = vm_state.global_game_objects[obj_hdr->id] & OBJ_STATE;
  uint8_t parent_state = obj_hdr->pos_y_and_parent_state & 0x80;

  map_set_ds(ds_save);

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
/**
 * @brief Finds a free script slot
 *
 * Finds a free script slot by iterating over all script slots and returning the first free slot
 * found. If no free slot is found, the function will trigger a fatal error.
 *
 * @return uint8_t The index of the first free script slot
 *
 * Code section: code_main
 */
static uint8_t find_free_script_slot(void)
{
  uint8_t slot;
  for (slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (vm_state.proc_state[slot] == PROC_STATE_FREE)
    {
      return slot;
    }
  }

  fatal_error(ERR_OUT_OF_SCRIPT_SLOTS);
}
#pragma clang diagnostic pop

static void update_script_timers(uint8_t elapsed_jiffies)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (get_proc_state(slot) == PROC_STATE_WAITING_FOR_TIMER)
    {
      vm_state.proc_wait_timer[slot] += elapsed_jiffies;
      uint8_t timer_msb = (uint8_t)((uintptr_t)(vm_state.proc_wait_timer[slot]) >> 24);
      if (timer_msb == 0)
      {
        set_proc_state(slot, PROC_STATE_RUNNING);
      }
    }
  }
}

static void execute_script_slot(uint8_t slot)
{
  uint32_t map_save = map_get();

  // walk down to last child
  //uint8_t parent = vm.proc_parent[slot];
  while (get_proc_state(slot) == PROC_STATE_WAITING_FOR_CHILD)
  {
    //parent = slot;
    slot = vm_state.proc_child[slot];
  }

  uint8_t save_active_script_slot = active_script_slot;

  unmap_cs();

  // checking for PROC_STATE_RUNNING also implies frozen states won't match and
  // don't get executed
  while (slot != 0xff && get_proc_state(slot) == PROC_STATE_RUNNING) {
    active_script_slot = slot;
    if (parallel_script_count == 0) {
      // top-level scripts don't need to save state on stack
      script_run_active_slot();
    }
    else {
      // never execute the script that called us recursively
      // (remember we can be called directly from within a script)
      if (active_script_slot == save_active_script_slot) {
        break;
      }

      // save state on stack for nested scripts
      script_run_slot_stacked(active_script_slot);
    }
    if (get_proc_state(slot) == PROC_STATE_WAITING_FOR_CHILD) {
      // script has just started a new child script
      //debug_out("Script in slot %d has started new child in slot %d", slot, vm.proc_child[slot]);
      slot = vm_state.proc_child[slot];
      continue;
    }
    else if (vm_state.proc_state[slot] == PROC_STATE_FREE) {
      // script has exited, launch parent script if needed
      //debug_out("Script in slot %d has exited, moving on to parent slot %d", slot, vm.proc_parent[slot]);
      slot = vm_state.proc_parent[slot];
      continue;
    }
    // in all other cases, the script will resume in the next cycle
    // no further parent or child scripts will be executed
    break;
  }

  active_script_slot = save_active_script_slot;

  map_set(map_save);
}

static const char *get_object_name(uint16_t global_object_id)
{
  struct object_code *obj_hdr = inv_get_object_by_id(global_object_id);
  if (!obj_hdr) {
    obj_hdr = vm_get_room_object_hdr(global_object_id);
    if (!obj_hdr) {
      return NULL;
    }
  }

  uint16_t name_offset = obj_hdr->name_offset;
  return ((const char *)obj_hdr) + name_offset;
}

static uint8_t start_child_script_at_address(uint8_t res_slot, uint16_t offset)
{
  uint8_t slot = find_free_script_slot();
  vm_state.proc_script_id[slot] = 0xff; // room scripts have no id
  vm_state.proc_state[slot] = PROC_STATE_RUNNING;
  vm_state.proc_parent[slot] = active_script_slot;
  vm_state.proc_child[slot] = 0xff;
  proc_res_slot[slot] = res_slot;
  vm_state.proc_pc[slot] = offset;
  
  // update the calling script to wait for the child script
  vm_state.proc_child[active_script_slot] = slot;
  set_proc_state(active_script_slot, PROC_STATE_WAITING_FOR_CHILD);

  return slot;
}

static void execute_sentence_stack(void)
{
  if (sentence_stack.num_entries == 0 || vm_is_script_running(SCRIPT_ID_SENTENCE))
  {
    return;
  }

  --sentence_stack.num_entries;
  uint8_t verb = sentence_stack.verb[sentence_stack.num_entries];
  uint16_t noun1 = sentence_stack.noun1[sentence_stack.num_entries];
  uint16_t noun2 = sentence_stack.noun2[sentence_stack.num_entries];

  vm_write_var(VAR_CURRENT_VERB, verb);
  vm_write_var(VAR_CURRENT_NOUN1, noun1);
  vm_write_var(VAR_CURRENT_NOUN2, noun2);
  
  uint8_t script_offset = 0;
  uint8_t local_object_id = vm_get_local_object_id(noun1);
  if (local_object_id != 0xff) {
    script_offset = get_room_object_script_offset(verb, local_object_id);
  }
  vm_write_var(VAR_VALID_VERB, script_offset != 0);

  uint8_t slot = vm_start_script(SCRIPT_ID_SENTENCE);
  //debug_out("Sentence script verb %d noun1 %d noun2 %d valid-verb %d", verb, noun1, noun2, script_offset != 0);
  execute_script_slot(slot);
}

static uint8_t get_room_object_script_offset(uint8_t verb, uint8_t local_object_id)
{
  uint16_t save_ds = map_get_ds();

  map_ds_resource(obj_page[local_object_id]);
  uint16_t cur_offset = obj_offset[local_object_id];
  cur_offset += 15;
  uint8_t *ptr = NEAR_U8_PTR(RES_MAPPED + cur_offset);
  uint8_t script_offset = 0;

  //debug_out("Searching for verb %02x in object %d", verb, local_object_id);

  while (*ptr != 0) {
    //debug_out("Verb %02x, offset %02x", *ptr, *(ptr + 1));
    if (*ptr == verb || *ptr == 0xff) {
      script_offset = *(ptr + 1);
      break;
    }
    ptr += 2;
  }

  map_set_ds(save_ds);

  return script_offset;
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

static void update_camera(void)
{
  if (camera_state == CAMERA_STATE_FOLLOW_ACTOR && camera_follow_actor_id == 0xff)
  {
    camera_state = 0;
    return;
  }

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
      debug_msg("Camera reached target");
      camera_state &= ~CAMERA_STATE_MOVING;
      if (camera_state & CAMERA_STATE_MOVE_TO_TARGET_POS) {
        camera_state &= ~CAMERA_STATE_MOVE_TO_TARGET_POS;
      }
      return;
    }
  }
  else {
    if (camera_target <= camera_x - 10 && camera_x > 20)
    {
      debug_msg("Camera start moving left");
      --camera_x;
      camera_state |= CAMERA_STATE_MOVING;
    }
    else if (camera_target >= camera_x + 10 && camera_x < max_camera_x)
    {
      debug_msg("Camera start moving right");
      ++camera_x;
      camera_state |= CAMERA_STATE_MOVING;
    }
    else {
      return;
    }
  }

  vm_write_var(VAR_CAMERA_X, camera_x);
  //debug_out("camera_x: %d", camera_x);

  vm_update_bg();
  vm_update_actors();
}

static void override_cutscene(void)
{
  if (cs_override_pc) {
    vm_state.proc_pc[cs_proc_slot] = cs_override_pc;
    cs_override_pc = 0;
    vm_state.proc_state[cs_proc_slot] &= ~PROC_FLAGS_FROZEN;
    if (vm_state.proc_state[cs_proc_slot] == PROC_STATE_WAITING_FOR_TIMER) {
      vm_state.proc_state[cs_proc_slot] = PROC_STATE_RUNNING;
    }
  }
}

static void update_sentence_line(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_SENTENCE)) {
    map_cs_gfx();
    gfx_clear_sentence();
    unmap_cs();
    return;
  }

  uint16_t save_ds = map_get_ds();

  sentence_length = 0;

  //debug_out("Printing sentence");
  uint8_t verb_slot = get_verb_slot_by_id(vm_read_var8(VAR_SENTENCE_VERB));
  //debug_out("  Verb id %d slot %d", vm_read_var8(VAR_SENTENCE_VERB), verb_slot);
  if (verb_slot != 0xff) {
    map_ds_heap();
    add_string_to_sentence(vm_state.verbs.name[verb_slot], 0);
  }

  uint16_t noun1 = vm_read_var(VAR_SENTENCE_NOUN1);
  if (noun1) {
    const char *noun1_name = get_object_name(noun1);
    if (noun1_name) {
      //debug_out("  Noun1 name: %s", noun1_name);
    }
    add_string_to_sentence(noun1_name, 1);
  }
  
  uint8_t preposition = vm_read_var8(VAR_SENTENCE_PREPOSITION);
  if (preposition) {
    const char *preposition_name = get_preposition_name(preposition);
    if (preposition_name) {
      //debug_out("  Preposition name: %s", preposition_name);
    }
    add_string_to_sentence(preposition_name, 1);
  }

  uint16_t noun2 = vm_read_var(VAR_SENTENCE_NOUN2);
  if (noun2) {
    const char *noun2_name = get_object_name(noun2);
    if (noun2_name) {
      //debug_out("  Noun2 name: %s", noun2_name);
    }
    add_string_to_sentence(noun2_name, 1);
  }

  uint8_t num_chars = sentence_length;
  while (num_chars < 40) {
    sentence_text[num_chars] = '@';
    ++num_chars;
  }  
  sentence_text[40] = '\0';

  map_cs_gfx();
  gfx_print_interface_text(0, 18, sentence_text, TEXT_STYLE_SENTENCE);
  unmap_cs();

  prev_sentence_highlighted = 0;

  map_set_ds(save_ds);
}

static void update_sentence_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_SENTENCE)) {
    return;
  }

  if (input_cursor_y >= 18 * 8 && input_cursor_y < 19 * 8) {
    if (!prev_sentence_highlighted) {
      map_cs_gfx();
      gfx_change_interface_text_style(0, 18, 40, TEXT_STYLE_HIGHLIGHTED);
      unmap_cs();
      prev_sentence_highlighted = 1;
    }
  }
  else {
    if (prev_sentence_highlighted) {
      map_cs_gfx();
      gfx_change_interface_text_style(0, 18, 40, TEXT_STYLE_SENTENCE);
      unmap_cs();
      prev_sentence_highlighted = 0;
    }
  }
}

static void add_string_to_sentence(const char *str, uint8_t prepend_space)
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

static void update_verb_interface(void)
{
  gfx_clear_verbs();
  if (ui_state & UI_FLAGS_ENABLE_VERBS) {
    //debug_out("Updating verbs");
    map_ds_heap();
    for (uint8_t i = 0; i < MAX_VERBS; ++i) {
      if (vm_state.verbs.id[i] != 0xff) {
        gfx_print_interface_text(vm_state.verbs.x[i], vm_state.verbs.y[i], vm_state.verbs.name[i], TEXT_STYLE_NORMAL);
      }
    }
    prev_verb_highlighted = 0xff;
    unmap_ds();
  }
}

static void update_verb_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_VERBS)) {
    return;
  }

  uint8_t cur_verb = 0xff;
  if (input_cursor_y >= 19 * 8 && input_cursor_y < 22 * 8) {
    cur_verb = get_hovered_verb_slot();
  }

  if (cur_verb != prev_verb_highlighted) {
    map_cs_gfx();
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
    unmap_cs();
    prev_verb_highlighted = cur_verb;
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
  vm_start_script(SCRIPT_ID_INPUT_EVENT);
  screen_update_needed |= SCREEN_UPDATE_SENTENCE;
}

static const char *get_preposition_name(uint8_t preposition)
{
  static const char *prepositions_english[] = { NULL, "in", "with", "on", "to" };
  return prepositions_english[preposition];
}

static void update_inventory_interface()
{
  unmap_ds();
  map_cs_gfx();
  gfx_clear_inventory();
  if (ui_state & UI_FLAGS_ENABLE_INVENTORY) {
    uint8_t end_id = min(vm_state.inv_num_objects, inventory_pos + 4);
    uint8_t x = 0;
    uint8_t y = 22;
    char buf[19];
    buf[18] = '\0';
    for (uint8_t i = inventory_pos; i < end_id; ++i) {
      const char *name = inv_get_object_name(i);
      for (uint8_t j = 0; j < 18; ++j) {
        buf[j] = name[j];
        if (buf[j] == '\0') {
          break;
        }
      }
      gfx_print_interface_text(x, y, buf, TEXT_STYLE_INVENTORY);
      x = (i & 1) ? 22 : 0;
      if (i == 2) {
        ++y;
      }
    }

    update_inventory_scroll_buttons();
  }
  unmap_cs();
}

static void update_inventory_highlighting(void)
{
  if (!(ui_state & UI_FLAGS_ENABLE_INVENTORY)) {
    return;
  }

  uint8_t cur_inventory = get_hovered_inventory_slot();

  if (cur_inventory != prev_inventory_highlighted) {
    map_cs_gfx();
    if (prev_inventory_highlighted != 0xff) {
      uint8_t style = (prev_inventory_highlighted & 4) ? TEXT_STYLE_INVENTORY_ARROW : TEXT_STYLE_INVENTORY;
      gfx_change_interface_text_style(inventory_pos_to_x(prev_inventory_highlighted), 
                                      inventory_pos_to_y(prev_inventory_highlighted), 
                                      prev_inventory_highlighted & 4 ? 4 : 18, 
                                      style);
    }
    if (cur_inventory != 0xff) {
      gfx_change_interface_text_style(inventory_pos_to_x(cur_inventory), 
                                      inventory_pos_to_y(cur_inventory), 
                                      cur_inventory & 4 ? 4 : 18, 
                                      TEXT_STYLE_HIGHLIGHTED);
    }
    unmap_cs();
    prev_inventory_highlighted = cur_inventory;
  }
}

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

static uint8_t inventory_pos_to_x(uint8_t pos)
{
  if (pos & 4) {
    return 18;
  }
  return (pos & 1) ? 22 : 0;
}

static uint8_t inventory_pos_to_y(uint8_t pos)
{
  return 22 + (pos >> 1);
}

static void inventory_scroll_up(void)
{
  if (inventory_pos) {
    inventory_pos -= 2;
    screen_update_needed |= SCREEN_UPDATE_INVENTORY;
  }
}

static void inventory_scroll_down(void)
{
  if (inventory_pos + 4 < vm_state.inv_num_objects) {
    inventory_pos += 2;
    screen_update_needed |= SCREEN_UPDATE_INVENTORY;
  }
}

static void update_inventory_scroll_buttons(void)
{
  if (inventory_pos && inventory_pos + 4 < vm_state.inv_num_objects) {
    gfx_print_interface_text(19, 22, "\xfc\xfd", TEXT_STYLE_INVENTORY_ARROW);
  }

  if (inventory_pos + 4 < vm_state.inv_num_objects) {
    gfx_print_interface_text(19, 23, "\xfe\xff", TEXT_STYLE_INVENTORY_ARROW);
  }
}

/// @} // vm_private

//-----------------------------------------------------------------------------------------------
