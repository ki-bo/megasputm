#include "vm.h"
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
  uint8_t  boxes_offset;
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

#pragma clang section bss="zdata"

uint8_t global_game_objects[780];
uint8_t variables_lo[256];
uint8_t variables_hi[256];
char dialog_buffer[256];

uint8_t actor_sounds[NUM_ACTORS];
uint8_t actor_palette_idx[NUM_ACTORS];
uint8_t actor_palette_colors[NUM_ACTORS];
char    actor_names[NUM_ACTORS][ACTOR_NAME_LEN];
uint8_t actor_costumes[NUM_ACTORS];
uint8_t actor_talk_colors[NUM_ACTORS];
uint8_t actor_talking;

uint8_t state_iface;
uint16_t camera_x;

uint8_t active_script_slot;
uint8_t dialog_speed = 6;
uint16_t dialog_timer;
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
uint8_t cs_cursor_state;
uint8_t cs_iface_state;
uint16_t cs_pc;

// room and object data
uint8_t  room_res_slot;
uint8_t  num_objects;
uint8_t  obj_page[MAX_OBJECTS];
uint8_t  obj_offset[MAX_OBJECTS];
uint16_t obj_id[MAX_OBJECTS];
uint8_t  screen_update_needed = 0;

// command queue
struct cmd_stack_t cmd_stack;

// Private functions
static void process_dialog(uint8_t jiffies_elapsed);
static uint8_t wait_for_jiffy(void);
static void read_objects(void);
static void redraw_screen(void);
static void handle_input(void);
static uint8_t match_parent_object_state(uint8_t parent, uint8_t state);
static uint8_t find_free_script_slot(void);
static void update_script_timers(uint8_t elapsed_jiffies);
static void execute_script_slot(uint8_t slot);
static uint8_t get_local_object_id(uint16_t global_object_id);
static uint8_t start_child_script_at_address(uint8_t res_slot, uint16_t offset);
static void execute_command_stack(void);
static uint8_t get_room_object_script_offset(uint8_t verb, uint8_t local_object_id);

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
  dialog_timer = 0;

  while (1) {
    uint8_t elapsed_jiffies = wait_for_jiffy();
    map_cs_diskio();
    diskio_check_motor_off(elapsed_jiffies);
    unmap_cs();

    process_dialog(elapsed_jiffies);

    handle_input();

    update_script_timers(elapsed_jiffies);

    for (active_script_slot = 0; 
         active_script_slot < NUM_SCRIPT_SLOTS;
         ++active_script_slot)
    {
      execute_script_slot(active_script_slot);
    }

    execute_command_stack();

    if (screen_update_needed)
    {
      //VICIV.bordercol = 15;
      redraw_screen();
      screen_update_needed = 0;
      //VICIV.bordercol = 0;
      map_cs_gfx();
      gfx_update_screen();
      unmap_cs();
    }
  }
}

/**
 * @brief Returns the current state of the active process
 * 
 * @return uint8_t The current state of the active process
 *
 * Code section: code_main
 */
