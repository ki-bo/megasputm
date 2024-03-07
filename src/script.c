#include "script.h"
#include "error.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

enum Opcodes
{
  OP_ActorOps                = 0x13,
    OP_ActorOps2     = 0x53,
  OP_Print                   = 0x14,
    OP_Print2        = 0x94,
  OP_ResourceCmd             = 0x0C,
  OP_Move                    = 0x1A,
  OP_Delay                   = 0x2E,
  OP_Cutscene                = 0x40,
  OP_CursorCmd               = 0x60,
    OP_CursorCmd2    = 0xE0,
  OP_LoadRoom                = 0x72,
  OP_StopObjectCode          = 0xA0,
  OP_PrintEgo                = 0xD8
};

// private functions
static inline uint16_t read_var(uint8_t var);
static inline void write_var(uint8_t var, uint16_t value);
static uint8_t read_byte(void);
static uint16_t read_word(void);
static void actor_ops(void);
static void print(void);
static void resource_cmd(void);
static void move(void);
static void delay(void);
static void cutscene(void);
static void cursor_cmd(void);
static void load_room(void);
static void print_ego(void);

#pragma clang section bss="zzpage"
static uint8_t __attribute__((zpage)) opcode;
static uint8_t * __attribute__((zpage)) pc;


#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

uint8_t script_run(uint8_t proc_id)
{
  map_ds_resource(proc_res_slot[proc_id]);
  pc = NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS) + proc_pc[proc_id];
  while(1) {
    opcode = read_byte();
    switch(opcode) {
      case OP_ActorOps:
      case OP_ActorOps2:
        actor_ops();
        break;
      case OP_Print:
      case OP_Print2:
        print();
        break;
      case OP_ResourceCmd:
        resource_cmd();
        break;
      case OP_Move:
        move();
        break;
      case OP_Delay:
        delay();
        break;
      case OP_Cutscene:
        cutscene();
        break;
      case OP_CursorCmd:
      case OP_CursorCmd2:
        cursor_cmd();
        break;
      case OP_LoadRoom:
        load_room();
        break;
      case OP_StopObjectCode:
        return SCRIPT_FINISHED;
      case OP_PrintEgo:
        print_ego();
        break;
      default:
        fatal_error(ERR_UNKNOWN_OPCODE);
    }
  }
}

static inline uint16_t read_var(uint8_t var)
{
  uint16_t value;
  __asm(" lda variables_lo, x\n"
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

  __asm(" lda %[val]\n"
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
  __asm(" ldy #0\n"
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

static void move(void)
{
  debug_msg("Move");
  uint8_t target_var;
  if (opcode & 0x80) {
    uint16_t indirect_var_idx = read_word();
    if (MSB(indirect_var_idx)) {
      fatal_error(ERR_VARIDX_OUT_OF_RANGE);
    }
    target_var = read_var(LSB(indirect_var_idx));
  }
  else {
    target_var = read_byte();
  }

  uint16_t value = read_word();

  if ((opcode & 0x60) == 0) {
    write_var(target_var, value);
  }
  else if (opcode & 0x40) {
    value += read_var(target_var);
    write_var(target_var, value);
  }
  else if (opcode & 0x20) {
    value = read_var(target_var) - value;
    write_var(target_var, value);
  }
}

static void delay(void)
{
  debug_msg("Delay");
}

static void cutscene(void)
{
  debug_msg("Cutscene");
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
