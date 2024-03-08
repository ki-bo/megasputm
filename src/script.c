#include "script.h"
#include "error.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

// private functions
static inline uint16_t read_var(uint8_t var);
static inline void write_var(uint8_t var, uint16_t value);
static uint8_t read_byte(void);
static uint16_t read_word(void);
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

#pragma clang section bss="zzpage"

static uint8_t __attribute__((zpage)) opcode;
static uint8_t * __attribute__((zpage)) pc;

#pragma clang section bss="zdata"

void (*opcode_jump_table[128])(void);

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata"

void script_init(void)
{
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
  while(1) {
    opcode = read_byte();
    exec_opcode(opcode);
  }
}

static inline uint16_t read_var(uint8_t var)
{
  volatile uint16_t value;
  // Problem: the sta instructions get optimized away, although we added volatile.
  // If we remove the volatile keyword, then even the whole function will be optimized away...
  __asm volatile(" lda variables_lo, x\n"
                 " sta %0\n"
                 " lda variables_hi, x\n"
                 " sta %0+1\n"
                 : "=Kzp16" (value)
                 : "Kx" (var)
                 : "a");
  return value;
}

static inline void write_var(uint8_t var, uint16_t value)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
  // Need to do this as otherwise those variables are not visible to the inline assembly
  variables_lo;
  variables_hi;
#pragma clang diagnostic pop

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
  __asm volatile(" ldy #0\n"
                 " lda (pc),y\n"
                 " sta %0\n"
                 " inw pc\n"
                 " lda (pc),y\n"
                 " sta %0+1\n"
                 " inw pc"
                 : "=Kzp16" (value)
                 :
                 : "y");

  return value;
}

static uint8_t resolve_var_param()
{
  uint8_t param = read_byte();
  if (opcode & 0x80) {
    return read_var(param);
  }
  else {
    return param;
  }
}

static void actor_ops(void)
{
  debug_msg("Actor ops");
}

static void print(void)
{
  debug_msg("Print");
}

static void resource_cmd(void)
{
  debug_msg("Resource cmd");
}

/**
 * @brief Opcode 0x1A: Assign
 *
 * Assigns a value to a variable.
 */
static void assign(void)
{
  uint8_t var_idx = read_byte();
  if (opcode & 0x80) {
    var_idx = read_var(var_idx);
  }
  write_var(var_idx, read_word());
}

/**
 * @brief Opcode 0x2E: Delay
 * 
 */
static void delay(void)
{
  debug_msg("Delay");
}

/**
 * @brief Opcode 0x3A: Subtract
 *
 * Subtracts a value from a variable.
 */
static void subtract(void)
{
  debug_msg("Subtract");
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
}

static void cursor_cmd(void)
{
  debug_msg("Cursor cmd");
}

static void load_room(void)
{
  debug_msg("Load room");
}

static void print_ego(void)
{
  debug_msg("Print ego");
}
