#include "script.h"
#include "actor.h"
#include "error.h"
#include "gfx.h"
#include "inventory.h"
#include "io.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include "walk_box.h"
#include <mega65.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------

#ifdef DEBUG_SCRIPTS
#define debug_scr(...) debug_out(__VA_ARGS__)
#else
#define debug_scr(...)
#endif

// private functions
static uint8_t run_active_slot(void);
static uint8_t run_slot_stacked(uint8_t slot);
static void run_script_first_time(uint8_t slot);
static uint8_t find_free_script_slot(void);
static void reset_script_slot(uint8_t slot, uint8_t type, uint16_t script_or_object_id, uint8_t parent, uint8_t res_slot, uint16_t offset);
static void stop_script_from_table(uint8_t table_idx);
static void proc_slot_table_insert(uint8_t slot);
static void debug_header();
static void exec_opcode(uint8_t opcode);
static uint8_t read_byte(void);
static uint16_t read_word(void);
static uint32_t read_24bits(void);
static uint8_t resolve_next_param8(void);
static uint16_t resolve_next_param16(void);
static void read_null_terminated_string(char *dest);

static void stop_or_break(void);
static void put_actor(void);
static void start_music(void);
static void actor_room(void);
static void jump_if_greater(void);
static void draw_object(void);
static void assign_array(void);
static void jump_if_equal(void);
static void face_towards(void);
static void assign_variable_indirect(void);
static void state_of(void);
static void resource_cmd(void);
static void walk_to_actor(void);
static void put_actor_at_object(void);
static void owner_of(void);
static void do_animation(void);
static void camera_pan_to(void);
static void actor_ops(void);
static void say_line(void);
static void find_actor(void);
static void random_number(void);
static void set_or_clear_untouchable(void);
static void jump_or_restart(void);
static void do_sentence(void);
static void assign_variable(void);
static void assign_bit_variable(void);
static void start_sound(void);
static void walk_to(void);
static void savegame_operation(void);
static void actor_y(void);
static void come_out_door(void);
static void jump_if_or_if_not_equal_zero(void);
static void set_owner_of(void);
static void sleep_for_variable(void);
static void put_actor_in_room(void);
static void subtract(void);
static void wait_for_actor(void);
static void stop_sound(void);
static void actor_elevation(void);
static void jump_if_or_if_not_pickupable(void);
static void add(void);
static void sleep_for_or_wait_for_message(void);
static void jump_if_or_if_not_locked(void);
static void set_box(void);
static void assign_from_bit_variable(void);
static void camera_at(void);
static void proximity(void);
static void get_object_at_position(void);
static void walk_to_object(void);
static void set_or_clear_pickupable(void);
static void jump_if_smaller(void);
static void cut_scene(void);
static void start_script(void);
static void actor_x(void);
static void jump_if_smaller_or_equal(void);
static void increment_or_decrement(void);
static void jump_if_not_equal(void);
static void chain_script(void);
static void jump_if_object_active_or_not_active(void);
static void pick_up_object(void);
static void camera_follows_actor(void);
static void new_name_of(void);
static void begin_override_or_say_line_selected_actor(void);
static void begin_override(void);
static void cursor(void);
static void stop_script(void);
static void closest_actor(void);
static void lock_or_unlock(void);
static void script_running(void);
static void preposition(void);
static void lights(void);
static void current_room(void);
static void jump_if_greater_or_equal(void);
static void verb(void);
static void sound_running(void);
static void say_line_selected_actor(void);
static void unimplemented_opcode(void);

//----------------------------------------------------------------------

#pragma clang section bss="zzpage"

// private zeropage variables
uint8_t __attribute__((zpage)) parallel_script_count;
static uint8_t __attribute__((zpage)) opcode;
static uint8_t __attribute__((zpage)) param_mask;
static uint8_t * __attribute__((zpage)) pc;

//----------------------------------------------------------------------

#pragma clang section bss="zdata"

// private variables
static void (*opcode_jump_table[128])(void);
static uint8_t break_script = 0;
static uint8_t backup_opcode;
static uint8_t backup_param_mask;
static uint8_t *backup_pc;
static uint8_t backup_break_script;
#ifdef DEBUG_SCRIPTS
  uint16_t active_script_id;
#endif


//----------------------------------------------------------------------

/**
  * @defgroup script_init Script Init Functions
  * @{
  */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata"

/**
  * @brief Initializes the script module.
  * 
  * Initializes the opcode jump table with the corespinding function
  * pointers for each opcode.
  *
  * Code section: code_init
  */
void script_init(void)
{
  for (uint8_t i = 0; i < 128; i++) {
    opcode_jump_table[i] = &unimplemented_opcode;
  }

  opcode_jump_table[0x00] = &stop_or_break;
  opcode_jump_table[0x01] = &put_actor;
  opcode_jump_table[0x02] = &start_music;
  opcode_jump_table[0x03] = &actor_room;
  opcode_jump_table[0x04] = &jump_if_greater;
  opcode_jump_table[0x05] = &draw_object;
  opcode_jump_table[0x07] = &state_of;
  opcode_jump_table[0x08] = &jump_if_equal;
  opcode_jump_table[0x09] = &face_towards;
  opcode_jump_table[0x0a] = &assign_variable_indirect;
  opcode_jump_table[0x0c] = &resource_cmd;
  opcode_jump_table[0x0d] = &walk_to_actor;
  opcode_jump_table[0x0e] = &put_actor_at_object;
  opcode_jump_table[0x0f] = &jump_if_object_active_or_not_active;
  opcode_jump_table[0x10] = &owner_of;
  opcode_jump_table[0x11] = &do_animation;
  opcode_jump_table[0x12] = &camera_pan_to;
  opcode_jump_table[0x13] = &actor_ops;
  opcode_jump_table[0x14] = &say_line;
  opcode_jump_table[0x15] = &find_actor;
  opcode_jump_table[0x16] = &random_number;
  opcode_jump_table[0x17] = &set_or_clear_untouchable;
  opcode_jump_table[0x18] = &jump_or_restart;
  opcode_jump_table[0x19] = &do_sentence;
  opcode_jump_table[0x1a] = &assign_variable;
  opcode_jump_table[0x1b] = &assign_bit_variable;
  opcode_jump_table[0x1c] = &start_sound;
  opcode_jump_table[0x1e] = &walk_to;
  opcode_jump_table[0x20] = &stop_or_break;
  opcode_jump_table[0x21] = &put_actor;
  opcode_jump_table[0x22] = &savegame_operation;
  opcode_jump_table[0x23] = &actor_y;
  opcode_jump_table[0x24] = &come_out_door;
  opcode_jump_table[0x25] = &draw_object;
  opcode_jump_table[0x26] = &assign_array;
  opcode_jump_table[0x27] = &lock_or_unlock;
  opcode_jump_table[0x28] = &jump_if_or_if_not_equal_zero;
  opcode_jump_table[0x29] = &set_owner_of;
  opcode_jump_table[0x2b] = &sleep_for_variable;
  opcode_jump_table[0x2d] = &put_actor_in_room;
  opcode_jump_table[0x2e] = &sleep_for_or_wait_for_message;
  opcode_jump_table[0x2f] = &jump_if_or_if_not_locked;
  opcode_jump_table[0x30] = &set_box;
  opcode_jump_table[0x31] = &assign_from_bit_variable;
  opcode_jump_table[0x32] = &camera_at;
  opcode_jump_table[0x3e] = &walk_to;
  opcode_jump_table[0x34] = &proximity;
  opcode_jump_table[0x35] = &get_object_at_position;
  opcode_jump_table[0x36] = &walk_to_object;
  opcode_jump_table[0x37] = &set_or_clear_pickupable;
  opcode_jump_table[0x38] = &jump_if_smaller;
  opcode_jump_table[0x39] = &do_sentence;
  opcode_jump_table[0x3a] = &subtract;
  opcode_jump_table[0x3b] = &wait_for_actor;
  opcode_jump_table[0x3c] = &stop_sound;
  opcode_jump_table[0x3d] = &actor_elevation;
  opcode_jump_table[0x3f] = &jump_if_or_if_not_pickupable;
  opcode_jump_table[0x40] = &cut_scene;
  opcode_jump_table[0x41] = &put_actor;
  opcode_jump_table[0x42] = &start_script;
  opcode_jump_table[0x43] = &actor_x;
  opcode_jump_table[0x44] = &jump_if_smaller_or_equal;
  opcode_jump_table[0x45] = &draw_object;
  opcode_jump_table[0x46] = &increment_or_decrement;
  opcode_jump_table[0x47] = &state_of;
  opcode_jump_table[0x48] = &jump_if_not_equal;
  opcode_jump_table[0x49] = &face_towards;
  opcode_jump_table[0x4a] = &chain_script;
  opcode_jump_table[0x4d] = &walk_to_actor;
  opcode_jump_table[0x4e] = &put_actor_at_object;
  opcode_jump_table[0x4f] = &jump_if_object_active_or_not_active;
  opcode_jump_table[0x50] = &pick_up_object;
  opcode_jump_table[0x51] = &do_animation;
  opcode_jump_table[0x52] = &camera_follows_actor;
  opcode_jump_table[0x53] = &actor_ops;
  opcode_jump_table[0x54] = &new_name_of;
  opcode_jump_table[0x55] = &find_actor;
  opcode_jump_table[0x57] = &set_or_clear_untouchable;
  opcode_jump_table[0x58] = &begin_override_or_say_line_selected_actor;
  opcode_jump_table[0x59] = &do_sentence;
  opcode_jump_table[0x5a] = &add;
  opcode_jump_table[0x5b] = &assign_bit_variable;
  opcode_jump_table[0x5e] = &walk_to;
  opcode_jump_table[0x60] = &cursor;
  opcode_jump_table[0x61] = &put_actor;
  opcode_jump_table[0x62] = &stop_script;
  opcode_jump_table[0x64] = &come_out_door;
  opcode_jump_table[0x65] = &draw_object;
  opcode_jump_table[0x66] = &closest_actor;
  opcode_jump_table[0x67] = &lock_or_unlock;
  opcode_jump_table[0x68] = &script_running;
  opcode_jump_table[0x69] = &set_owner_of;
  opcode_jump_table[0x6c] = &preposition;
  opcode_jump_table[0x6d] = &put_actor_in_room;
  opcode_jump_table[0x6f] = &jump_if_or_if_not_locked;
  opcode_jump_table[0x70] = &lights;
  opcode_jump_table[0x72] = &current_room;
  opcode_jump_table[0x74] = &proximity;
  opcode_jump_table[0x75] = &get_object_at_position;
  opcode_jump_table[0x76] = &walk_to_object;
  opcode_jump_table[0x77] = &set_or_clear_pickupable;
  opcode_jump_table[0x78] = &jump_if_greater_or_equal;
  opcode_jump_table[0x79] = &do_sentence;
  opcode_jump_table[0x7a] = &verb;
  opcode_jump_table[0x7c] = &sound_running;
  opcode_jump_table[0x3d] = &actor_elevation;
  opcode_jump_table[0x7e] = &walk_to;
  opcode_jump_table[0x7f] = &jump_if_or_if_not_pickupable;
}

