#include "vm.h"
#include "actor.h"
#include "costume.h"
#include "diskio.h"
#include "error.h"
#include "gfx.h"
#include "input.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "script.h"
#include "util.h"
#include <stdint.h>
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
  uint8_t  walk_to_y;
  uint8_t  height_and_actor_dir;
  uint8_t  name_offset;
};

struct verb {
  uint8_t id[MAX_VERBS];
  uint8_t state[MAX_VERBS];
  char    name[MAX_VERBS][9];
};

#pragma clang section bss="zdata"

uint8_t global_game_objects[780];
uint8_t variables_lo[256];
uint8_t variables_hi[256];

uint8_t message_color;
char message_buffer[256];
char *message_ptr;
char *print_message_ptr;
uint8_t print_message_num_chars;

volatile uint8_t script_watchdog;
uint8_t ui_state;
uint16_t camera_x;
uint16_t camera_target;
uint8_t camera_state;
uint8_t camera_follow_actor_id;

uint8_t active_script_slot;
uint8_t message_speed = 6;
uint16_t message_timer;
uint8_t actor_talking;

uint8_t proc_script_id[NUM_SCRIPT_SLOTS];
uint8_t proc_state[NUM_SCRIPT_SLOTS];
uint8_t proc_parent[NUM_SCRIPT_SLOTS];
uint8_t proc_child[NUM_SCRIPT_SLOTS];
uint8_t proc_type[NUM_SCRIPT_SLOTS];
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint16_t proc_pc[NUM_SCRIPT_SLOTS];
int32_t proc_wait_timer[NUM_SCRIPT_SLOTS];

// cutscene backup data
uint8_t cs_room;
int8_t cs_cursor_state;
uint8_t cs_ui_state;
uint8_t cs_proc_slot;
uint16_t cs_override_pc;

// room and object data
uint8_t          room_res_slot;
uint16_t         room_width;
uint8_t          num_objects;
uint8_t          obj_page[MAX_OBJECTS];
uint8_t          obj_offset[MAX_OBJECTS];
uint16_t         obj_id[MAX_OBJECTS];
uint8_t          screen_update_needed = 0;
uint8_t          num_walk_boxes;
struct walk_box *walk_boxes;
uint8_t         *walk_box_matrix;

// verb data
struct verb verbs;

// command queue
struct cmd_stack_t cmd_stack;

