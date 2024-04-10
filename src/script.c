#include "script.h"
#include "actor.h"
#include "error.h"
#include "io.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

//----------------------------------------------------------------------

#ifdef DEBUG_SCRIPTS
#define debug_scr(...) debug_out(__VA_ARGS__)
#else
#define debug_scr(...)
#endif

// private functions
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
static void jump_if_greater(void);
static void draw_object(void);
static void jump_if_equal(void);
static void state_of(void);
static void resource_cmd(void);
static void owner_of(void);
static void do_animation(void);
static void camera_pan_to(void);
static void actor_ops(void);
static void say_line(void);
static void random(void);
static void jump(void);
static void execute_command(void);
static void jump_if_not_pickupable(void);
static void assign(void);
static void start_sound(void);
static void walk_to(void);
static void jump_if_or_if_not_equal_zero(void);
static void sleep_for_variable(void);
static void put_actor_in_room(void);
static void subtract(void);
static void wait_for_actor(void);
static void stop_sound(void);
static void add(void);
static void sleep_for(void);
static void set_camera(void);
static void get_object_at_position(void);
static void jump_if_smaller(void);
static void cut_scene(void);
static void start_script(void);
static void get_actor_position_x(void);
static void jump_if_smaller_or_equal(void);
static void increment_or_decrement(void);
static void jump_if_not_equal(void);
static void jump_if_object_not_active(void);
static void camera_follows_actor(void);
static void begin_override_or_print_ego(void);
static void begin_override(void);
static void cursor_cmd(void);
static void is_script_running(void);
static void current_room(void);
static void jump_if_greater_or_equal(void);
static void print_ego(void);
static void unimplemented_opcode(void);

static void reset_command(void);

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
static uint8_t *override_pc;
static uint8_t break_script = 0;
static uint8_t backup_opcode;
static uint8_t backup_param_mask;
static uint8_t *backup_pc;
static uint8_t backup_break_script;

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
  opcode_jump_table[0x04] = &jump_if_greater;
  opcode_jump_table[0x05] = &draw_object;
  opcode_jump_table[0x07] = &state_of;
  opcode_jump_table[0x08] = &jump_if_equal;
  opcode_jump_table[0x0c] = &resource_cmd;
  opcode_jump_table[0x10] = &owner_of;
  opcode_jump_table[0x11] = &do_animation;
  opcode_jump_table[0x12] = &camera_pan_to;
  opcode_jump_table[0x13] = &actor_ops;
  opcode_jump_table[0x14] = &say_line;
  opcode_jump_table[0x16] = &random;
  opcode_jump_table[0x18] = &jump;
  opcode_jump_table[0x19] = &execute_command;
  opcode_jump_table[0x1a] = &assign;
  opcode_jump_table[0x1c] = &start_sound;
  opcode_jump_table[0x1e] = &walk_to;
  opcode_jump_table[0x20] = &stop_or_break;
  opcode_jump_table[0x21] = &put_actor;
  opcode_jump_table[0x25] = &draw_object;
  opcode_jump_table[0x28] = &jump_if_or_if_not_equal_zero;
  opcode_jump_table[0x2b] = &sleep_for_variable;
  opcode_jump_table[0x2d] = &put_actor_in_room;
  opcode_jump_table[0x2e] = &sleep_for;
  opcode_jump_table[0x32] = &set_camera;
  opcode_jump_table[0x3e] = &walk_to;
  opcode_jump_table[0x35] = &get_object_at_position;
  opcode_jump_table[0x38] = &jump_if_smaller;
  opcode_jump_table[0x39] = &execute_command;
  opcode_jump_table[0x3a] = &subtract;
  opcode_jump_table[0x3b] = &wait_for_actor;
  opcode_jump_table[0x3c] = &stop_sound;
  opcode_jump_table[0x40] = &cut_scene;
  opcode_jump_table[0x41] = &put_actor;
  opcode_jump_table[0x42] = &start_script;
  opcode_jump_table[0x43] = &get_actor_position_x;
  opcode_jump_table[0x44] = &jump_if_smaller_or_equal;
  opcode_jump_table[0x45] = &draw_object;
  opcode_jump_table[0x46] = &increment_or_decrement;
  opcode_jump_table[0x47] = &state_of;
  opcode_jump_table[0x48] = &jump_if_not_equal;
  opcode_jump_table[0x4f] = &jump_if_object_not_active;
  opcode_jump_table[0x51] = &do_animation;
  opcode_jump_table[0x52] = &camera_follows_actor;
  opcode_jump_table[0x53] = &actor_ops;
  opcode_jump_table[0x58] = &begin_override_or_print_ego;
  opcode_jump_table[0x59] = &execute_command;
  opcode_jump_table[0x5a] = &add;
  opcode_jump_table[0x5e] = &walk_to;
  opcode_jump_table[0x60] = &cursor_cmd;
  opcode_jump_table[0x61] = &put_actor;
  opcode_jump_table[0x65] = &draw_object;
  opcode_jump_table[0x68] = &is_script_running;
  opcode_jump_table[0x6d] = &put_actor_in_room;
  opcode_jump_table[0x72] = &current_room;
  opcode_jump_table[0x75] = &get_object_at_position;
  opcode_jump_table[0x78] = &jump_if_greater_or_equal;
  opcode_jump_table[0x79] = &execute_command;
  opcode_jump_table[0x7e] = &walk_to;
  opcode_jump_table[0x7f] = &jump_if_not_pickupable;
}

