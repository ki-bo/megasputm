#include "script.h"
#include "error.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

// private functions
static inline uint16_t read_var(uint8_t var);
static inline uint8_t read_var8(uint8_t var);
static inline void write_var(uint8_t var, uint16_t value);
static uint8_t read_byte(void);
static uint16_t read_word(void);
static uint32_t read_24bits(void);
static uint8_t resolve_var_param(void);
static uint8_t resolve_next_param8(void);
static uint16_t resolve_next_param16(void);
static void read_null_terminated_string(char *dest);
static void actor_ops(void);
static void print(void);
static void resource_cmd(void);
static void assign(void);
static void subtract(void);
static void add(void);
static void delay(void);
static void cutscene(void);
static void cursor_cmd(void);
static void load_room(void);
static void print_ego(void);
static void unimplemented_opcode(void);

#pragma clang section bss="zzpage"

static uint8_t __attribute__((zpage)) opcode;
static uint8_t __attribute__((zpage)) param_mask;
static uint8_t * __attribute__((zpage)) pc;

#pragma clang section bss="zdata"

void (*opcode_jump_table[128])(void);

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata"

void script_init(void)
{
  for (uint8_t i = 0; i < 128; i++) {
    opcode_jump_table[i] = &unimplemented_opcode;
  }

  opcode_jump_table[0x0c] = &resource_cmd;
  opcode_jump_table[0x13] = &actor_ops;
  opcode_jump_table[0x14] = &print;
  opcode_jump_table[0x1a] = &assign;
  opcode_jump_table[0x2e] = &delay;
  opcode_jump_table[0x3a] = &subtract;
  opcode_jump_table[0x40] = &cutscene;
  opcode_jump_table[0x53] = &actor_ops;
  opcode_jump_table[0x5a] = &add;
  opcode_jump_table[0x60] = &cursor_cmd;
  opcode_jump_table[0x72] = &load_room;
}

#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

/**
 * @brief Executes the function for the given opcode.
 *
 * We are not doing this code inline but as a separate function
 * because the compiler would otherwise not know about the clobbering
 * of registers (including zp registers) by the called function.
 * 
 * @param opcode The opcode to be called (0-127).
 */
void exec_opcode(uint8_t opcode)
{
  //opcode_jump_table[opcode]();
  __asm volatile(" asl a\n"
                 " tax\n"
                 " jsr (opcode_jump_table, x)"
                 : /* no output operands */
                 : "Ka" (opcode)
                 : "x");
}

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

uint8_t script_run(uint8_t proc_id)
{
  map_ds_resource(proc_res_slot[proc_id]);
  pc = NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS) + proc_pc[proc_id];
  while (vm_get_active_proc_state() == PROC_STATE_RUNNING) {
    opcode = read_byte();
    param_mask = opcode & 0xe0;
    exec_opcode(opcode);
  }

  proc_pc[proc_id] = (uint16_t)(pc - NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS));

  return 0;
}

static inline uint16_t read_var(uint8_t var)
{
  volatile uint16_t value;
  value = variables_lo[var] | (variables_hi[var] << 8);
  return value;
}

static inline uint8_t read_var8(uint8_t var)
{
  return variables_lo[var];
}


static inline void write_var(uint8_t var, uint16_t value)
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

/**
 * @brief Reads a byte from the script.
 *
 * Reads the byte at pc and increments pc by 1.
 * 
 * @return uint8_t The byte.
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

static uint8_t resolve_var_param()
{
  uint8_t param = read_byte();
  if (opcode & 0x80) {
    return read_var8(param);
  }
  else {
    return param;
  }
}

static uint8_t resolve_next_param8(void)
{
  uint8_t param;
  uint8_t masked_opcode = opcode & param_mask;
  if (masked_opcode) {
    param = read_var8(read_byte());
  }
  else {
    param = read_byte();
  }
  param_mask >>= 1;
  return param;
}

static uint16_t resolve_next_param16(void)
{
  uint16_t param;
  uint8_t masked_opcode = opcode & param_mask;
  if (masked_opcode) {
    param = read_var(read_byte());
  }
  else {
    param = read_word();
  }
  param_mask >>= 1;
  return param;
}

static void read_null_terminated_string(char *dest)
{
  char c;
  while ((c = read_byte())) {
    *dest++ = c;
  }
  *dest = 0;
}

static void read_encoded_string_null_terminated(char *dest)
{
  uint8_t num_chars = 0;
  char c;
  while ((c = read_byte())) {
    if (c & 0x80) {
      *dest = c;
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
 * @brief Opcode 0x0c: Resource cmd
 * 
 */
static void resource_cmd(void)
{
  debug_msg("Resource cmd");
  uint8_t resource_id = resolve_next_param8();
  uint8_t sub_opcode = read_byte();

  switch (sub_opcode) {
    case 0x31:
      res_provide(RES_TYPE_ROOM, resource_id, 0);
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
  }
}

/**
 * @brief Opcode 0x13: Actor ops
 * 
 * Variant opcodes: 0x53, 0x93, 0xD3
 */
static void actor_ops(void)
{
  debug_msg("Actor ops");
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

static void print(void)
{
  debug_msg("Print");
  uint8_t actor_id = resolve_next_param8();
  read_encoded_string_null_terminated(sentence);
  vm_actor_start_talking(actor_id);
}

/**
 * @brief Opcode 0x1A: Assign
 *
 * Assigns a value to a variable.
 */
static void assign(void)
{
  debug_msg("Assign");
  uint8_t var_idx = read_byte();
  write_var(var_idx, resolve_next_param16());
}

/**
 * @brief Opcode 0x2E: Delay
 * 
 * Reads 24 bits of ticks value for the wait timer. Note that the amount
 * of ticks that the script should pause actually needs to be calculated by
 * ticks_to_wait = 0xffffff - param_value.
 */
static void delay(void)
{
  debug_msg("Delay");
  int32_t negative_ticks = read_int24();
  vm_set_script_wait_timer(negative_ticks);
}

/**
 * @brief Opcode 0x3A: Subtract
 *
 * Subtracts a value from a variable.
 */
static void subtract(void)
{
  debug_msg("Subtract");
  uint8_t var_idx = read_byte();
  write_var(var_idx, resolve_next_param16() - read_word());
}

static void cutscene(void)
{
  debug_msg("Cutscene");
}

/**
 * @brief Opcode 0x5A: Add
 * 
 */
static void add(void)
{
  debug_msg("Add");
  uint8_t var_idx = read_byte();
  write_var(var_idx, resolve_next_param16() + read_word());
}

static void cursor_cmd(void)
{
  debug_msg("Cursor cmd");
  state_cursor = read_byte();
  state_iface  = read_byte();
}

static void load_room(void)
{
  debug_msg("Load room");
  uint8_t room_no = read_byte();
  uint8_t res_slot = res_provide(RES_TYPE_ROOM, room_no, 0);
  vm_switch_room(res_slot);
}

static void print_ego(void)
{
  debug_msg("Print ego");
}

static void unimplemented_opcode(void)
{
  debug_out("Unimplemented opcode: %02x at %04x", opcode, (uint16_t)(pc - 1));
  fatal_error(ERR_UNKNOWN_OPCODE);
}