// Private functions
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
static void redraw_actors(void);
static void handle_input(void);
static uint8_t match_parent_object_state(uint8_t parent, uint8_t state);
static uint8_t find_free_script_slot(void);
static void update_script_timers(uint8_t elapsed_jiffies);
static void execute_script_slot(uint8_t slot);
static uint8_t get_local_object_id(uint16_t global_object_id);
static uint8_t start_child_script_at_address(uint8_t res_slot, uint16_t offset);
static void execute_command_stack(void);
static uint8_t get_room_object_script_offset(uint8_t verb, uint8_t local_object_id);
static void update_actors(void);
static void animate_actors(void);
static void update_camera(void);
static void override_cutscene(void);

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
    proc_state[active_script_slot] = PROC_STATE_FREE;
  }

  camera_x = 20;
  camera_state = 0;
  camera_follow_actor_id = 0xff;
  actor_talking = 0xff;
  ui_state = UI_FLAGS_ENABLE_CURSOR | UI_FLAGS_ENABLE_INVENTORY | UI_FLAGS_ENABLE_SENTENCE | UI_FLAGS_ENABLE_VERBS;
  vm_write_var(VAR_CURSOR_STATE, 3);

  for (uint8_t i = 0; i < MAX_VERBS; ++i) {
    verbs.id[i] = 0xff;
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

  map_cs_gfx();
  gfx_start();
  unmap_cs();

  vm_start_script(1);

  wait_for_jiffy(); // this resets the elapsed jiffies timer
  message_timer = 0;

  while (1) {
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
      if (proc_parent[active_script_slot] != 0xff) {
        continue;
      }
      execute_script_slot(active_script_slot);
    }

    // executes the command script if any commands are in the queue
    execute_command_stack();

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
        redraw_actors();
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
      unmap_cs();

      screen_update_needed = 0;
    }

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
  return proc_state[active_script_slot];
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
  }

  debug_out("UI state enable-cursor: %d", ui_state & UI_FLAGS_ENABLE_CURSOR);
  debug_out("cursor state: %d", vm_read_var8(VAR_CURSOR_STATE));
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
  debug_out("Deactivating old room %d", vm_read_var8(VAR_SELECTED_ROOM));
  stop_all_dialog();
  if (vm_read_var(VAR_SELECTED_ROOM) != 0) {
    map_ds_resource(room_res_slot);
    uint16_t exit_script_offset = room_hdr->exit_script_offset;
    if (exit_script_offset) {
      vm_start_room_script(exit_script_offset + 4);
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
    debug_out("Activating new room %d", room_no);
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
    read_walk_boxes();
    actor_room_changed();

    // run entry script
    uint16_t entry_script_offset = room_hdr->entry_script_offset;
    if (entry_script_offset) {
      vm_start_room_script(entry_script_offset + 4);
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
  proc_state[active_script_slot] = PROC_STATE_WAITING_FOR_TIMER;
  proc_wait_timer[active_script_slot] = negative_ticks;
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
  debug_out("cut-scene ended");
  if (vm_read_var8(VAR_SELECTED_ROOM) != cs_room) {
    debug_out("switching to room %d", cs_room);
    vm_set_current_room(cs_room);
  }
  vm_write_var(VAR_CURSOR_STATE, cs_cursor_state);
  vm_change_ui_flags(cs_ui_state | UI_FLAGS_APPLY_CURSOR | UI_FLAGS_APPLY_FREEZE | UI_FLAGS_APPLY_INTERFACE);
  cs_proc_slot = 0xff;
  cs_override_pc = 0;

  debug_out("  cursor state %d, ui state %d", vm_read_var8(VAR_CURSOR_STATE), ui_state);
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
  debug_out("Starting new top-level script %d", script_id);

  uint8_t slot = find_free_script_slot();
  proc_script_id[slot] = script_id;
  proc_state[slot] = PROC_STATE_RUNNING;
  proc_parent[slot] = 0xff;
  proc_child[slot] = 0xff;
  proc_pc[slot] = 4; // skipping script header directly to first opcode

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
  debug_out("Starting script %d as child of slot %d", script_id, active_script_slot);

  uint8_t slot = vm_start_script(script_id);
  proc_parent[slot] = active_script_slot;
  
  // update the calling script to wait for the child script
  proc_child[active_script_slot] = slot;
  proc_state[active_script_slot] = PROC_STATE_WAITING_FOR_CHILD;

  return slot;
}

uint8_t vm_start_object_script(uint8_t verb, uint16_t global_object_id)
{
  uint8_t id = get_local_object_id(global_object_id);
  if (id == 0xff) {
    return 0xff;
  }

  uint16_t script_offset = get_room_object_script_offset(verb, id);
  if (script_offset == 0) {
    return 0xff;
  }

  uint8_t res_slot = obj_page[id];
  script_offset += obj_offset[id];

  debug_out("Starting object script %d for verb %d at slot %02x offset %04x", global_object_id, verb, res_slot, script_offset);

  return start_child_script_at_address(res_slot, script_offset);
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
  debug_out("Script %d ended", proc_script_id[active_script_slot]);
  if (!is_room_object_script(active_script_slot)) {
    res_deactivate_slot(proc_res_slot[active_script_slot]);
  }
  proc_state[active_script_slot] = PROC_STATE_FREE;
  uint8_t parent = proc_parent[active_script_slot];
  if (parent != 0xff)
  {
    proc_state[parent] = PROC_STATE_RUNNING;
    proc_child[parent] = 0xff;
  }
}

void vm_stop_script(uint8_t script_id)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (proc_state[slot] != PROC_STATE_FREE && proc_script_id[slot] == script_id)
    {
      debug_out("Stopping script %d", script_id);
      res_deactivate_slot(proc_res_slot[slot]);
      proc_state[slot] = PROC_STATE_FREE;
      // stop all children
      while (proc_child[slot] != 0xff)
      {
        proc_child[slot] = 0xff;
        slot = proc_child[slot];
        debug_out("Stopped child script %d", proc_script_id[slot]);
        res_deactivate_slot(proc_res_slot[slot]);
        proc_state[slot] = PROC_STATE_FREE;
      }
    }
  }
}