/// @} // script_init

//----------------------------------------------------------------------

/**
  * @defgroup script_public Script Public Functions
  * @{
  */
#pragma clang section text="code_script" rodata="cdata_script" data="data_script" bss="zdata"

void script_schedule_init_script(void)
{
  uint8_t script_id = 1;
  uint8_t script1_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_activate_slot(script1_page);
  reset_script_slot(0, PROC_TYPE_GLOBAL, script_id, 0xff, script1_page, 4 /* skipping script header directly to first opcode*/);
  vm_state.proc_slot_table[0] = 0;
  vm_state.num_active_proc_slots = 1;
}

uint8_t script_execute_slot(uint8_t slot)
{
  uint8_t save_active_script_slot = active_script_slot;
  uint8_t result;

  active_script_slot = slot;

  if (parallel_script_count == 0) {
    // top-level script
    //debug_out("  executing top-level script slot %d", slot);
    result = run_active_slot();
  }
  else {
    // script execution from within another script
    // need to stack the current script state
    //debug_out("  executing child script slot %d", slot);
    result = run_slot_stacked(active_script_slot);
  }
  if (result != PROC_STATE_FREE && vm_state.proc_parent[slot] != 0xff) {
    vm_state.proc_parent[slot] = 0xff;
  }

  active_script_slot = save_active_script_slot;

  return result;
}

uint16_t script_get_current_pc(void)
{
  return (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED));
}

void script_break(void)
{
  break_script = 1;
}

/**
  * @brief Starts a global script
  * 
  * The script with the given id is added to a free process slot and marked with state
  * PROC_STATE_RUNNING. The script will start execution immediately.
  *
  * @param script_id 
  *
  * Code section: code_script
  */
uint8_t script_start(uint8_t script_id)
{
  uint8_t slot = vm_get_first_script_slot_by_script_id(script_id);
  if (slot != 0xff) {
    //debug_out("Script %d already running in slot %d", script_id, slot);
    script_stop(script_id);
  }
  else {
    slot = find_free_script_slot();
  }

  uint8_t new_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_activate_slot(new_page);
  proc_res_slot[slot] = new_page;

  uint8_t parent = script_is_room_object_script(active_script_slot) ? 0xff : active_script_slot;

  // init script slot
  reset_script_slot(slot, PROC_TYPE_GLOBAL, script_id, parent, new_page, 4 /* skipping script header directly to first opcode*/);

  //debug_out("Starting global script %d in slot %d", script_id, slot);

  run_script_first_time(slot);

  return slot;
}

void script_execute_room_script(uint16_t room_script_offset)
{
  //debug_out("Starting room entry/exit script at offset %x", room_script_offset);

  uint8_t  res_slot = room_res_slot + MSB(room_script_offset);
  uint16_t offset   = LSB(room_script_offset);

  uint8_t script_slot = find_free_script_slot();

  reset_script_slot(script_slot, 0 /*type*/, 0xffff /*script_id*/, 0xff /*parent*/, res_slot, offset);
  run_script_first_time(script_slot);
}

void script_execute_object_script(uint8_t verb, uint16_t global_object_id, uint8_t background)
{
  SAVE_DS_AUTO_RESTORE

  uint8_t  is_inventory;
  uint8_t  type;
  uint8_t  script_slot;
  uint16_t script_offset;
  uint8_t  res_slot;
  uint8_t  id = vm_get_local_object_id(global_object_id);
  if (id != 0xff) {
    is_inventory  = 0;
    type          = 0;
    res_slot      = obj_page[id];
    script_offset = obj_offset[id];
  }
  else {
    id = inv_get_position_by_id(global_object_id);
    if (id != 0xff) {
      is_inventory  = 1;
      type          = PROC_TYPE_INVENTORY;
      res_slot      = 0;
      script_offset = (uint16_t)vm_state.inv_objects[id] - RES_MAPPED;
    }
    else {
      // object not in room or inventory
      return;
    }
  }

  uint8_t verb_offset = vm_get_room_object_script_offset(verb, id, is_inventory);
  if (verb_offset == 0) {
    return;
  }
  script_offset += verb_offset;

  if (background) {
    type |= PROC_TYPE_BACKGROUND;
  }
  if (verb < 250) {
    type |= PROC_TYPE_REGULAR_VERB;
  }

  for (script_slot = 0; script_slot < NUM_SCRIPT_SLOTS; ++script_slot) {
    if (vm_state.proc_state[script_slot] != PROC_STATE_FREE &&
        vm_state.proc_type[script_slot] == (type & (PROC_TYPE_BACKGROUND | PROC_TYPE_REGULAR_VERB)) &&
        vm_state.proc_script_or_object_id[script_slot] == LSB(global_object_id) &&
        vm_state.proc_object_id_msb[script_slot] == MSB(global_object_id)) {
      //debug_out("reusing script slot %d for object %d verb %d", script_slot, global_object_id, verb);
      break;
    }
  }
  
  if (script_slot == NUM_SCRIPT_SLOTS) {
    script_slot = find_free_script_slot();
  }

  //debug_out("start object script %d verb %d type %d in slot %d", global_object_id, verb, type, script_slot);
  reset_script_slot(script_slot, type, global_object_id, 0xff /*parent*/, res_slot, script_offset);

  run_script_first_time(script_slot);
}

/**
  * @brief Stops the script in the specified script slot
  * 
  * Deactivates the script and sets the process state to PROC_STATE_FREE.
  * The associated script resource is deactivated and thus marked free to override if needed.
  *
  * @param script_id The id of the script to stop
  *
  * Code section: code_script
  */
void script_stop_slot(uint8_t slot)
{
  vm_state.proc_state[slot] = PROC_STATE_FREE;

  if (!script_is_room_object_script(slot)) {
    //debug_out("Script %d ended slot %d", vm_state.proc_script_or_object_id[slot], slot);
    res_deactivate_slot(proc_res_slot[slot]);

    for (uint8_t table_idx = 1; table_idx < vm_state.num_active_proc_slots; ++table_idx)
    {
      // stop children of us
      uint8_t child_slot = vm_state.proc_slot_table[table_idx];
      if (child_slot != 0xff && vm_state.proc_parent[child_slot] == slot) {
        stop_script_from_table(table_idx);
      }
    }

  }
  else {
    // debug_out("Object script %d ended slot %d", 
    //           make16(vm_state.proc_script_or_object_id[slot]), vm_state.proc_object_id_msb[slot]), 
    //           slot);
  }

  // Mark all stopped scripts as 0xff in slot table
  for (uint8_t table_idx = 0; table_idx < vm_state.num_active_proc_slots; ++table_idx)
  {
    uint8_t tmp_slot = vm_state.proc_slot_table[table_idx];
    if (tmp_slot != 0xff && vm_state.proc_state[tmp_slot] == PROC_STATE_FREE) {
      vm_state.proc_slot_table[table_idx] = 0xff;
    }
  }

  // Will remove all 0xff entries from the table after all scripts have been processed
  // in the current cycle
  proc_table_cleanup_needed = 1;

  //script_print_slot_table();
}

void script_stop(uint8_t script_id)
{
  // check active script
  if (active_script_slot != 0xff &&
      vm_state.proc_type[active_script_slot] == PROC_TYPE_GLOBAL &&
      vm_state.proc_script_or_object_id[active_script_slot] == script_id)
  {
    script_stop_slot(active_script_slot);
  }

  // check all scripts in slot table
  for (uint8_t table_idx = 0; table_idx < vm_state.num_active_proc_slots; ++table_idx)
  {
    uint8_t slot = vm_state.proc_slot_table[table_idx];
    if (slot != 0xff &&
        vm_state.proc_type[slot] == PROC_TYPE_GLOBAL &&
        vm_state.proc_script_or_object_id[slot] == script_id)
    {
      script_stop_slot(slot);
    }
  }
}