uint8_t vm_get_active_proc_state(void)
{
  return proc_state[active_script_slot];
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
void vm_switch_room(uint8_t room_no)
{
  // save DS
  uint16_t ds_save = map_get_ds();

  debug_out("Switching to room %02x", room_no);

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;

  // exit and free old room
  debug_out("Deactivating old room %02x", vm_read_var8(VAR_ROOM_NO));
  if (vm_read_var(VAR_ROOM_NO) != 0) {
    map_ds_resource(room_res_slot);
    vm_start_room_script(room_hdr->exit_script_offset + 4);
    res_deactivate(RES_TYPE_ROOM, vm_read_var8(VAR_ROOM_NO), 0);
  }

  // activate new room data
  room_res_slot = res_provide(RES_TYPE_ROOM, room_no, 0);
  res_set_flags(room_res_slot, RES_ACTIVE_MASK);
  map_ds_resource(room_res_slot);

  map_cs_gfx();
  gfx_fade_out();
  gfx_decode_bg_image(NEAR_U8_PTR(RES_MAPPED + room_hdr->bg_data_offset), 
                      room_hdr->bg_width);

  vm_write_var(VAR_ROOM_NO, room_no);

  read_objects();

  // run entry script
  map_ds_resource(room_res_slot);
  vm_start_room_script(room_hdr->entry_script_offset + 4);

  redraw_screen();

  map_cs_gfx();
  gfx_fade_in();
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
void vm_start_cutscene(void)
{
  cs_room = vm_read_var8(VAR_ROOM_NO);
  cs_cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  cs_iface_state = state_iface;
}

/**
 * @brief Outputs dialog on screen for the specified actor
 * 
 * Outputs dialog on screen for the specified actor. The dialog is stored in the dialog buffer
 * and the dialog timer is set to the number of jiffies needed to display the dialog. The
 * variable VAR_DIALOG_ACTIVE is set to 1 while the dialog is presented on screen. It will
 * be reset back to 0 in the main loop once the dialog timer expires.
 * 
 * For general text outputs, actor_id 0xff can be used.
 *
 * The dialog is directly printed on screen without waiting for a redraw event.

 * @param actor_id ID of the actor talking, will be used to select the configured text color
 *                 for the text output on screen.
 *
 * Code section: code_main
 */
void vm_actor_start_talking(uint8_t actor_id)
{
  actor_talking = actor_id;
  uint8_t talk_color = actor_id != 0xff ? actor_talk_colors[actor_id] : 0x09;
  map_cs_gfx();
  uint8_t num_chars = gfx_print_dialog(talk_color, dialog_buffer);
  unmap_cs();
  dialog_timer = 60 + (uint16_t)num_chars * (uint16_t)dialog_speed;
  vm_write_var(VAR_DIALOG_ACTIVE, 1);
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
  res_set_flags(new_page, RES_ACTIVE_MASK);
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

  uint8_t script_offset = get_room_object_script_offset(verb, id);
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
  res_deactivate(RES_TYPE_SCRIPT, proc_res_slot[active_script_slot], 0);
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
      res_deactivate(RES_TYPE_SCRIPT, proc_res_slot[slot], 0);
      proc_state[slot] = PROC_STATE_FREE;
      // stop all children
      while (proc_child[slot] != 0xff)
      {
        proc_child[slot] = 0xff;
        slot = proc_child[slot];
        res_deactivate(RES_TYPE_SCRIPT, proc_res_slot[slot], 0);
        proc_state[slot] = PROC_STATE_FREE;
      }
    }
  }
}

/**
 * @brief Requests that the screen is updated
 *
 * The screen update flag is set to 1, which will trigger a screen update in the main loop.
 * The redraw will happen after all active scripts have finished their current execution cycle.
 *
 * Code section: code_main
 */