uint8_t vm_is_script_running(uint8_t script_id)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (proc_state[slot] != PROC_STATE_FREE && proc_script_id[slot] == script_id)
    {
      return 1;
    }
  }

  return 0;
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

uint16_t vm_get_object_at(uint8_t x, uint8_t y)
{
  // save DS
  uint16_t ds_save = map_get_ds();

  uint16_t found_obj_id = 0;
  y >>= 2;

  for (uint8_t i = 0; i < num_objects; ++i) {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
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

/**
 * @brief Clears state of objects matching position and size
 *
 * Clears the state of all objects that match the given position and size. The function will
 * iterate over all objects and check if the object is currently active and if the object
 * position and size matches the given parameters. If a match is found, the object state is
 * cleared.
 * 
 * @param global_object_id Object to compare with
 */
void vm_clear_all_other_object_states(uint16_t global_object_id)
{
  uint8_t local_object_id = get_local_object_id(global_object_id);
  if (local_object_id == 0xff)
  {
    return;
  }

  uint16_t ds_save = map_get_ds();

  map_ds_resource(obj_page[local_object_id]);
  __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[local_object_id]);
  uint8_t width = obj_hdr->width;
  uint8_t height = obj_hdr->height_and_actor_dir >> 3;
  uint8_t x = obj_hdr->pos_x;
  uint8_t y = obj_hdr->pos_y_and_parent_state & 0x7f;

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
      global_game_objects[obj_hdr->id] &= ~OBJ_STATE;
      debug_out("Cleared state of object %d due to identical position and size", obj_hdr->id);
    }
  }

  map_set_ds(ds_save);
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

void vm_camera_pan_to(uint8_t x)
{
  camera_target = x;
  camera_follow_actor_id = 0xff;
  camera_state = CAMERA_STATE_MOVE_TO_TARGET_POS;
}

void vm_delete_verb(uint8_t slot)
{
  verbs.id[slot] = 0xff;
}

void vm_verb_set_state(uint8_t slot, uint8_t state)
{
  verbs.state[slot] = state;
}

char *vm_verb_get_name(uint8_t slot)
{
  return verbs.name[slot];
}

/// @} // vm_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup vm_private VM Private Functions
 * @{
 */

static void set_proc_state(uint8_t slot, uint8_t state)
{
  proc_state[slot] &= ~0x07; // clear state without changing flags
  proc_state[slot] |= state;
}

static uint8_t get_proc_state(uint8_t slot)
{
  return proc_state[slot] & 0x07;
}

static uint8_t is_room_object_script(uint8_t slot)
{
  return proc_script_id[slot] == 0xff;
}

static void freeze_non_active_scripts(void)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot) {
    if (slot != active_script_slot && proc_state[slot] != PROC_STATE_FREE) {
      proc_state[slot] |= PROC_FLAGS_FROZEN;
    }
  }
}
static void unfreeze_scripts(void)
{
  for (uint8_t slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot) {
    proc_state[slot] &= ~PROC_FLAGS_FROZEN;
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
    message_timer = 60 + (uint16_t)timer_chars * (uint16_t)message_speed;
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
  map_cs_gfx();
  uint8_t num_jiffies_elapsed = gfx_wait_for_jiffy_timer();
  unmap_cs();
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

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;
  num_objects = room_hdr->num_objects;

  __auto_type image_offset = (struct offset *)NEAR_U8_PTR(RES_MAPPED + sizeof(struct room_header));
  __auto_type obj_hdr_offset = image_offset + num_objects;

  for (uint8_t i = 0; i < num_objects; ++i)
  {
    // read object and image offsets
    uint8_t cur_obj_offset = obj_hdr_offset->lsb;
    uint8_t cur_obj_page = room_res_slot + obj_hdr_offset->msb;
    uint8_t cur_image_offset = image_offset->lsb;
    uint8_t cur_image_page = room_res_slot + image_offset->msb;
    ++obj_hdr_offset;
    ++image_offset;
    obj_offset[i] = cur_obj_offset;
    obj_page[i] = cur_obj_page;

    // read object metadata from header
    map_ds_resource(cur_obj_page);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + cur_obj_offset);
    obj_id[i] = obj_hdr->id;
    uint8_t width = obj_hdr->width * 8;
    uint8_t height = obj_hdr->height_and_actor_dir & 0xf8;

    // read object image
    map_ds_resource(cur_image_page);
    uint8_t *obj_image = NEAR_U8_PTR(RES_MAPPED + cur_image_offset);
    gfx_decode_object_image(obj_image, width, height); 

    // reset ds back to room header
    map_ds_resource(room_res_slot);
  }
}