/// @} // script_init

//----------------------------------------------------------------------

/**
 * @defgroup script_public Script Public Functions
 * @{
 */
#pragma clang section text="code_script" rodata="cdata_script" data="data_script" bss="zdata"

/**
 * @brief Runs the next script cycle of the currently active script slot.
 *
 * The script is run from the current pc until the script's state is not PROC_STATE_RUNNING.
 * 
 * Code section: code_script
 */
void script_run_active_slot(void)
{
  if (parallel_script_count == 6) {
    fatal_error(ERR_SCRIPT_RECURSION);
  }

  ++parallel_script_count;

  break_script = 0;

  map_ds_resource(proc_res_slot[active_script_slot]);
  pc = NEAR_U8_PTR(RES_MAPPED) + proc_pc[active_script_slot];
  while (vm_get_active_proc_state() == PROC_STATE_RUNNING && !(break_script)) {
    opcode = read_byte();
    param_mask = 0x80;
#ifdef DEBUG_SCRIPTS
    debug_out2("[%02d](%03x) %02x ", active_script_slot, (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED) - 5), opcode);
#endif
    exec_opcode(opcode);
  }

  proc_pc[active_script_slot] = (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED));
  --parallel_script_count;
}

void script_run_slot_stacked(uint8_t slot)
{
  uint8_t  stack_opcode       = opcode;
  uint8_t  stack_param_mask   = param_mask;
  uint8_t *stack_pc           = pc;
  uint8_t  stack_break_script = break_script;
  uint8_t  stack_active_slot  = active_script_slot;

  active_script_slot = slot;
  script_run_active_slot();

  opcode = stack_opcode;
  param_mask = stack_param_mask;
  pc = stack_pc;
  break_script = stack_break_script;
  active_script_slot = stack_active_slot;
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
 * @brief Reads a byte from the script.
 *
 * Reads the byte at pc and increments pc by 1.
 * 
 * @return uint8_t The byte.
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
 * @return uint16_t The 16-bit word.
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
 * @return int32_t The read value.
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
 * @return uint8_t The resolved parameter.
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
 * @return uint16_t The resolved parameter.
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
      *dest = c;
      ++dest;
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
    debug_scr("break-here");
    break_script = 1;
  }
  else if (opcode == 0x20){
    debug_scr("stop-music");
  }
  else {
    debug_scr("stop-script");
    vm_stop_active_script();
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
  debug_scr("put-actor at %d, %d", x, y);
  actors.x[actor_id] = x;
  actors.y[actor_id] = y;
}