// void script_print_slot_table(void)
// {
//   debug_out2("Table (%d slots):", vm_state.num_active_proc_slots);
//   for (uint8_t i = 0; i < vm_state.num_active_proc_slots; ++i) {
//     uint8_t slot = vm_state.proc_slot_table[i];
//     if (slot == 0xff) {
//       debug_msg2(" X");
//       continue;
//     }
//     uint16_t id = make16(vm_state.proc_script_or_object_id[slot], vm_state.proc_object_id_msb[slot]);
//     debug_out2(" %d(%d)", slot, id);
//     uint8_t state = vm_get_proc_state(slot);
//     if (!(vm_state.proc_type[slot] & PROC_TYPE_GLOBAL)) {
//       debug_msg2("o");
//     }
//     if (state == PROC_STATE_FREE) {
//       debug_msg2("X");
//     }
//     else if (state == PROC_STATE_WAITING_FOR_TIMER) {
//       debug_msg2("W");
//     }
//     else if (state == PROC_STATE_RUNNING) {
//       debug_msg2("R");
//     }
//     else {
//       debug_out2("?%d", state);
//     }
//     if (vm_state.proc_state[slot] & PROC_FLAGS_FROZEN) {
//       debug_msg2("F");
//     }
//   }
//   debug_out("");
// }

uint8_t script_is_room_object_script(uint8_t slot)
{
  return (vm_state.proc_type[slot] & PROC_TYPE_GLOBAL) == 0;
}

#pragma clang section text="code"

/**
  * @brief Executes the function for the given opcode.
  *
  * We are not doing this code inline but as a separate function
  * because the compiler would otherwise not know about the clobbering
  * of registers (including zp registers) by the called function.
  * Placing the function in section code will make sure that it
  * is not inlined by functions in other sections, like code_script.
  * 
  * The highest bit of the opcode is ignored, so opcodes 128-255 are
  * jumping to the same function as opcodes 0-127.
  *
  * @param opcode The opcode to be called (0-127).
  *
  * Code section: code
  */
 //__attribute__((section("code")))
void exec_opcode(uint8_t opcode)
{
  //opcode_jump_table[opcode]();
  __asm (" asl a\n"
         " tax\n"
         " jsr (opcode_jump_table, x)"
         : /* no output operands */
         : "Ka" (opcode)
         : "x");
}

#pragma clang section text="code_script"

/**
  * @brief Runs the next script cycle of the currently active script slot.
  *
  * The script is run from the current pc until the script's state is not PROC_STATE_RUNNING.
  * 
  * Code section: code_script
  */
static uint8_t run_active_slot(void)
{
  if (parallel_script_count == 6) {
    fatal_error(ERR_SCRIPT_RECURSION);
  }

  ++parallel_script_count;

  break_script = 0;

#ifdef DEBUG_SCRIPTS
  char *script_type  = vm_state.proc_type[active_script_slot] & PROC_TYPE_GLOBAL ? "S" : 
                       vm_state.proc_type[active_script_slot] & PROC_TYPE_INVENTORY ? "I" : "R";
  active_script_id = make16(vm_state.proc_script_or_object_id[active_script_slot], vm_state.proc_object_id_msb[active_script_slot]);
#endif

  SAVE_DS_AUTO_RESTORE
  if (vm_state.proc_type[active_script_slot] & PROC_TYPE_INVENTORY) {
    UNMAP_DS
  }
  else {
    map_ds_resource(proc_res_slot[active_script_slot]);
  }
  pc = NEAR_U8_PTR(RES_MAPPED) + vm_state.proc_pc[active_script_slot];
  // check for PROC_STATE_RUNNING only will also mean we won't continue executing if
  // PROC_FLAGS_FROZEN is set.
  while (vm_get_active_proc_state_and_flags() == PROC_STATE_RUNNING && !(break_script)) {
    opcode = read_byte();
    param_mask = 0x80;
#ifdef DEBUG_SCRIPTS
    debug_out("[%d]%s %d (%x) %x", 
      active_script_slot, 
      script_type,
      active_script_id,
      (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED) - 5), 
      opcode);
#endif
    exec_opcode(opcode);
  }

  vm_state.proc_pc[active_script_slot] = (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED));
  --parallel_script_count;


  return vm_get_active_proc_state_and_flags();
}

static uint8_t run_slot_stacked(uint8_t slot)
{
  uint8_t  stack_opcode       = opcode;
  uint8_t  stack_param_mask   = param_mask;
  uint8_t *stack_pc           = pc;
  uint8_t  stack_break_script = break_script;
  uint8_t  stack_active_slot  = active_script_slot;
#ifdef DEBUG_SCRIPTS
  uint16_t stack_active_script_id = active_script_id;
#endif

  active_script_slot = slot;
  uint8_t state = run_active_slot();

  opcode = stack_opcode;
  param_mask = stack_param_mask;
  pc = stack_pc;
  break_script = stack_break_script;
  active_script_slot = stack_active_slot;
#ifdef DEBUG_SCRIPTS
  active_script_id = stack_active_script_id;
#endif

  return state;
}