void vm_update_screen(void)
{
  screen_update_needed = 1;
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

/// @} // vm_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup vm_private VM Private Functions
 * @{
 */

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
  if (dialog_timer == 0)
  {
    return;
  }

  if (dialog_timer >= jiffies_elapsed)
  {
    dialog_timer -= jiffies_elapsed;
  }
  else
  {
    dialog_timer = 0;
  }

  if (!dialog_timer) {
    map_cs_gfx();
    gfx_clear_dialog();
    unmap_cs();
    vm_write_var(VAR_DIALOG_ACTIVE, 0);
  }
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
  gfx_draw_bg();

  for (int8_t i = num_objects - 1; i >= 0; --i)
  {
    map_ds_resource(obj_page[i]);
    __auto_type obj_hdr = (struct object_code *)NEAR_U8_PTR(RES_MAPPED + obj_offset[i]);
    if (!(global_game_objects[obj_hdr->id] & OBJ_STATE_ACTIVE)) {
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

static void handle_input(void)
{
  static uint8_t last_input_button_pressed = 0;

  vm_write_var(VAR_SCENE_CURSOR_X, input_cursor_x >> 2);
  vm_write_var(VAR_SCENE_CURSOR_Y, (input_cursor_y >> 1) - 8);

  if (input_button_pressed == last_input_button_pressed)
  {
    return;
  }
  last_input_button_pressed = input_button_pressed;

  if (input_button_pressed == INPUT_BUTTON_LEFT)
  {
    if (input_cursor_y >= 16 && input_cursor_y < 144) {
      vm_write_var(VAR_INPUT_EVENT, INPUT_EVENT_SCENE_CLICK);
      vm_start_script(SCRIPT_ID_INPUT_EVENT);
    }
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
    if (proc_state[slot] == PROC_STATE_WAITING_FOR_TIMER)
    {
      proc_wait_timer[slot] += elapsed_jiffies;
      uint8_t timer_msb = (uint8_t)((uintptr_t)(proc_wait_timer[slot]) >> 24);
      if (timer_msb == 0)
      {
        proc_state[slot] = PROC_STATE_RUNNING;
      }
    }
  }

}

static void execute_script_slot(uint8_t slot)
{
  // non-top-level scripts will be executed as childs in other scripts, so skip them here
  if (proc_parent[slot] != 0xff) {
    return;
  }

  // walk down to last child
  uint8_t parent = 0xff;
  while (proc_state[slot] == PROC_STATE_WAITING_FOR_CHILD)
  {
    parent = slot;
    slot = proc_child[slot];
  }

  uint8_t save_active_script_slot = active_script_slot;

  while (slot != 0xff && proc_state[slot] == PROC_STATE_RUNNING) {
    active_script_slot = slot;
    script_run_active_slot();
    if (proc_state[slot] == PROC_STATE_WAITING_FOR_CHILD) {
      // script has just started a new child script
      debug_out("Script %d has started new child %d", slot, proc_child[slot]);
      slot = proc_child[slot];
      continue;
    }
    else if (proc_state[slot] == PROC_STATE_FREE) {
      // script has exited, launch parent script if needed
      debug_out("Script %d has exited, moving on to parent %d", slot, proc_parent[slot]);
      slot = proc_parent[slot];
      continue;
    }
    // in all other cases, the script will resume in the next cycle
    // no further parent or child scripts will be executed
    break;
  }

  active_script_slot = save_active_script_slot;
}

static uint8_t get_local_object_id(uint16_t global_object_id)
{
  debug_out("Searching for object %d", global_object_id);
  for (uint8_t i = 0; i < num_objects; ++i)
  {
    if (obj_id[i] == global_object_id)
    {
      debug_out("  Found object at local id %d", i);
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
  proc_state[active_script_slot] = PROC_STATE_WAITING_FOR_CHILD;

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
  uint16_t object_left = cmd_stack.object_left[cmd_stack.num_entries];
  uint16_t object_right = cmd_stack.object_right[cmd_stack.num_entries];

  vm_write_var(VAR_COMMAND_VERB, verb);
  vm_write_var(VAR_COMMAND_OBJECT_LEFT, object_left);
  vm_write_var(VAR_COMMAND_OBJECT_RIGHT, object_right);
  
  uint8_t script_offset = 0;
  uint8_t local_object_id = get_local_object_id(object_left);
  if (local_object_id != 0xff) {
    script_offset = get_room_object_script_offset(verb, local_object_id);
  }
  vm_write_var(VAR_COMMAND_VERB_AVAILABLE, script_offset != 0);

  uint8_t slot = vm_start_script(SCRIPT_ID_COMMAND);
  debug_out("Command script verb %d object1 %d object2 %d verb_available %d", verb, object_left, object_right, script_offset != 0);
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

  debug_out("Searching for verb %02x in object %d", verb, local_object_id);

  while (*ptr != 0) {
    debug_out("Verb %02x, offset %02x", *ptr, *(ptr + 1));
    if (*ptr == verb || *ptr == 0xff) {
      script_offset = *(ptr + 1);
      break;
    }
    ptr += 2;
  }

  map_set_ds(save_ds);

  return script_offset;
}

/// @} // vm_private

//-----------------------------------------------------------------------------------------------
