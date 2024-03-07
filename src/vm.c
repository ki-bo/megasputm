#include "vm.h"
#include "diskio.h"
#include "error.h"
#include "gfx.h"
#include "map.h"
#include "resource.h"
#include "script.h"
#include "util.h"

#pragma clang section bss="zdata"

enum {
  PROC_STATE_FREE,
  PROC_STATE_RUNNING,
  PROC_STATE_WAITING,
  PROC_STATE_DEAD
};

uint8_t global_game_objects[780];
uint8_t variables_lo[256];
uint8_t variables_hi[256];
uint8_t jiffy_counter;
uint8_t proc_state[NUM_SCRIPT_SLOTS];
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint16_t proc_pc[NUM_SCRIPT_SLOTS];

// Private functions
static void load_script(uint8_t script_id);

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

void vm_init(void)
{
  for (int i = 0; i < NUM_SCRIPT_SLOTS; i++)
  {
    proc_state[i] = PROC_STATE_FREE;
  }
}

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"
__task void vm_mainloop(void)
{
  // We will never return, so reset the stack pointer to the top of the stack
  __asm(" ldx #0xff\n"
        " txs"
        : /* no output operands */
        : /* no input operands */
        : "x" /* clobber list */);

  load_script(1);

  while(1) {
    map_cs_diskio();
    diskio_check_motor_off();
    unmap_cs();

    for (int i = 0; i < NUM_SCRIPT_SLOTS; i++)
    {
      if (proc_state[i] == PROC_STATE_RUNNING)
      {
        script_run(i);
      }
    }
  }
}

static void load_script(uint8_t script_id)
{
  uint8_t slot = 0;
  for (int i = 0; i < NUM_SCRIPT_SLOTS; i++)
  {
    if (proc_state[i] == PROC_STATE_FREE)
    {
      slot = i;
      proc_state[i] = PROC_STATE_RUNNING;
      proc_pc[i] = 4; // skipping script header directly to first opcode
      break;
    }
  }

  if (slot == NUM_SCRIPT_SLOTS)
  {
    fatal_error(ERR_OUT_OF_SCRIPT_SLOTS);
  }

  res_provide(RES_TYPE_SCRIPT | RES_LOCKED_MASK, script_id, 0);
}

uint8_t vm_wait_for_jiffy(void)
{
  uint8_t num_jiffies_elapsed = wait_for_jiffy_timer();
  return num_jiffies_elapsed;
}
