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

uint8_t actor_sounds[NUM_ACTORS];
uint8_t actor_palette_idx[NUM_ACTORS];
uint8_t actor_palette_colors[NUM_ACTORS];
char    actor_names[NUM_ACTORS][ACTOR_NAME_LEN];
uint8_t actor_costumes[NUM_ACTORS];
uint8_t actor_talk_colors[NUM_ACTORS];

uint8_t state_cursor;
uint8_t state_iface;

uint8_t jiffy_counter;
uint8_t proc_state[NUM_SCRIPT_SLOTS];
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint16_t proc_pc[NUM_SCRIPT_SLOTS];

// Private functions
static void load_script(uint8_t script_id);
static uint8_t wait_for_jiffy(void);

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

  wait_for_jiffy(); // this resets the elapsed jiffies timer

  while(1) {
    map_cs_diskio();
    diskio_check_motor_off();
    unmap_cs();
    uint8_t elapsed_jiffies = wait_for_jiffy();

    for (int i = 0; i < NUM_SCRIPT_SLOTS; i++)
    {
      if (proc_state[i] == PROC_STATE_RUNNING)
      {
        script_run(i);
      }
    }
  }
}

void vm_switch_room(uint8_t res_slot)
{
  map_ds_resource(res_slot);
  uint8_t *data_ptr = NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS + 4);
  uint16_t width = *NEAR_U16_PTR(data_ptr);
  data_ptr += 6;
  uint16_t bg_data_offset = *NEAR_U16_PTR(data_ptr);

  map_cs_gfx();
  //gfx_fade_out();
  gfx_decode_bg_image(NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS + bg_data_offset), width);
  unmap_cs();
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

static uint8_t wait_for_jiffy(void)
{
  map_cs_gfx();
  uint8_t num_jiffies_elapsed = gfx_wait_for_jiffy_timer();
  unmap_cs();
  return num_jiffies_elapsed;
}