static void read_walk_boxes(void)
{
  __auto_type room_hdr = (struct room_header *)RES_MAPPED;
  __auto_type box_ptr = NEAR_U8_PTR(RES_MAPPED + room_hdr->walk_boxes_offset);
  num_walk_boxes = *box_ptr++;
  walk_boxes = (struct walk_box *)box_ptr;
  debug_out("Reading %d walk boxes", num_walk_boxes);
  for (uint8_t i = 0; i < num_walk_boxes; ++i) {
    __auto_type box = (struct walk_box *)box_ptr;
    box_ptr += sizeof(struct walk_box);
    debug_out("Walk box %d:", i);
    debug_out("  uy:  %d", box->top_y);
    debug_out("  ly:  %d", box->bottom_y);
    debug_out("  ulx: %d", box->topleft_x);
    debug_out("  urx: %d", box->topright_x);
    debug_out("  llx: %d", box->bottomleft_x);
    debug_out("  lrx: %d", box->bottomright_x);
    debug_out("  mask: %02x", box->mask);
    debug_out("  flags: %02x", box->flags);
  }
  walk_box_matrix = box_ptr;
  for (uint8_t i = 0; i < num_walk_boxes; ++i) {
    debug_out("  row offset %d = %d", i, *box_ptr++);
  }
  for (uint8_t src_box = 0; src_box < num_walk_boxes; ++src_box) {
    for (uint8_t dst_box = 0; dst_box < num_walk_boxes; ++dst_box) {
      uint8_t next_box = *box_ptr++;
      debug_out("  box matrix[%d][%d] = %d", src_box, dst_box, next_box);
    }
  }
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
    if (!(global_game_objects[obj_hdr->id] & OBJ_STATE)) {
      continue;
    }
    uint8_t width = obj_hdr->width;
    uint8_t height = obj_hdr->height_and_actor_dir >> 3;
    uint8_t x = obj_hdr->pos_x;
    uint8_t y = obj_hdr->pos_y_and_parent_state & 0x7f;
    unmap_ds();

    gfx_draw_object(i, x, y, width, height);
  }

  map_set(map_save);
}

static void redraw_actors(void)
{
  uint32_t map_save = map_get();
  map_cs_gfx();
  gfx_reset_cel_drawing();

  // iterate over all actors and draw their current cels on all cel levels
  for (uint8_t local_id = 0; local_id < MAX_LOCAL_ACTORS; ++local_id) {
    if (local_actors.global_id[local_id] == 0xff) {
      // no actor in local actor slot, skip
      continue;
    }

    actor_draw(local_id);
  }

  gfx_finalize_cel_drawing();

  map_set(map_save);
}

static void handle_input(void)
{
  // mouse cursor handling
  static uint8_t last_input_button_pressed = 0;

  vm_write_var(VAR_SCENE_CURSOR_X, input_cursor_x >> 2);
  vm_write_var(VAR_SCENE_CURSOR_Y, (input_cursor_y >> 1) - 8);

  if (input_button_pressed != last_input_button_pressed)
  {
    last_input_button_pressed = input_button_pressed;

    if (input_button_pressed == INPUT_BUTTON_LEFT)
    {
      if (input_cursor_y >= 16 && input_cursor_y < 144) {
        vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_SCENE_CLICK);
        vm_start_script(SCRIPT_ID_INPUT_EVENT);
      }
    }
  }

  // keyboard handling
  if (input_key_pressed) {
    if (input_key_pressed == INPUT_KEY_RUNSTOP || input_key_pressed == INPUT_KEY_ESCAPE) {
      override_cutscene();
    }
    // ack the key press
    input_key_pressed = 0;
  }
}