static void run_script_first_time(uint8_t slot)
{
  // execute new script immediately (like subroutine)
  script_execute_slot(slot);

  if (vm_state.proc_state[slot] != PROC_STATE_FREE) {
    // script hit first break and is still running, put it into slot table for scheduling next cycle
    // (it will be placed directly after the current script in the slot table)
    proc_slot_table_insert(slot);
    // increase exec counter so we don't execute it again in this cycle
    ++proc_slot_table_exec;
    //script_print_slot_table();
  }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
/**
  * @brief Finds a free script slot
  *
  * Finds a free script slot by iterating over all script slots and returning the first free slot
  * found. If no free slot is found, the function will trigger a fatal error.
  *
  * @return The index of the first free script slot
  *
  * Code section: code_script
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

static void reset_script_slot(uint8_t slot, uint8_t type, uint16_t script_or_object_id, uint8_t parent, uint8_t res_slot, uint16_t offset)
{
  vm_state.proc_script_or_object_id[slot] = LSB(script_or_object_id);
  vm_state.proc_object_id_msb[slot]       = MSB(script_or_object_id);
  vm_state.proc_state[slot]               = PROC_STATE_RUNNING;
  vm_state.proc_parent[slot]              = parent;
  vm_state.proc_type[slot]                = type;
  proc_res_slot[slot]                     = res_slot;
  vm_state.proc_pc[slot]                  = offset;
}

/**
  * @brief Stops a script from the slot table and also stops all its children
  *
  * Function will stop the mentioned script slot from the table and all of its children. The
  * children are stopped recusively by setting their status to PROC_STATE_FREE and deactivating
  * their resource slot.
  */
static void stop_script_from_table(uint8_t table_idx)
{
  uint8_t slot = vm_state.proc_slot_table[table_idx];
  vm_state.proc_state[slot] = PROC_STATE_FREE;

  //debug_out(" Child script %d ended slot %d", vm_state.proc_script_or_object_id[slot], slot);
  res_deactivate_slot(proc_res_slot[slot]);
  for (++table_idx; table_idx < vm_state.num_active_proc_slots; ++table_idx)
  {
    // stop children of us
    uint8_t child_slot = vm_state.proc_slot_table[table_idx];
    if (child_slot != 0xff && vm_state.proc_parent[child_slot] == slot) {
      stop_script_from_table(table_idx);
    }
  }
}

static void proc_slot_table_insert(uint8_t slot)
{
  int8_t new_entry = proc_slot_table_idx + 1;

  for (int8_t i = vm_state.num_active_proc_slots - 1; i >= new_entry; --i) {
    vm_state.proc_slot_table[i + 1] = vm_state.proc_slot_table[i];
  }

  vm_state.proc_slot_table[new_entry] = slot;
  ++vm_state.num_active_proc_slots;
}

/**
  * @brief Reads a byte from the script.
  *
  * Reads the byte at pc and increments pc by 1.
  * 
  * @return The byte.
  *
  * Code section: code_script
  */
static uint8_t read_byte(void)
{
  uint8_t value;
  value = *pc++;
  return value;
}

/**
  * @brief Reads a 16-bit word from the script.
  *
  * Reads the 16 bit word at pc and increments pc by 2.
  * 
  * @return The 16-bit word.
  *
  * Code section: code_script
  */
static uint16_t read_word(void)
{
  uint16_t value;

  // value = *NEAR_U16_PTR(pc);
  // pc += 2;
  __asm (" ldy #0\n"
         " lda (pc),y\n"
         " sta %0\n"
         " inw pc\n"
         " lda (pc),y\n"
         " sta %0+1\n"
         " inw pc"
         : "=Kzp16" (value)
         :
         : "a", "y");

  return value;
}

/**
  * @brief Reads 24 bits from the script.
  *
  * The value is stored in a 32-bit signed integer.
  * 
  * @return The read value.
  *
  * Code section: code_script
  */
static int32_t read_int24(void)
{
  int32_t value;

  __asm (" ldz #0\n"
         " ldq (pc),z\n"
         " sta %0\n"
         " stx %0+1\n"
         " sty %0+2\n"
         " ldz #0\n"
         " tya\n"
         " bpl positive\n"
         " dez\n"
         "positive:\n"
         " stz %0+3\n"
         " inw pc\n"
         " inw pc\n"
         " inw pc\n"
         : "=Kzp32" (value)
         :
         : "a", "x", "y", "z");

  return value;
}

/**
  * @brief Resolves the next parameter as an 8-bit value.
  *
  * The parameter can be either a variable index or an 8-bit value.
  * If the parameter is a variable index, the value of the variable is
  * truncated to the lower 8 bits.
  * 
  * @return The resolved parameter.
  *
  * Code section: code_script
  */
static uint8_t resolve_next_param8(void)
{
  uint8_t param;
  uint8_t masked_opcode = opcode & param_mask;
  if (masked_opcode) {
    param = vm_read_var8(read_byte());
  }
  else {
    param = read_byte();
  }
  param_mask >>= 1;
  return param;
}

/**
  * @brief Resolves the next parameter as a 16-bit value.
  *
  * The parameter can be either a variable index or a 16-bit value.
  * 
  * @return The resolved parameter.
  *
  * Code section: code_script
  */
static uint16_t resolve_next_param16(void)
{
  uint16_t param;
  uint8_t masked_opcode = opcode & param_mask;
  if (masked_opcode) {
    param = vm_read_var(read_byte());
  }
  else {
    param = read_word();
  }
  param_mask >>= 1;
  return param;
}

/**
  * @brief Reads a null-terminated string from the script.
  *
  * Reads the string at pc and increments pc until a null byte is found.
  * 
  * @param dest The destination buffer where the string will be stored.
  *
  * Code section: code_script
  */
static void read_null_terminated_string(char *dest)
{
  char c;
  while ((c = read_byte())) {
    *dest++ = c;
  }
  *dest = 0;
}

/**
  * @brief Reads a string from the script and decodes it.
  * 
  * In this case, the string is encoding space characters as
  * MSB of the character byte directly before the space.
  *
  * The resulting string in dest is null-terminated.
  *
  * @param dest The destination buffer where the decoded string will be stored.
  *
  * Code section: code_script
  */
static void read_encoded_string_null_terminated(char *dest)
{
  uint8_t num_chars = 0;
  char c;
  while ((c = read_byte())) {
    if (c & 0x80) {
      *dest = c & 0x7f;
      ++dest;
      *dest = ' ';
      ++dest;
    }
    else {
      if (c == 0x04) {
        // special case char for printing integer variable
        int16_t value = vm_read_var(read_byte());
        dest += sprintf(dest, "%d", value);
      }
      else {
        if (c == 0x07) {
          // special case char for printing ASCII character from variable
          c = vm_read_var8(read_byte());
        }
        *dest++ = c;
      }
    }
  }
  *dest = 0;
}

/**
  * @brief Opcode 0x00: stop-script (current one) or break-here
  *
  * Opcodes 0x00 or 0xa0 (doesn't seem to be different) are used to stop 
  * the currently running script (as if using stop-script without parameter
  * in SCUMM).
  *
  * Opcode 0x80 is used to break the script and return to the main loop
  * for one cycle (break-here command in SCUMM). This usually is used to execute a redraw of the screen
  * before continuing the script.
  *
  * Opcode 0x20 is used to stop the music playback.
  *
  * Variant opcodes: 0x20, 0x80, 0xa0
  * 
  * Code section: code_script
  */
static void stop_or_break(void)
{
  if (opcode == 0x80) {
    //debug_scr("break-here");
    break_script = 1;
  }
  else if (opcode == 0x20){
    //debug_scr("stop-music");
  }
  else {
    //debug_scr("stop-script");
    script_stop_slot(active_script_slot);
  }
}

/**
  * @brief Opcode 0x01: put-actor
  *
  * Reads an actor id and two 8-bit values for x and y position. The actor
  * is then positioned at the given position.
  *
  * Variant opcodes: 0x21, 0x61, 0xA1, 0xE1
  *
  * Code section: code_script
  */
static void put_actor(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  //debug_scr("put-actor at %d, %d", x, y);
  actor_place_at(actor_id, x, y);
}

static void start_music(void)
{
  //debug_msg("Start music");
  uint8_t music_id = resolve_next_param8();
  //debug_scr("start-music %d", music_id);
}

static void actor_room(void)
{
  uint8_t var_idx = read_byte();
  uint8_t actor_id = resolve_next_param8();
  //debug_scr("actor-room %d", actor_id);
  vm_write_var(var_idx, actors.room[actor_id]);
}

/**
  * @brief Opcode 0x04: Jump if greater
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * greater than the value, the script will jump to the new pc. The offset is
  * signed two-complement and is a byte position relative to the opcode of the
  * script command following. An offset of 0 would therefore disable the condition
  * completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode
  * is 0x04) or a variable index (if opcode is 0x84).
  *
  * Variant opcodes: 0x84
  *
  * Code section: code_script
  */
static void jump_if_greater(void)
{
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  //debug_scr("if (VAR[%d]=%d <= %d(ind))", var_idx, vm_read_var(var_idx), value);
  if (vm_read_var(var_idx) > value) {
    pc += offset;
  }
}

static void draw_object(void)
{
  //debug_msg("Show object");
  uint16_t obj_id = resolve_next_param16();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();

  if (x != 255 || y != 255) {
    //debug_scr("draw-object %d at %d, %d", obj_id, x, y);
    fatal_error(ERR_NOT_IMPLEMENTED);
  }

  //debug_scr("draw-object %d", obj_id);
  vm_state.global_game_objects[obj_id] |= OBJ_STATE;
  uint8_t local_object_id = vm_get_local_object_id(obj_id);
  if (local_object_id != 0xff) {
    vm_draw_object(local_object_id, x, y);
  }
}

static void assign_array(void)
{
  uint8_t var_idx = read_byte();
  uint8_t array_size = read_byte();
  //debug_scr("assign array at VAR[%d] (size=%d)", var_idx, array_size);
  do {
    uint16_t value = (opcode & 0x80) ? read_word() : read_byte();
    vm_write_var(var_idx, value);
    ++var_idx;
  } 
  while (--array_size);
}

/**
  * @brief Opcode 0x08: Jump if equal
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * equal to the value, the script will jump to the new pc. The offset is
  * signed two-complement and is a byte position relative to the opcode of the
  * script command following. An offset of 0 would therefore disable the
  * condition completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode
  * is 0x08) or a variable index (if opcode is 0x88).
  *
  * Code section: code_script
  */
static void jump_if_equal(void)
{
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  //debug_scr("if (VAR[%d]=%d != %d(ind))", var_idx, vm_read_var(var_idx), value);
  if (vm_read_var(var_idx) == value) {
    pc += offset;
  }
}

static void face_towards(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint16_t object_or_actor_id = resolve_next_param16();
  
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id == 0xff) {
    return;
  }

  uint8_t x1 = actors.x[actor_id];
  uint8_t y1 = actors.y[actor_id];

  uint8_t x2, y2;
  if (vm_get_object_position(object_or_actor_id, &x2, &y2)) {
    uint8_t diff_x = abs8((int8_t)x2 - (int8_t)x1);
    uint8_t diff_y = abs8((int8_t)y2 - (int8_t)y1);
    uint8_t new_dir;
    if (diff_x > diff_y) {
      if (x2 > x1) {
        new_dir = FACING_RIGHT;
      }
      else {
        new_dir = FACING_LEFT;
      }
    }
    else {
      if (y2 > y1) {
        new_dir = FACING_FRONT;
      }
      else {
        new_dir = FACING_BACK;
      }
    }
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    actor_walk_to(actor_id, x1, y1, new_dir);
  }
}

static void assign_variable_indirect(void)
{
  uint8_t var_idx = read_byte();
  var_idx = vm_read_var8(var_idx); // resulting variable is indirect
  uint16_t value = resolve_next_param16();
  vm_write_var(var_idx, value);
}

/**
  * @brief Opcode 0x07: state-of
  * 
  * If the opcode bit 6 is set, the object state is cleared, otherwise it is set.
  * As an object active state is relevant for screen rendering (used for showing/
  * hiding an object), a screen update is requested after the state change.
  *
  * Variant opcodes: 0x47, 0x87, 0xC7
  *
  * Code section: code_script
  */
static void state_of(void)
{
  uint16_t obj_id = resolve_next_param16();
  if (opcode & 0x40) {
    //debug_scr("state-of %d is OFF", obj_id);
    vm_state.global_game_objects[obj_id] &= ~OBJ_STATE;
  }
  else {
    //debug_scr("state-of %d is ON", obj_id);
    vm_state.global_game_objects[obj_id] |= OBJ_STATE;
  }
  if (vm_get_local_object_id(obj_id) != 0xff) {
    vm_update_bg();
    vm_update_actors();
  }
}

/**
  * @brief Opcode 0x0c: Resource cmd
  *
  * A subcode is read and the resource id is resolved. Depending on the subcode,
  * the resource type is determined and the resource is either provided or locked.
  *
  * Providing a resource make sure it is available in memory. If the resource is
  * still available, an unnecessary reload is avoided. Locking a resource makes
  * sure that it is not unloaded from memory as long as it stays locked. 
  *
  * Variant opcodes: 0x8c
  *
  * Code section: code_script
  */
