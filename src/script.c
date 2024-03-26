#include "script.h"
#include "error.h"
#include "io.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

//----------------------------------------------------------------------

// private functions
static void exec_opcode(uint8_t opcode);
static uint8_t read_byte(void);
static uint16_t read_word(void);
static uint32_t read_24bits(void);
static uint8_t resolve_next_param8(void);
static uint16_t resolve_next_param16(void);
static void read_null_terminated_string(char *dest);
static void stop_or_break(void);
static void obj_state_active(void);
static void resource_cmd(void);
static void actor_ops(void);
static void print(void);
static void create_random_number(void);
static void jump(void);
static void assign(void);
static void delay_variable(void);
static void subtract(void);
static void add(void);
static void delay(void);
static void cutscene(void);
static void start_script(void);
static void jump_if_not_equal(void);
static void begin_override_or_print_ego(void);
static void begin_override(void);
static void cursor_cmd(void);
static void load_room(void);
static void print_ego(void);
static void unimplemented_opcode(void);

//----------------------------------------------------------------------

#pragma clang section bss="zzpage"

// private zeropage variables
static uint8_t __attribute__((zpage)) opcode;
static uint8_t __attribute__((zpage)) param_mask;
static uint8_t * __attribute__((zpage)) pc;

//----------------------------------------------------------------------

#pragma clang section bss="zdata"

// private functions
static void (*opcode_jump_table[128])(void);
static uint8_t *override_pc;
static uint8_t break_script = 0;

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
  opcode_jump_table[0x07] = &obj_state_active;
  opcode_jump_table[0x0c] = &resource_cmd;
  opcode_jump_table[0x13] = &actor_ops;
  opcode_jump_table[0x14] = &print;
  opcode_jump_table[0x16] = &create_random_number;
  opcode_jump_table[0x18] = &jump;
  opcode_jump_table[0x1a] = &assign;
  opcode_jump_table[0x2b] = &delay_variable;
  opcode_jump_table[0x2e] = &delay;
  opcode_jump_table[0x3a] = &subtract;
  opcode_jump_table[0x40] = &cutscene;
  opcode_jump_table[0x42] = &start_script;
  opcode_jump_table[0x47] = &obj_state_active;
  opcode_jump_table[0x48] = &jump_if_not_equal;
  opcode_jump_table[0x53] = &actor_ops;
  opcode_jump_table[0x58] = &begin_override_or_print_ego;
  opcode_jump_table[0x5a] = &add;
  opcode_jump_table[0x60] = &cursor_cmd;
  opcode_jump_table[0x72] = &load_room;
}

/// @} // script_init

//----------------------------------------------------------------------

/**
 * @defgroup script_public Script Public Functions
 * @{
 */
#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

/**
 * @brief Runs the next script cycle of the specified process slot.
 *
 * The script is run from the current pc until the script's state is not PROC_STATE_RUNNING.
 * 
 * @param proc_id The process id of the script to run.
 * @return uint8_t 0 if the script has finished, 1 if the script is still running.
 *
 * Code section: code_main
 */
uint8_t script_run(uint8_t proc_id)
{
  break_script = 0;

  map_ds_resource(proc_res_slot[proc_id]);
  pc = NEAR_U8_PTR(RES_MAPPED) + proc_pc[proc_id];
  while (vm_get_active_proc_state() == PROC_STATE_RUNNING && !(break_script)) {
    //debug_out("pc: %04x", (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED) - 4));
    opcode = read_byte();
    param_mask = 0x80;
    exec_opcode(opcode);
  }

  proc_pc[proc_id] = (uint16_t)(pc - NEAR_U8_PTR(RES_MAPPED));

  return 0;
}

#pragma clang section text="code"