static uint8_t match_parent_object_state(uint8_t parent, uint8_t state)
{
  uint16_t ds_save = map_get_ds();

  map_ds_resource(obj_page[parent]);
  __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[parent]);
  if (obj_hdr->parent == 0) {
    map_set_ds(ds_save);
    return (obj_hdr->pos_y_and_parent_state & 0x80) == state;
  }

  uint8_t new_parent = obj_hdr->parent - 1;
  uint8_t new_state = obj_hdr->pos_y_and_parent_state & 0x80;
  map_set_ds(ds_save);
  return match_parent_object_state(new_parent, new_state);
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
    if (proc_state[slot] == PROC_STATE_FREE)
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
      proc_wait_timer[slot] += elapsed_jiffies;
      uint8_t timer_msb = (uint8_t)((uintptr_t)(proc_wait_timer[slot]) >> 24);
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
  uint8_t parent = 0xff;
  while (get_proc_state(slot) == PROC_STATE_WAITING_FOR_CHILD)
  {
    parent = slot;
    slot = proc_child[slot];
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
      // (remember we can be called from within a script)
      if (active_script_slot == save_active_script_slot) {
        break;
      }

      // save state on stack for nested scripts
      script_run_slot_stacked(active_script_slot);
    }
    if (get_proc_state(slot) == PROC_STATE_WAITING_FOR_CHILD) {
      // script has just started a new child script
      debug_out("Script in slot %d has started new child in slot %d", slot, proc_child[slot]);
      slot = proc_child[slot];
      continue;
    }
    else if (proc_state[slot] == PROC_STATE_FREE) {
      // script has exited, launch parent script if needed
      debug_out("Script in slot %d has exited, moving on to parent slot %d", slot, proc_parent[slot]);
      slot = proc_parent[slot];
      continue;
    }
    // in all other cases, the script will resume in the next cycle
    // no further parent or child scripts will be executed
    break;
  }

  active_script_slot = save_active_script_slot;

  map_set(map_save);
}

static uint8_t get_local_object_id(uint16_t global_object_id)
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

static uint8_t start_child_script_at_address(uint8_t res_slot, uint16_t offset)
{
  uint8_t slot = find_free_script_slot();
  proc_script_id[slot] = 0xff; // room scripts have no id
  proc_state[slot] = PROC_STATE_RUNNING;
  proc_parent[slot] = active_script_slot;
  proc_child[slot] = 0xff;
  proc_res_slot[slot] = res_slot;
  proc_pc[slot] = offset;
  
  // update the calling script to wait for the child script
  proc_child[active_script_slot] = slot;
  set_proc_state(active_script_slot, PROC_STATE_WAITING_FOR_CHILD);

  return slot;
}

static void execute_command_stack(void)
{
  if (cmd_stack.num_entries == 0)
  {
    return;
  }

  --cmd_stack.num_entries;
  uint8_t verb = cmd_stack.verb[cmd_stack.num_entries];
  uint16_t noun1 = cmd_stack.noun1[cmd_stack.num_entries];
  uint16_t noun2 = cmd_stack.noun2[cmd_stack.num_entries];

  vm_write_var(VAR_COMMAND_VERB, verb);
  vm_write_var(VAR_COMMAND_NOUN1, noun1);
  vm_write_var(VAR_COMMAND_NOUN2, noun2);
  
  uint8_t script_offset = 0;
  uint8_t local_object_id = get_local_object_id(noun1);
  if (local_object_id != 0xff) {
    script_offset = get_room_object_script_offset(verb, local_object_id);
  }
  vm_write_var(VAR_VALID_VERB, script_offset != 0);

  uint8_t slot = vm_start_script(SCRIPT_ID_COMMAND);
  debug_out("Command script verb %d noun1 %d noun2 %d valid-verb %d", verb, noun1, noun2, script_offset != 0);
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
    proc_pc[cs_proc_slot] = cs_override_pc;
    cs_override_pc = 0;
    proc_state[cs_proc_slot] &= ~PROC_FLAGS_FROZEN;
    if (proc_state[cs_proc_slot] == PROC_STATE_WAITING_FOR_TIMER) {
      proc_state[cs_proc_slot] = PROC_STATE_RUNNING;
    }
  }
}

/// @} // vm_private

//-----------------------------------------------------------------------------------------------