static void resource_cmd(void)
{
  uint8_t resource_id = resolve_next_param8();
  uint8_t sub_opcode = read_byte();

  switch (sub_opcode) {
    case 0x21: {
      //debug_scr("load-costume %d", resource_id);
      uint8_t slot = res_provide(RES_TYPE_COSTUME, resource_id, 0);
      break;
    }
    case 0x22:
      //debug_scr("unlock-costume %d", resource_id);
      res_unlock(RES_TYPE_COSTUME, resource_id, 0);
      break;
    case 0x23:
      //debug_scr("lock-costume %d", resource_id);
      res_lock(RES_TYPE_COSTUME, resource_id, 0);
      break;
    case 0x31:
      //debug_scr("load-room %d", resource_id);
      res_provide(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x32:
      //debug_scr("unlock-room %d", resource_id);
      res_unlock(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x33:
      //debug_scr("lock-room %d", resource_id);
      res_lock(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x51:
      //debug_scr("load-script %d", resource_id);
      res_provide(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x52:
      //debug_scr("unlock-script %d", resource_id);
      res_unlock(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x53:
      //debug_scr("lock-script %d", resource_id);
      res_lock(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x61:
      //debug_scr("load-sound %d", resource_id);
      res_provide(RES_TYPE_SOUND, resource_id, 0);
      break;
    case 0x62:
      //debug_scr("unlock-sound %d", resource_id);
      res_unlock(RES_TYPE_SOUND, resource_id, 0);
      break;
    case 0x63:
      //debug_scr("lock-sound %d", resource_id);
      res_lock(RES_TYPE_SOUND, resource_id, 0);
      break;
    default:
      //debug_out("unknown sub-opcode %x", sub_opcode);
      fatal_error(ERR_UNKNOWN_RESOURCE_OPERATION);
  }
}

static void walk_to_actor(void)
{
  uint8_t actor_id1 = resolve_next_param8();
  uint8_t actor_id2 = resolve_next_param8();
  uint8_t target_distance = read_byte();

  if (actor_id1 >= vm_read_var8(VAR_NUMBER_OF_ACTORS) || 
      actor_id2 >= vm_read_var8(VAR_NUMBER_OF_ACTORS) ||
      actors.room[actor_id1] != vm_read_var8(VAR_SELECTED_ROOM) || 
      actors.room[actor_id1] != actors.room[actor_id2]) {
    return;
  }

  uint8_t x  = actors.x[actor_id2];
  uint8_t y  = actors.y[actor_id2];
  uint8_t cx = actors.x[actor_id1];
  uint8_t cy = actors.y[actor_id1];

  if (cx < x) {
    x -= target_distance;
  }
  else {
    x += target_distance;
  }

  actor_walk_to(actor_id1, x, y, 0xff);
}

static void put_actor_at_object(void)
{
  uint8_t  actor_id  = resolve_next_param8();
  uint16_t object_id = resolve_next_param16();

  uint8_t x, y;

  if (vm_get_object_position(object_id, &x, &y) == 0) {
    x = 30;
    y = 60;
  }
  else {
    walkbox_correct_position_to_closest_box(&x, &y);
  }

  actor_place_at(actor_id, x, y);
}

static void owner_of(void)
{
  uint8_t var_idx = read_byte();
  uint16_t obj_id = resolve_next_param16();
  //debug_scr("owner-of %d is %d", obj_id, vm_state.global_game_objects[obj_id] & 0x0f);
  vm_write_var(var_idx, vm_state.global_game_objects[obj_id] & 0x0f);
}

static void do_animation(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint8_t animation_id = resolve_next_param8();
  //debug_scr("do-animation (actor=)%d (anim=)%d", actor_id, animation_id);
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    if (animation_id < 0xf8) {
      animation_id += actors.dir[actor_id];
    }
    actor_start_animation(local_id, animation_id);
  }
  else {
    if ((animation_id & 0xfc) == 0xf8) {
      actors.dir[actor_id] = animation_id & 0x03;
    }
  }
}

static void camera_pan_to(void)
{
  uint8_t x = resolve_next_param8();
  //debug_scr("camera-pan-to %d", x);
  vm_camera_pan_to(x);
}

/**
  * @brief Opcode 0x13: Actor ops
  * 
  * Used to configure actors. The subcode is read and the actor id is resolved.
  * Depending on the subcode, different actor properties are set.
  *
  * Variant opcodes: 0x53, 0x93, 0xD3
  *
  * Code section: code_script
  */
static void actor_ops(void)
{
  //debug_msg("Actor ops");
  uint8_t actor_id   = resolve_next_param8();
  uint8_t param      = resolve_next_param8();
  uint8_t sub_opcode = read_byte();

  switch (sub_opcode) {
    case 0x01:
      //debug_scr("actor %d sound %d", actor_id, param);
      actors.sound[actor_id] = param;
      break;
    case 0x02:
      actor_map_palette(actor_id, read_byte(), param);
      break;
    case 0x03:
      read_null_terminated_string(actors.name[actor_id]);
      //debug_scr("actor %d name \"%s\"", actor_id, actors.name[actor_id]);
      break;
    case 0x04:
      //debug_scr("actor %d costume %d", actor_id, param);
      actors.costume[actor_id] = param;
      break;
    case 0x05:
      //debug_scr("actor %d talk-color %d", actor_id, param);
      actors.talk_color[actor_id] = param;
      break;
  }
}

/**
  * @brief Opcode 0x14: say-line (or print-line)
  *
  * Outputs a text message to the screen. The actor id is resolved and the
  * dialog string is read/decoded from the script.
  *
  * If the actor id is 0xff, the text is printed as a message from the game
  * with a default color. No actor talking animation will be triggered
  * in that case.
  *
  * The text is displayed on screen immediately. No update screen request is
  * necessary.
  * 
  * Code section: code_script
  */
static void say_line(void)
{
  uint8_t actor_id = resolve_next_param8();
  // check if old message buffer ended with 0x02 (continuation of message)
  uint8_t len = strlen(message_buffer);
  char *msg_ptr;
  if (len > 0 && message_buffer[len - 1] == 0x02) {
    msg_ptr = message_buffer + len - 1;
  }
  else {
    msg_ptr = message_buffer;
  }
  read_encoded_string_null_terminated(msg_ptr);
  if (actor_id == 0xff) {
    //debug_scr("print-line");
  }
  else {
    //debug_scr("say-line");
  }
  vm_say_line(actor_id);
}

static void find_actor(void)
{
  uint8_t var_idx = read_byte();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  //debug_scr("find-actor %d at %d, %d", actor_id, x, y);
  vm_write_var(var_idx, actor_find(x, y));
}

/**
  * @brief Opcode 0x16: random_number
  * 
  * Creates a random number in the range [0..upper_bound] (including the upper_bound).
  * The random number is stored in the variable that is specified by the first parameter.
  *
  * Variant opcodes: 0x96
  *
  * Code section: code_script
  */
static void random_number(void)
{
  uint8_t var_idx = read_byte();
  uint8_t upper_bound = resolve_next_param8();
  //debug_scr("VAR[%d] = random %d", var_idx, upper_bound);
  while (RNDRDY & 0x80); // wait for random number generator to be ready
  uint8_t rnd_number = RNDGEN * (upper_bound + 1) / 256;
  vm_write_var(var_idx, rnd_number);
}

static void set_or_clear_untouchable(void)
{
  uint16_t obj_id = resolve_next_param16();
  if (opcode & 0x40) {
    //debug_scr("class-of %d is UNTOUCHABLE", obj_id);
    vm_state.global_game_objects[obj_id] |= OBJ_CLASS_UNTOUCHABLE;
  }
  else {
    //debug_scr("class-of %d is TOUCHABLE", obj_id);
    vm_state.global_game_objects[obj_id] &= ~OBJ_CLASS_UNTOUCHABLE;
  }
}

/**
  * @brief Opcode 0x18: Jump or restart
  * 
  * Opcode 0x18:
  * Reads a 16 bit offset value and jumps to the new pc. An offset of 0 means
  * that the script will continue with the next opcode. The offset is signed
  * two-complement.
  *
  * Opcide 0x98:
  * Will restart the game from the beginning.
  *
  * Code section: code_script
  */
static void jump_or_restart(void)
{
  if (!(opcode & 0x80)) {
    pc += read_word(); // will effectively jump backwards if the offset is negative
    //debug_scr("jump %x", (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED) - 4));
  }
  else {
    //debug_scr("restart");
    reset_game = RESET_RESTART;
  }
}

/**
  * @brief Opcode 0x1A: Assign variable
  *
  * Assigns a value to a variable. The variable index is read from the script as
  * the first parameter, and the value is read as the next 16-bit value. The value
  * can either be a 16-bit constant value or a variable index (if opcode is 0x9A).
  *
  * Variant opcodes: 0x9A
  *
  * Code section: code_script
  */
static void assign_variable(void)
{
  //debug_scr("assign-variable");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, resolve_next_param16());
}

static void assign_bit_variable(void)
{
  //debug_scr("assign-bit-variable");
  uint16_t bit_var_hi = read_word() + resolve_next_param8();
  uint8_t bit_var_lo = bit_var_hi & 0x0f;
  bit_var_hi >>= 4;
  if (resolve_next_param8()) {
    //debug_scr("set bit variable %x.%x", bit_var_hi, bit_var_lo);
    vm_write_var(bit_var_hi, vm_read_var(bit_var_hi) | (1 << bit_var_lo));
  }
  else {
    //debug_scr("clear bit variable %x.%x", bit_var_hi, bit_var_lo);
    vm_write_var(bit_var_hi, vm_read_var(bit_var_hi) & ~(1 << bit_var_lo));
  }
}

static void start_sound(void)
{
  //debug_scr("start-sound");
  volatile uint8_t sound_id = resolve_next_param8();
}

static void walk_to(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  actor_walk_to(actor_id, x, y, 0xff);
}

static void savegame_operation(void)
{
  char savegame_filename[10];

  uint8_t var_idx        = read_byte();
  uint8_t sub_opcode     = resolve_next_param8();
  uint8_t savegame_slot  = sub_opcode & 0x1f;
  
  sub_opcode &= 0xe0;

  uint8_t result = 0;

  switch (sub_opcode) {

    case 0x00:
      result = 32;
      break;

    case 0x20:
      // print message on screen:
      //  1 = put save disk in drive A
      //  2 = put save disk in drive B
      // >2 = not asking for a disk
      result = 1;
      break;

    case 0x40:
      if (vm_load_game(savegame_slot)) {
        result = 5; // load error
      }
      else {
        result = 3; // load ok
      }
      break;

    case 0x80:
      // save
      if (vm_save_game(savegame_slot)) {
        result = 2; // save error
      }
      // result = 0 -> save ok
      break;
    
    case 0xc0:
      if (vm_savegame_exists(savegame_slot)) {
        result = 6;
      }
      else {
        result = 7;
      }
      break;
    
  }

  vm_write_var(var_idx, result);
}

static void actor_y(void)
{
  uint8_t var_idx = read_byte();
  uint8_t actor_id = resolve_next_param8();
  vm_write_var(var_idx, actors.y[actor_id]);
}

static void come_out_door(void)
{
  SAVE_DS_AUTO_RESTORE

  uint16_t arrive_at_object_id = resolve_next_param16();
  uint8_t  new_room_id         = resolve_next_param8();

  uint8_t walk_to_x = read_byte();
  uint8_t walk_to_y = read_byte();

  uint8_t actor_id = vm_read_var8(VAR_SELECTED_ACTOR);
  
  actor_put_in_room(actor_id, new_room_id);

  // Changing room will break the current script (and terminate it if we are a room script).
  // The current opcode will continue to be executed (this function), but the script
  // will not continue its execution cycle afterwards. Also, the resource of this 
  // script might be deleted from memory already if it was a room script. Therefore,
  // we must not access any further script bytes anymore past this point.
  vm_set_current_room(new_room_id);

  __auto_type obj_hdr = vm_get_room_object_hdr(arrive_at_object_id);
  uint8_t x = obj_hdr->walk_to_x;
  uint8_t y = (obj_hdr->walk_to_y_and_preposition & 0x1f) << 2;
  uint8_t dir = actor_invert_direction(obj_hdr->height_and_actor_dir & 0x03);
  actor_place_at(actor_id, x, y);
  actor_change_direction(actors.local_id[actor_id], dir);
  vm_set_camera_to(actors.x[actor_id]);
  vm_set_camera_follow_actor(actor_id);
  vm_revert_sentence();

  if (walk_to_x != 0xff && walk_to_y != 0xff) {
    actor_walk_to(actor_id, walk_to_x, walk_to_y, 0xff);
  }
}

/**
  * @brief Opcode 0x28: Jump if equal or not equal zero
  *
  * Reads a variable index and compares it to zero. Depending on the opcode, the
  * script will jump to the new pc if the variable is equal to zero (opcode 0x28)
  * or not equal to zero (opcode 0xA8). The offset is signed two-complement and
  * is a byte position relative to the opcode of the script command following.
  * An offset of 0 would therefore disable the condition completely, as practically
  * no jump will occur.
  *
  * Variant opcodes: 0xA8
  *
  * Code section: code_script
  */
static void jump_if_or_if_not_equal_zero(void)
{
  uint8_t var_idx = read_byte();
  int16_t offset = read_word();
  if (opcode & param_mask) {
    //debug_msg("Jump if equal zero");
    if (vm_read_var(var_idx) == 0) {
      pc += offset;
    }
  }
  else {
    //debug_msg("Jump if not equal zero");
    if (vm_read_var(var_idx) != 0) {
      pc += offset;
    }
  }
}

static void set_owner_of(void)
{
  uint16_t obj_id = resolve_next_param16();
  uint8_t owner_id = resolve_next_param8();
  //debug_scr("owner-of %d is %d", obj_id, owner_id);
  vm_state.global_game_objects[obj_id] = (vm_state.global_game_objects[obj_id] & 0xf0) | owner_id;
  vm_update_inventory();
}

/**
  * @brief Opcode 0x2B: sleep-for using value from a variable
  * 
  * Reads a variable index and delays the script execution by the negative value
  * of the variable. The delay is calculated as -1 - variable_value. The timer
  * will count upwards and resume execution of the script once it reaaches 0.
  *
  * Code section: code_script
  */
static void sleep_for_variable(void)
{
  //debug_msg("sleep-for with variable");
  uint8_t var_idx = read_byte();
  int32_t negative_ticks = -1 - (int32_t)vm_read_var(var_idx);
  vm_set_script_wait_timer(negative_ticks);
}

/**
  * @brief Opcode 0x2D: put-actor a in-room r
  *
  * Reads the actor id and the room id from the script and places the actor in
  * the room.
  *
  * Variant opcodes: 0x6D, 0xAD, 0xED
  *
  * Code section: code_script
  */
static void put_actor_in_room(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint8_t room_id = resolve_next_param8();
  actor_put_in_room(actor_id, room_id);
  //debug_scr("put-actor %d in-room %d", actor_id, room_id);
}

/**
  * @brief Opcode 0x2E/0xAE: sleep-for or wait-for-message
  * 
  * If the opcode is 0x2E, the script will pause for a certain amount of time.
  * Reads 24 bits of ticks value for the wait timer. Note that the amount
  * of ticks that the script should pause actually needs to be calculated by
  * ticks_to_wait = 0xffffff - param_value.
  *
  * If the opcode is 0xAE, the script will wait until the current message is
  * processed. If the message is still going, the script will be paused and
  * the pc will be decremented to repeat the wait-for-message command in the
  * next script cycle.
  *
  * Code section: code_script
  */
static void sleep_for_or_wait_for_message(void)
{
  if (!(opcode & 0x80)) {
    //debug_scr("sleep-for");
    int32_t negative_ticks = read_int24();
    vm_set_script_wait_timer(negative_ticks);
  }
  else {
    //debug_scr("wait-for-message");
    if (vm_read_var8(VAR_MESSAGE_GOING)) {
      --pc;
      break_script = 1;
    }
  }
}

static void jump_if_or_if_not_locked(void)
{
  uint16_t obj_id = resolve_next_param16();
  int16_t offset = read_word();

  if (opcode & 0x40) {
    if (!(vm_state.global_game_objects[obj_id] & OBJ_CLASS_LOCKED)) {
      pc += offset;
    }
  }
  else {
    if (vm_state.global_game_objects[obj_id] & OBJ_CLASS_LOCKED) {
      pc += offset;
    }
  }
}

static void set_box(void)
{
  uint8_t box_id = resolve_next_param8();
  uint8_t value  = read_byte();
  debug_out("set-box %d to %d", box_id, value);
  SAVE_DS_AUTO_RESTORE
  map_ds_resource(room_res_slot);
  walk_boxes[box_id].flags = value;
}

/**
  * @brief Opcode 0x31: assign from bit variable
  *
  * Reads a variable index and a 16-bit value. The value is used as a bit variable
  * and the bit is assigned to the variable index. The bit variable is read from
  * the script as the first parameter.
  *
  * Variant opcodes: 0xB1
  *
  * Code section: code_script
  */
static void assign_from_bit_variable(void)
{
  uint8_t var_idx = read_byte();
  uint16_t bit_var_hi = read_word() + resolve_next_param8();
  uint8_t bit_var_lo = bit_var_hi & 0x0f;
  bit_var_hi >>= 4;
  //debug_scr("VAR[%d] = bit-variable %x.%x", var_idx, bit_var_hi, bit_var_lo);
  vm_write_var(var_idx, (vm_read_var(bit_var_hi) >> bit_var_lo) & 1);
}

static void camera_at(void)
{
  uint16_t new_camera_x = resolve_next_param8();
  //debug_scr("camera-at %d", new_camera_x);
  vm_camera_at(new_camera_x);
}

static void proximity(void)
{
  uint8_t var_idx = read_byte();
  uint16_t object1_id = resolve_next_param16();
  uint16_t object2_id = resolve_next_param16();

  vm_write_var(var_idx, vm_calc_proximity(object1_id, object2_id));
}

static void do_sentence(void)
{
  //debug_scr("do-sentence");
  uint8_t sentence_verb = resolve_next_param8();

  if (sentence_verb == 0xfb) {
    //debug_scr("  sentence_verb: %d = RESET", sentence_verb);
    vm_revert_sentence();
    return;
  }
  else if (sentence_verb == 0xfc) {
    //debug_scr("  sentence_verb: %d = STOP", sentence_verb);
    sentence_stack.num_entries = 0;
    script_stop(SCRIPT_ID_SENTENCE);
    return;
  }
  else if (sentence_verb == 0xfa) {
    //debug_scr("  unimplemented sentence verb 0xfa, needed only in Zak?");
    fatal_error(ERR_UNKNOWN_VERB);
  }

  uint16_t sentence_noun1 = resolve_next_param16();
  uint16_t sentence_noun2 = resolve_next_param16();

  uint8_t sub_opcode = read_byte();
  switch (sub_opcode) {
  case 0:
    // put sentence into cmd stack
    //debug_msg("  put sentence into sentence stack");
    //debug_out("  verb %d noun1 %d noun2 %d", sentence_verb, sentence_noun1, sentence_noun2);
    if (sentence_stack.num_entries == CMD_STACK_SIZE) {
      fatal_error(ERR_CMD_STACK_OVERFLOW);
    }
    sentence_stack.verb[sentence_stack.num_entries]  = sentence_verb;
    sentence_stack.noun1[sentence_stack.num_entries] = sentence_noun1;
    sentence_stack.noun2[sentence_stack.num_entries] = sentence_noun2;
    ++sentence_stack.num_entries;
    //debug_out("  verb %d, noun1 %d, noun2 %d, stacksize %d", sentence_verb, sentence_noun1, sentence_noun2, sentence_stack.num_entries);
    return;

  case 1:
  {
    // execute a sentence directly
    //debug_msg("  execute a sentence directly");

    uint8_t background;
    if (sentence_verb == 0xfd) {
      background = 1;
    }
    else {
      background = 0;
      vm_write_var(VAR_CURRENT_VERB, sentence_verb);
      vm_write_var(VAR_CURRENT_NOUN1, sentence_noun1);
      vm_write_var(VAR_CURRENT_NOUN2, sentence_noun2);
    }
  
    script_execute_object_script(sentence_verb, sentence_noun1, background);
    break;
  }

  case 2:
    debug_msg("  print sentence on screen");
    // prepare a new sentence for printing on screen
    vm_write_var(VAR_SENTENCE_VERB, sentence_verb);
    vm_write_var(VAR_SENTENCE_NOUN1, sentence_noun1);
    vm_write_var(VAR_SENTENCE_NOUN2, sentence_noun2);
    vm_update_sentence();
  }
  
}

static void get_object_at_position(void)
{
  uint8_t var_idx = read_byte();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  uint16_t obj_id = vm_get_object_at(x, y);
  //debug_scr("object-at %d, %d is %d", x, y, obj_id);
  vm_write_var(var_idx, obj_id);
}

static void walk_to_object(void)
{
  //debug_msg("Walk to object");
  uint8_t actor_id = resolve_next_param8();
  uint16_t obj_id = resolve_next_param16();
  actor_walk_to_object(actor_id, obj_id);
}

static void set_or_clear_pickupable(void)
{
  uint16_t obj_id = resolve_next_param16();
  if (opcode & 0x40) {
    //debug_scr("clear-pickupable %d", obj_id);
    vm_state.global_game_objects[obj_id] &= ~OBJ_CLASS_PICKUPABLE;
  }
  else {
    //debug_scr("set-pickupable %d", obj_id);
    vm_state.global_game_objects[obj_id] |= OBJ_CLASS_PICKUPABLE;
  }
}

/**
  * @brief Opcode 0x38: Jump if smaller
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * smaller than the value, the script will jump to the new pc. The offset is
  * signed two-complement and is a byte position relative to the opcode of the
  * script command following. An offset of 0 would therefore disable the
  * condition completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode
  * is 0x38) or a variable index (if opcode is 0xB8).
  *
  * Code section: code_script
  */
static void jump_if_smaller(void)
{
  //debug_msg("Jump if smaller");
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  if (vm_read_var(var_idx) < value) {
    pc += offset;
  }
}

/**
  * @brief Opcode 0x3A: Subtract
  *
  * Subtracts a value from a variable. The value can be either a 16-bit constant
  * or a variable index (if opcode is 0xBA). The variable to modify is read from
  * the script as the first parameter.
  *
  * Opcode 0x3A: VAR(PARAM1) -= PARAM2
  * Opcode 0xBA: VAR(PARAM1) -= VAR(PARAM2)
  *
  * Code section: code_script
  */
static void subtract(void)
{
  //debug_msg("Subtract");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, vm_read_var(var_idx) - resolve_next_param16());
}

static void wait_for_actor(void)
{
  //debug_msg("Wait for actor");
  uint8_t actor_id = resolve_next_param8();
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id == 0xff) {
    return;
  }

  uint8_t walk_state = local_actors.walking[local_id];
  if (walk_state != WALKING_STATE_FINISHED && walk_state != WALKING_STATE_STOPPED) {
    // if actor is still moving, we break the script for one cycle
    // but need to make sure we execute this opcode again
    // so we set the pc back to the opcode
    pc -= 2;
    break_script = 1;
  }
}

static void stop_sound(void)
{
  //debug_msg("Stop sound");
  uint8_t sound_id = resolve_next_param8();
}

static void actor_elevation(void)
{
  uint8_t actor_id  = resolve_next_param8();
  uint8_t elevation = resolve_next_param8();
  // debug_out("act %d elev %d", actor_id, elevation);

  if (actors.elevation[actor_id] != elevation) {
    actors.elevation[actor_id] = elevation;
    if (actors.local_id[actor_id] != 0xff) {
      vm_update_actors();
    }
  }
}

static void jump_if_or_if_not_pickupable(void)
{
  uint16_t obj_id = resolve_next_param16();
  int16_t  offset = read_word();

  if (opcode & 0x40) {
    if (!(vm_state.global_game_objects[obj_id] & OBJ_CLASS_PICKUPABLE)) {
      pc += offset;
    }
  }
  else {
    if (vm_state.global_game_objects[obj_id] & OBJ_CLASS_PICKUPABLE) {
      pc += offset;
    }
  }
}


/**
  * @brief Opcode 0x40: Cutscene
  * 
  * Starts a cutscene. Certain aspects of the game state will be saved so that
  * it can be restored once the cutscene is over (eg. remembering the current room
  * so the scene can be restored after the cutscene).
  *
  * Code section: code_script
  */
static void cut_scene(void)
{
  //debug_msg("cut-scene");
  if (!(opcode & 0x80)) {
    vm_cut_scene_begin();
    vm_revert_sentence();
  }
  else {
    //debug_msg("end of cut-scene");
    vm_cut_scene_end();
  }
}

/**
  * @brief Opcode 0x42: Start script
  * 
  * Starts a new script. The script id is read from the script and the script
  * is started. The script will run in parallel to the current script.
  *
  * Variant opcodes: 0xC2
  *
  * Code section: code_script
  */
static void start_script(void)
{
  //debug_msg("Start script");
  uint8_t script_id = resolve_next_param8();
  //debug_scr("start-script %d", script_id);
  script_start(script_id);
}

static void actor_x(void)
{
  uint8_t var_idx = read_byte();
  uint8_t actor_id = resolve_next_param8();
  //debug_scr("VAR[%d] = actor-x %d", var_idx, actor_id);
  vm_write_var(var_idx, actors.x[actor_id]);
}

/**
  * @brief Opcode 0x44: Jump if smaller or equal
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * smaller or equal to the value, the script will jump to the new pc. The offset
  * is signed two-complement and is a byte position relative to the opcode of the
  * script command following. An offset of 0 would therefore disable the condition
  * completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode
  * is 0x44) or a variable index (if opcode is 0xC4).
  *
  * Variant opcodes: 0xC4
  *
  * Code section: code_script
  */
static void jump_if_smaller_or_equal(void)
{
  //debug_msg("Jump if smaller or equal");
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  if (vm_read_var(var_idx) <= value) {
    pc += offset;
  }
}

/**
  * @brief Opcode 0x46: Increment or decrement
  * 
  * Reads a variable index and increments or decrements the value of the variable.
  * If the opcode bit 7 is set, the variable is decremented, otherwise it is
  * incremented.
  *
  * Variant opcodes: 0xC6
  *
  * Code section: code_script
  */
static void increment_or_decrement(void)
{
  //debug_msg("Increment variable");
  uint8_t var_idx = read_byte();
  uint16_t value = vm_read_var(var_idx);
  if (opcode & 0x80) {
    vm_write_var(var_idx, value - 1);
  }
  else {
    vm_write_var(var_idx, value + 1);
  }
}

/**
  * @brief Opcode 0x48: Jump if not equal
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * not equal to the value, the script will jump to the new pc. The offset is
  * signed two-complement and is a byte position relative to the opcode of the 
  * script command following. An offset of 0 would therefore disable the 
  * condition completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode 
  * is 0x48) or a variable index (if opcode is 0xC8).
  * 
  * Code section: code_script
  */
static void jump_if_not_equal(void)
{
  //debug_msg("Jump if not equal");
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  if (vm_read_var(var_idx) != value) {
    pc += offset;
  }
}

/**
  * @brief Opcode 0x4A: Chain script
  *
  * Reads a script id and chains the script. The current script will be stopped
  * immediately and the new script will be started. The new script will run 
  * instead of the current script in the same script slot. The new script resource
  * will be loaded and the new script will be started. The script resource will 
  * be mapped already at the end of this function and the PC will be set to the
  * beginning of the new script. Therefore, when returning from this function,
  * the new script will be executed from the beginning. The old script resource 
  * is deactivated and marked free to override if needed.
  *
  * Variant opcodes: 0xCA
  *
  * Code section: code_script
  */
void chain_script(void)
{
  //debug_msg("chain-script");
  uint8_t script_id = resolve_next_param8();
  
  //debug_out("Chaining script %d from script %d slot %d", script_id, vm_state.proc_script_or_object_id[active_script_slot], active_script_slot);

  if (script_is_room_object_script(active_script_slot)) {
    fatal_error(ERR_CHAINING_ROOM_SCRIPT);
  }

  res_deactivate_slot(proc_res_slot[active_script_slot]);

  uint8_t new_page = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_activate_slot(new_page);
  map_ds_resource(new_page);

  proc_res_slot[active_script_slot]                     = new_page;
  vm_state.proc_pc[active_script_slot]                  = 4; // skipping script header directly to first opcode
  vm_state.proc_script_or_object_id[active_script_slot] = script_id;
  vm_state.proc_object_id_msb[active_script_slot]       = 0;

  //script_print_slot_table();

  pc = NEAR_U8_PTR(RES_MAPPED) + vm_state.proc_pc[active_script_slot];
}

/**
  * @brief Opcode 0x4F: Jump if object not active
  *
  * Reads an object id and a signed offset. If the object is not active, the script
  * will jump to the new pc. The offset is signed two-complement and is a byte
  * position relative to the opcode of the script command following. An offset of 0
  * would therefore disable the condition completely, as practically no jump will occur.
  *
  * The object active state is relevant for screen rendering (used for showing/hiding
  * an object).
  *
  * Variant opcodes: 0xCF
  *
  * Code section: code_script
  */
static void jump_if_object_active_or_not_active(void)
{
  //debug_msg("Jump if object not active");
  uint16_t obj_id = resolve_next_param16();
  int16_t offset = read_word();
  if (opcode & 0x40) {
    if ((vm_state.global_game_objects[obj_id] & OBJ_STATE) == 0) {
      //debug_scr("jump if object %d not active", obj_id);
      pc += offset;
    }
  }
  else {
    if (vm_state.global_game_objects[obj_id] & OBJ_STATE) {
      //debug_scr("jump if object %d active", obj_id);
      pc += offset;
    }
  }
}

static void pick_up_object(void)
{
  uint16_t obj_id = resolve_next_param16();
  //debug_scr("pick-up-object %d", obj_id);

  if (obj_id == 0xffff) {
    return;
  }

  if (inv_object_available(obj_id)) {
    // already in inventory
    return;
  }

  uint8_t local_object_id = vm_get_local_object_id(obj_id);
  if (local_object_id == 0xff) {
    return;
  }

  inv_add_object(local_object_id);

  uint8_t *game_object = &vm_state.global_game_objects[obj_id];
  *game_object &= 0xf0; // clear object owner
  *game_object |= OBJ_STATE | OBJ_CLASS_UNTOUCHABLE | vm_read_var8(VAR_SELECTED_ACTOR);
  vm_update_bg();
  vm_update_actors();
  vm_update_inventory();
}

static void camera_follows_actor(void)
{
  //debug_msg("Camera follows actor");
  uint8_t actor_id = resolve_next_param8();
  vm_set_camera_follow_actor(actor_id);
}

static void new_name_of(void)
{
  uint16_t obj_id = resolve_next_param16();

  char new_name[32];
  read_null_terminated_string(new_name);

  vm_set_object_name(obj_id, new_name);
  vm_update_inventory();
}

/**
  * @brief Opcode 0x58: Begin override or print ego
  * 
  * Function is called for both opcodes 0x58 and 0xd8 (0x58 | 0x80).
  * Forward to the correct function depending on the opcode.
  *
  * Variant opcodes: 0xd8
  *
  * Code section: code_script
  */
void begin_override_or_say_line_selected_actor(void)
{
  if (!(opcode & 0x80)) {
    begin_override();
  }
  else {
    say_line_selected_actor();
  }
}

/**
  * @brief Opcode 0x58: Begin override
  * 
  * Enables the user to override the currently running cutscene.
  * The goto command that will be executed is directly following, and is stored
  * in the override_pc variable. This is used to jump to the next opcode after
  * the cutscene has been overridden.
  *
  * The begin_override opcode is skipping the goto command that is following it.
  *
  * Code section: code_script
  */
static void begin_override(void)
{
  //debug_msg("Begin override");
  vm_begin_override();
  pc += 3; // Skip the jump command
}

/**
  * @brief Opcode 0xd8 (0x58 | 0x80): Print ego
  * 
  * Code section: code_script
  */
static void say_line_selected_actor(void)
{
  //debug_msg("say-line selected-actor");
  uint8_t actor_id = vm_read_var8(VAR_SELECTED_ACTOR);
  read_encoded_string_null_terminated(message_buffer);
  vm_say_line(actor_id);
}

/**
  * @brief Opcode 0x5A: Add
  * 
  * Adds a value to a variable. The value can be either a 16-bit constant
  * or a variable index (if opcode is 0xDA). The variable to modify is read from
  * the script as the first parameter.
  *
  * Opcode 0x5A: VAR(PARAM1) += PARAM2
  * Opcode 0xDA: VAR(PARAM1) += VAR(PARAM2)
  *
  * Code section: code_script
  */
static void add(void)
{
  //debug_msg("Add");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, vm_read_var(var_idx) + resolve_next_param16());
}

/**
  * @brief Opcode 0x60: Cursor opcode
  * 
  * Reads the cursor state and the state of the interface. The cursor state
  * is stored in VAR_CURSOR_STATE and the interface state is stored in state_iface.
  *
  * The lower 8 bits of the parameter are the cursor state, the upper 8 bits
  * are the interface state. If cursor state is 0, the cursor is not changed.
  *
  * Code section: code_script
  */
static void cursor(void)
{
  uint16_t param = resolve_next_param16();
  if (param & 0xff) {
    vm_write_var(VAR_CURSOR_STATE, param & 0xff);
  }
  vm_change_ui_flags(param >> 8);
  //debug_scr("cursor cursor-state %x ui-flags %x", param & 0xff, param >> 8);
}

static void stop_script(void)
{
  uint8_t script_id = resolve_next_param8();
  //debug_scr("stop-script %d", script_id);
  if (script_id == 0) {
    script_stop_slot(active_script_slot);
  }
  else {
    script_stop(script_id);
  }
}

static void closest_actor(void)
{
  uint8_t var_idx = read_byte();
  uint8_t actor_or_object_id = resolve_next_param16();

  uint8_t cur_actor        = NUM_ACTORS - 1;
  uint8_t closest_actor    = 0xff;
  uint8_t closest_distance = 0xff;
  do {
    if (actors.local_id[cur_actor] != 0xff && cur_actor != actor_or_object_id) {
      uint8_t distance = vm_calc_proximity(actor_or_object_id, cur_actor);
      //debug_out("dist a%d a%d = %d", actor_or_object_id, cur_actor, distance);
      if (distance < closest_distance) {
        closest_distance = distance;
        closest_actor    = cur_actor;
      }
    }
  }
  while (--cur_actor);

  vm_write_var(var_idx, closest_actor);
}

static void lock_or_unlock(void)
{
  uint16_t obj_id = resolve_next_param16();
  if (opcode & 0x40) {
    //debug_scr("unlock-object %d", obj_id);
    vm_state.global_game_objects[obj_id] &= ~OBJ_CLASS_LOCKED;
  }
  else {
    //debug_scr("lock-object %d", obj_id);
    vm_state.global_game_objects[obj_id] |= OBJ_CLASS_LOCKED;
  }
}

static void script_running(void)
{
  uint8_t var_idx = read_byte();
  uint8_t script_id = resolve_next_param8();
  //debug_scr("VAR[%d] = script-running %d", var_idx, script_id);
  if (vm_is_script_running(script_id)) {
    vm_write_var(var_idx, 1);
  }
  else {
    vm_write_var(var_idx, 0);
  }
}

static void preposition(void)
{
  uint8_t var_idx = read_byte();
  uint16_t obj_id = resolve_next_param16();

  SAVE_DS_AUTO_RESTORE
  uint8_t preposition = 0xff;
  struct object_code *obj_hdr = inv_get_object_by_id(obj_id);
  if (obj_hdr == NULL) {
    obj_hdr = vm_get_room_object_hdr(obj_id);
  }

  if (obj_hdr != NULL) {
    preposition = obj_hdr->walk_to_y_and_preposition >> 5;
  }

  vm_write_var(var_idx, preposition);
}

static void lights(void)
{
  uint8_t x = resolve_next_param8();
  uint8_t y = read_byte();
  uint8_t z = read_byte();

  if (!z) {
    //debug_scr("lights are %d", x);
    //debug_out("lights %d", x);
    // if (x == 0) {
    //   x = 12;
    // }
    if (vm_read_var8(VAR_CURRENT_LIGHTS) != x) {
      vm_write_var(VAR_CURRENT_LIGHTS, x);
      vm_update_bg();
      vm_update_actors();
      vm_update_flashlight();
    }
  }
  else if (z == 1) {
    vm_state.flashlight_width  = x;
    vm_state.flashlight_height = y;
    vm_update_flashlight();
    //debug_out("flashlight %d %d", x, y);
  }
}

/**
  * @brief Opcode 0x72: current-room
  *
  * Switches the scene to a new room. The room number is read from the script.
  * The new room is activated immediately and a screen update is requested.
  * 
  * Code section: code_script
  */
static void current_room(void)
{
  uint8_t room_no = resolve_next_param8();
  vm_set_current_room(room_no);
  //debug_scr("current-room %d", room_no);
}

/** 
  * @brief Opcode 0x78: Jump if greater or equal
  *
  * Reads a variable index and a 16-bit value. If the value of the variable is
  * greater or equal to the value, the script will jump to the new pc. The offset
  * is signed two-complement and is a byte position relative to the opcode of the
  * script command following. An offset of 0 would therefore disable the condition
  * completely, as practically no jump will occur.
  * The value to compare with can either be a 16-bit constant value (if opcode
  * is 0x78) or a variable index (if opcode is 0xF8).
  *
  * Variant opcodes: 0xF8
  *
  * Code section: code_script
  */
static void jump_if_greater_or_equal(void)
{
  //debug_msg("Jump if greater or equal");
  uint8_t var_idx = read_byte();
  uint16_t value = resolve_next_param16();
  int16_t offset = read_word();
  if (vm_read_var(var_idx) >= value) {
    pc += offset;
  }
}

static void verb(void)
{
  uint8_t verb_id = read_byte();

  if (!verb_id) {
    // delete verb
    uint8_t slot = resolve_next_param8();
    //debug_scr("delete-verb %d", slot);
    vm_verb_delete(slot);
    return;
  }
  
  if (verb_id == 0xff) {
    fatal_error(ERR_UNKNOWN_VERB);
  }

  // add verb
  uint8_t x = read_byte();
  uint8_t y = read_byte();
  uint8_t slot = resolve_next_param8();
  pc += 1; // skip next byte, seems to be unused
  char name[80];
  read_null_terminated_string(name);
  vm_verb_new(slot, verb_id, x, y, name);
}

static void sound_running(void)
{
  uint8_t var_idx = read_byte();
  uint8_t sound_id = resolve_next_param8();
  vm_write_var(var_idx, 0);
}

/**
  * @brief Error handler for unimplemented opcodes.
  * 
  * Code section: code_script
  */
static void unimplemented_opcode(void)
{
  debug_out("Unimplemented opcode: %x at %x", opcode, (uint16_t)(pc - 1));
  VICIV.bordercol = 4;
  fatal_error(ERR_UNKNOWN_OPCODE);
}