static void start_music(void)
{
  //debug_msg("Start music");
  uint8_t music_id = resolve_next_param8();
  debug_scr("start-music %d", music_id);
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
  debug_scr("if (VAR[%d]=%d <= %d(ind))", var_idx, vm_read_var(var_idx), value);
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
    debug_scr("draw-object %d at %d, %d", obj_id, x, y);
    fatal_error(ERR_NOT_IMPLEMENTED);
  }

  debug_scr("draw-object %d", obj_id);
  global_game_objects[obj_id] |= OBJ_STATE;
  vm_clear_all_other_object_states(obj_id);

  vm_update_bg();
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
  debug_scr("if (VAR[%d]=%d != %d(ind))", var_idx, vm_read_var(var_idx), value);
  if (vm_read_var(var_idx) == value) {
    pc += offset;
  }
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
    debug_scr("state-of %d is OFF", obj_id);
    global_game_objects[obj_id] &= ~OBJ_STATE;
  }
  else {
    debug_scr("state-of %d is ON", obj_id);
    global_game_objects[obj_id] |= OBJ_STATE;
  }
  vm_update_bg();
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
    case 0x21:
      debug_scr("load-costume %d", resource_id);
      uint8_t slot = res_provide(RES_TYPE_COSTUME, resource_id, 0);
      break;
    case 0x23:
      debug_scr("lock-costume %d", resource_id);
      res_lock(RES_TYPE_COSTUME, resource_id, 0);
      break;
    case 0x31:
      debug_scr("load-room %d", resource_id);
      res_provide(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x33:
      debug_scr("lock-room %d", resource_id);
      res_lock(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x51:
      debug_scr("load-script %d", resource_id);
      res_provide(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x53:
      debug_scr("lock-script %d", resource_id);
      res_lock(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x61:
      debug_scr("load-sound %d", resource_id);
      res_provide(RES_TYPE_SOUND, resource_id, 0);
      break;
    case 0x63:
      debug_scr("lock-sound %d", resource_id);
      res_lock(RES_TYPE_SOUND, resource_id, 0);
      break;
    default:
      fatal_error(ERR_UNKNOWN_RESOURCE_OPERATION);
  }
}

static void owner_of(void)
{
  uint8_t var_idx = read_byte();
  uint16_t obj_id = resolve_next_param16();
  debug_scr("owner-of %d is %d", obj_id, global_game_objects[obj_id] & 0x0f);
  vm_write_var(var_idx, global_game_objects[obj_id] & 0x0f);
}

static void do_animation(void)
{
  uint8_t actor_id = resolve_next_param8();
  uint8_t animation_id = resolve_next_param8();
  debug_scr("do-animation (actor=)%d (anim=)%d", actor_id, animation_id);
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    actor_start_animation(local_id, animation_id);
  }
}

static void camera_pan_to(void)
{
  uint8_t x = resolve_next_param8();
  debug_scr("camera-pan-to %d", x);
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
      debug_scr("actor %d sound %d", actor_id, param);
      actors.sound[actor_id] = param;
      break;
    case 0x02:
      actors.palette_color[actor_id] = param;
      actors.palette_idx[actor_id] = read_byte();
      debug_scr("actor %d color %d is %d", actor_id, param, actors.palette_idx[actor_id]);
      break;
    case 0x03:
      read_null_terminated_string(actors.name[actor_id]);
      debug_scr("actor %d name \"%s\"", actor_id, actors.name[actor_id]);
      break;
    case 0x04:
      debug_scr("actor %d costume %d", actor_id, param);
      actors.costume[actor_id] = param;
      break;
    case 0x05:
      debug_scr("actor %d talk-color %d", actor_id, param);
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
  read_encoded_string_null_terminated(message_buffer);
  if (actor_id == 0xff) {
    debug_scr("print-line");
  }
  else {
    debug_scr("say-line %d");
  }
  vm_say_line(actor_id);
}

/**
 * @brief Opcode 0x16: random
 * 
 * Creates a random number in the range [0..upper_bound] (including the upper_bound).
 * The random number is stored in the variable that is specified by the first parameter.
 *
 * Variant opcodes: 0x96
 *
 * Code section: code_script
 */
static void random(void)
{
  uint8_t var_idx = read_byte();
  uint8_t upper_bound = resolve_next_param8();
  debug_scr("VAR[%d] = random %d", var_idx, upper_bound);
  while (RNDRDY & 0x80); // wait for random number generator to be ready
  uint8_t rnd_number = RNDGEN * (upper_bound + 1) / 255;
  vm_write_var(var_idx, rnd_number);
}

/**
 * @brief Opcode 0x18: Jump
 * 
 * Reads a 16 bit offset value and jumps to the new pc. An offset of 0 means
 * that the script will continue with the next opcode. The offset is signed
 * two-complement.
 *
 * Code section: code_script
 */
static void jump(void)
{
  pc += read_word(); // will effectively jump backwards if the offset is negative
  debug_scr("jump %03x", (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED) - 5));
}