/**
 * @brief Executes the function for the given opcode.
 *
 * We are not doing this code inline but as a separate function
 * because the compiler would otherwise not know about the clobbering
 * of registers (including zp registers) by the called function.
 * Placing the function in section code will make sure that it
 * is not inlined by functions in other sections, like code_main.
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

#pragma clang section text="code_main"

/**
 * @brief Reads a byte from the script.
 *
 * Reads the byte at pc and increments pc by 1.
 * 
 * @return uint8_t The byte.
 *
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * @brief Opcode 0x00: Stop or break
 *
 * Opcode 0x00 is used to stop the currently running script.
 *
 * Opcode 0x80 is used to break the script and return to the main loop
 * for one cycle. This usually is used to execute a redraw of the screen
 * before continuing the script.
 *
 * Variant opcodes: 0x80
 * 
 * Code section: code_main
 */
static void stop_or_break(void)
{
  //debug_msg("Stop or break");
  if (!opcode) {
    vm_stop_active_script();
  }
  else {
    break_script = 1;
  }
}

/**
 * @brief Opcode 0x07: Set or clear active state of an object
 * 
 * If the opcode bit 6 is set, the object state is cleared, otherwise it is set.
 * As an object active state is relevant for screen rendering (used for showing/
 * hiding an object), a screen update is requested after the state change.
 *
 * Variant opcodes: 0x47, 0x87, 0xC7
 *
 * Code section: code_main
 */