/**
 * @brief Opcode 0x1A: Assign
 *
 * Assigns a value to a variable. The variable index is read from the script as
 * the first parameter, and the value is read as the next 16-bit value. The value
 * can either be a 16-bit constant value or a variable index (if opcode is 0x9A).
 *
 * Variant opcodes: 0x9A
 *
 * Code section: code_script
 */
static void assign(void)
{
  //debug_msg("Assign");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, resolve_next_param16());
}

static void start_sound(void)
{
  //debug_msg("Start sound");
  volatile uint8_t sound_id = resolve_next_param8();
}

static void walk_to(void)
{
  //debug_msg("Actor walk to");
  uint8_t actor_id = resolve_next_param8();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  actor_walk_to(actor_id, x, y);
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
  debug_scr("put-actor %d in-room %d", actor_id, room_id);
}

/**
 * @brief Opcode 0x2E: sleep-for
 * 
 * Reads 24 bits of ticks value for the wait timer. Note that the amount
 * of ticks that the script should pause actually needs to be calculated by
 * ticks_to_wait = 0xffffff - param_value.
 *
 * Code section: code_script
 */
static void sleep_for(void)
{
  //debug_msg("sleep-for");
  int32_t negative_ticks = read_int24();
  vm_set_script_wait_timer(negative_ticks);
}

static void set_camera(void)
{
  //debug_msg("Set camera");
  uint16_t new_camera_x = resolve_next_param8();
  if (new_camera_x != camera_x) {
    camera_x = new_camera_x;
    debug_scr("Camera x: %d", camera_x);
    vm_update_bg();
    vm_update_actors();
  }
}

static void execute_command(void)
{
  //debug_msg("Execute command");
  uint8_t command_verb = resolve_next_param8();

  if (command_verb == 0xfb) {
    debug_scr("  command_verb: %d = RESET", command_verb);
    reset_command();
    return;
  }
  else if (command_verb == 0xfc) {
    debug_scr("  command_verb: %d = STOP", command_verb);
    vm_stop_script(SCRIPT_ID_COMMAND);
    return;
  }
  else if (command_verb == 0xfd) {
    debug_scr("  unimplemented command verb 0xfd, needed for load/save");
    fatal_error(ERR_UNKNOWN_VERB);
  }

  uint16_t command_noun1 = resolve_next_param16();
  uint16_t command_noun2 = resolve_next_param16();

  uint8_t sub_opcode = read_byte();
  switch (sub_opcode) {
  case 0:
    // put command into cmd stack
    debug_msg("  put command into cmd stack");
    if (cmd_stack.num_entries == CMD_STACK_SIZE) {
      fatal_error(ERR_CMD_STACK_OVERFLOW);
    }
    cmd_stack.verb[cmd_stack.num_entries] = command_verb;
    cmd_stack.noun1[cmd_stack.num_entries] = command_noun1;
    cmd_stack.noun2[cmd_stack.num_entries] = command_noun2;
    ++cmd_stack.num_entries;
    debug_scr("  verb %d, noun1 %d, noun2 %d, stacksize %d", command_verb, command_noun1, command_noun2, cmd_stack.num_entries);
    return;

  case 1:
    // execute a command directly
    debug_msg("  execute a command directly");
    vm_write_var(VAR_COMMAND_VERB, command_verb);
    vm_write_var(VAR_COMMAND_NOUN1, command_noun1);
    vm_write_var(VAR_COMMAND_NOUN2, command_noun2);
    vm_start_object_script(command_verb, command_noun1);
    break;

  case 2:
    //debug_msg("  print sentence on screen");
    // prepare a new sentence for printing on screen
    vm_write_var(VAR_SENTENCE_VERB, command_verb);
    vm_write_var(VAR_SENTENCE_NOUN1, command_noun1);
    vm_write_var(VAR_SENTENCE_NOUN2, command_noun2);
  }
  
}

static void get_object_at_position(void)
{
  //debug_msg("Get object at position");
  uint8_t var_idx = read_byte();
  uint8_t x = resolve_next_param8();
  uint8_t y = resolve_next_param8();
  uint16_t obj_id = vm_get_object_at(x, y);
  vm_write_var(var_idx, obj_id);
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

  if (local_actors.walking[local_id]) {
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
    override_pc = NULL;
    reset_command();
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
  vm_start_script(script_id);
}

static void get_actor_position_x(void)
{
  //debug_msg("Assign actor position x");
  uint8_t var_idx = read_byte();
  uint8_t actor_id = resolve_next_param8();
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
static void jump_if_object_not_active(void)
{
  //debug_msg("Jump if object not active");
  uint16_t obj_id = resolve_next_param16();
  int16_t offset = read_word();
  if ((global_game_objects[obj_id] & OBJ_STATE) == 0) {
    pc += offset;
  }
}

static void camera_follows_actor(void)
{
  //debug_msg("Camera follows actor");
  uint8_t actor_id = resolve_next_param8();
  vm_set_camera_follow_actor(actor_id);
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
void begin_override_or_print_ego(void)
{
  if (!(opcode & 0x80)) {
    begin_override();
  }
  else {
    print_ego();
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
  override_pc = pc;
  pc += 3; // Skip the jump command
}

/**
 * @brief Opcode 0xd8 (0x58 | 0x80): Print ego
 * 
 * Code section: code_script
 */
static void print_ego(void)
{
  //debug_msg("Print ego");
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
 * @brief Opcode 0x60: Cursor cmd
 * 
 * Reads the cursor state and the state of the interface. The cursor state
 * is stored in VAR_CURSOR_STATE and the interface state is stored in state_iface.
 *
 * Code section: code_script
 */
static void cursor_cmd(void)
{
  //debug_msg("Cursor cmd");
  vm_write_var(VAR_CURSOR_STATE, read_byte());
  state_iface = read_byte();
}

static void is_script_running(void)
{
  //debug_msg("Is script running");
  uint8_t var_idx = read_byte();
  uint8_t script_id = resolve_next_param8();
  if (vm_is_script_running(script_id)) {
    vm_write_var(var_idx, 1);
  }
  else {
    vm_write_var(var_idx, 0);
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
  //debug_msg("Load room");
  uint8_t room_no = read_byte();
  vm_set_current_room(room_no);
  debug_out("Switched to room %d", room_no);
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

/**
 * @brief Opcode 0x7f: Jump if not pickupable
 *
 * Reads an object id and a signed offset. If the object can not be picked up,
 * the script will jump to the new pc. The offset is signed two-complement and
 * is a byte position relative to the opcode of the script command following.
 * An offset of 0 would therefore disable the condition completely, as practically
 * no jump will occur.
 *
 * Variant opcodes: 0xFF
 *
 * Code section: code_script
 */
static void jump_if_not_pickupable(void)
{
  //debug_msg("Jump if can not pick up object");
  uint16_t obj_id = resolve_next_param16();
  int16_t offset = read_word();
  if ((global_game_objects[obj_id] & OBJ_CLASS_PICKUPABLE) == 0) {
    pc += offset;
  }
}

/**
 * @brief Error handler for unimplemented opcodes.
 * 
 * Code section: code_script
 */
static void unimplemented_opcode(void)
{
  debug_out("Unimplemented opcode: %02x at %04x", opcode, (uint16_t)(pc - 1));
  fatal_error(ERR_UNKNOWN_OPCODE);
}

static void reset_command(void)
{
  //debug_msg("Reset command");
  vm_write_var(VAR_SENTENCE_VERB, vm_read_var(VAR_DEFAULT_VERB));
  vm_write_var(VAR_SENTENCE_NOUN1, 0);
  vm_write_var(VAR_SENTENCE_NOUN2, 0);
  vm_write_var(VAR_SENTENCE_PREPOSITION, 0);
}