static void obj_state_active(void)
{
  //debug_msg("obj state active");
  uint16_t obj_id = resolve_next_param16();
  if (opcode & 0x40) {
    global_game_objects[obj_id] &= ~OBJ_STATE_ACTIVE;
  }
  else {
    global_game_objects[obj_id] |= OBJ_STATE_ACTIVE;
  }
  vm_update_screen();
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
 * Code section: code_main
 */
static void resource_cmd(void)
{
  //debug_msg("Resource cmd");
  uint8_t resource_id = resolve_next_param8();
  uint8_t sub_opcode = read_byte();

  switch (sub_opcode) {
    case 0x31:
      res_provide(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x33:
      res_lock(RES_TYPE_ROOM, resource_id, 0);
      break;
    case 0x51:
      res_provide(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x53:
      res_lock(RES_TYPE_SCRIPT, resource_id, 0);
      break;
    case 0x61:
      res_provide(RES_TYPE_SOUND, resource_id, 0);
      break;
    case 0x63:
      res_lock(RES_TYPE_SOUND, resource_id, 0);
      break;
    default:
      fatal_error(ERR_UNKNOWN_RESOURCE_OPERATION);
  }
}

/**
 * @brief Opcode 0x13: Actor ops
 * 
 * Used to configure actors. The subcode is read and the actor id is resolved.
 * Depending on the subcode, different actor properties are set.
 *
 * Variant opcodes: 0x53, 0x93, 0xD3
 *
 * Code section: code_main
 */
static void actor_ops(void)
{
  //debug_msg("Actor ops");
  uint8_t actor_id   = resolve_next_param8();
  uint8_t param      = resolve_next_param8();
  uint8_t sub_opcode = read_byte();

  switch (sub_opcode) {
    case 0x01:
      actor_sounds[actor_id] = param;
      break;
    case 0x02:
      actor_palette_idx[actor_id] = read_byte();
      actor_palette_colors[actor_id] = param;
      break;
    case 0x03:
      read_null_terminated_string(actor_names[actor_id]);
      break;
    case 0x04:
      actor_costumes[actor_id] = param;
      break;
    case 0x05:
      actor_talk_colors[actor_id] = param;
      break;
  }
}

/**
 * @brief Opcode 0x14: Print
 *
 * Outputs a dialog string to the screen. The actor id is resolved and the
 * dialog string is read/decoded from the script.
 *
 * The text is displayed on screen immediately. No update screen request is
 * necessary.
 * 
 * Code section: code_main
 */
static void print(void)
{
  //debug_msg("Print");
  uint8_t actor_id = resolve_next_param8();
  read_encoded_string_null_terminated(dialog_buffer);
  vm_actor_start_talking(actor_id);
}

/**
 * @brief Opcode 0x16: Create random number
 * 
 * Creates a random number in the range [0..upper_bound] (including the upper_bound).
 * The random number is stored in the variable that is specified by the first parameter.
 *
 * Variant opcodes: 0x96
 *
 * Code section: code_main
 */
static void create_random_number(void)
{
  //debug_msg("Create random number");
  uint8_t var_idx = read_byte();
  uint8_t upper_bound = resolve_next_param8();
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
 * Code section: code_main
 */
static void jump(void)
{
  //debug_msg("Jump");
  pc += read_word(); // will effectively jump backwards if the offset is negative
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
 * Code section: code_main
 */
static void assign(void)
{
  //debug_msg("Assign");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, resolve_next_param16());
}

/**
 * @brief Opcode 0x2B: Delay variable
 * 
 * Reads a variable index and delays the script execution by the negative value
 * of the variable. The delay is calculated as -1 - variable_value. The timer
 * will count upwards and resume execution of the script once it reaaches 0.
 *
 * Code section: code_main
 */
static void delay_variable(void)
{
  //debug_msg("Delay variable");
  uint8_t var_idx = read_byte();
  int32_t negative_ticks = -1 - (int32_t)vm_read_var(var_idx);
  vm_set_script_wait_timer(negative_ticks);
}

/**
 * @brief Opcode 0x2E: Delay
 * 
 * Reads 24 bits of ticks value for the wait timer. Note that the amount
 * of ticks that the script should pause actually needs to be calculated by
 * ticks_to_wait = 0xffffff - param_value.
 *
 * Code section: code_main
 */
static void delay(void)
{
  //debug_msg("Delay");
  int32_t negative_ticks = read_int24();
  vm_set_script_wait_timer(negative_ticks);
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
 * Code section: code_main
 */
static void subtract(void)
{
  //debug_msg("Subtract");
  uint8_t var_idx = read_byte();
  vm_write_var(var_idx, vm_read_var(var_idx) - resolve_next_param16());
}

/**
 * @brief Opcode 0x40: Cutscene
 * 
 * Starts a cutscene. Certain aspects of the game state will be saved so that
 * it can be restored once the cutscene is over (eg. remembering the current room
 * so the scene can be restored after the cutscene).
 *
 * Code section: code_main
 */
static void cutscene(void)
{
  //debug_msg("Cutscene");
  vm_start_cutscene();
  override_pc = NULL;
}

/**
 * @brief Opcode 0x42: Start script
 * 
 * Starts a new script. The script id is read from the script and the script
 * is started. The script will run in parallel to the current script.
 *
 * Variant opcodes: 0xC2
 *
 * Code section: code_main
 */
static void start_script(void)
{
  //debug_msg("Start script");
  uint8_t script_id = resolve_next_param8();
  vm_start_script(script_id);
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
 * Code section: code_main
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
 * @brief Opcode 0x58: Begin override or print ego
 * 
 * Function is called for both opcodes 0x58 and 0xd8 (0x58 | 0x80).
 * Forward to the correct function depending on the opcode.
 *
 * Variant opcodes: 0xd8
 *
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
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
 * Code section: code_main
 */
static void cursor_cmd(void)
{
  //debug_msg("Cursor cmd");
  vm_write_var(VAR_CURSOR_STATE, read_byte());
  state_iface = read_byte();
}

/**
 * @brief Opcode 0x72: Load room
 *
 * Switches the scene to a new room. The room number is read from the script.
 * The new room is activated immediately and a screen update is requested.
 * 
 * Code section: code_main
 */
static void load_room(void)
{
  //debug_msg("Load room");
  uint8_t room_no = read_byte();
  vm_switch_room(room_no);
}

/**
 * @brief Error handler for unimplemented opcodes.
 * 
 * Code section: code_main
 */
static void unimplemented_opcode(void)
{
  debug_out("Unimplemented opcode: %02x at %04x", opcode, (uint16_t)(pc - 1));
  fatal_error(ERR_UNKNOWN_OPCODE);
}
