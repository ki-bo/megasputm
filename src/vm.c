#include "vm.h"
#include "diskio.h"
#include "error.h"
#include "gfx.h"
#include "map.h"
#include "resource.h"
#include "script.h"
#include "util.h"

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

uint8_t active_script_slot;
uint8_t jiffy_counter;
uint8_t dialog_speed = 6;
uint16_t dialog_timer;
uint8_t proc_state[NUM_SCRIPT_SLOTS];
uint8_t proc_res_slot[NUM_SCRIPT_SLOTS];
uint16_t proc_pc[NUM_SCRIPT_SLOTS];
int32_t proc_wait_timer[NUM_SCRIPT_SLOTS];

// cutscene backup data
uint8_t cs_room;
uint8_t cs_cursor_state;
uint8_t cs_iface_state;


// Private functions
static void process_dialog(uint8_t jiffies_elapsed);
static void load_script(uint8_t script_id);
static uint8_t wait_for_jiffy(void);

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

void vm_init(void)
{
  for (active_script_slot = 0; active_script_slot < NUM_SCRIPT_SLOTS; ++active_script_slot)
  {
    proc_state[active_script_slot] = PROC_STATE_FREE;
    proc_wait_timer[active_script_slot] = 0;
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

  map_cs_gfx();
  gfx_start();
  unmap_cs();

  load_script(1);

  wait_for_jiffy(); // this resets the elapsed jiffies timer
  dialog_timer = 0;

  while (1) {
    map_cs_diskio();
    diskio_check_motor_off();
    unmap_cs();
    uint8_t elapsed_jiffies = wait_for_jiffy();

    for (active_script_slot = 0; 
         active_script_slot < NUM_SCRIPT_SLOTS;
         ++active_script_slot)
    {
      if (proc_state[active_script_slot] == PROC_STATE_WAITING)
      {
        proc_wait_timer[active_script_slot] += elapsed_jiffies;
        uint8_t timer_msb = (uint8_t)((uintptr_t)(proc_wait_timer[active_script_slot]) >> 24);
        if (timer_msb == 0)
        {
          proc_state[active_script_slot] = PROC_STATE_RUNNING;
        }

        process_dialog(elapsed_jiffies);
      }
      if (proc_state[active_script_slot] == PROC_STATE_RUNNING)
      {
        script_run(active_script_slot);
      }
    }
  }
}

uint8_t vm_get_active_proc_state(void)
{
  return proc_state[active_script_slot];
}

void vm_switch_room(uint8_t room_no, uint8_t res_slot)
{
  map_ds_resource(res_slot);
  uint8_t *data_ptr = NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS + 4);
  uint16_t width = *NEAR_U16_PTR(data_ptr);
  data_ptr += 6;
  uint16_t bg_data_offset = *NEAR_U16_PTR(data_ptr);

  map_cs_gfx();
  gfx_fade_out();
  gfx_decode_bg_image(NEAR_U8_PTR(DEFAULT_RESOURCE_ADDRESS + bg_data_offset), width);
  gfx_fade_in();
  unmap_cs();

  vm_write_var(VAR_ROOM_NO, room_no);

  // restore respource mapping of running script again
  map_ds_resource(proc_res_slot[active_script_slot]);
}

/**
 * @brief Delays the currently active script
 *
 * Initializes the delay timer for the currently active script. Note that the timer
 * is actually counting up, so the script will provide a negative number of ticks
 * as parameter to this function.
 *
 * The scheduler will suspend execution of the currently active process until the
 * timer becomes positive.
 *
 * The process state will be set to PROC_STATE_WAITING when this function returns.
 * 
 * @param timer_value The negative amount of ticks to suspend
 */
void vm_set_script_wait_timer(int32_t negative_ticks)
{
  proc_state[active_script_slot] = PROC_STATE_WAITING;
  proc_wait_timer[active_script_slot] = negative_ticks;
}

void vm_start_cutscene(void)
{
  cs_room = vm_read_var8(VAR_ROOM_NO);
  cs_cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  cs_iface_state = state_iface;
}

void vm_actor_start_talking(uint8_t actor_id)
{
  actor_talking = actor_id;
  uint8_t talk_color = actor_id != 0xff ? actor_talk_colors[actor_id] : 0x0f;
  map_cs_gfx();
  uint8_t num_chars = gfx_print_dialog(talk_color, dialog_buffer);
  unmap_cs();
  dialog_timer = (uint16_t)num_chars * (uint16_t)dialog_speed;
}

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
    actor_talking = 0xff;
    gfx_print_dialog(0xff, "\0");
  }
}

static void load_script(uint8_t script_id)
{
  uint8_t slot;
  for (slot = 0; slot < NUM_SCRIPT_SLOTS; ++slot)
  {
    if (proc_state[slot] == PROC_STATE_FREE)
    {
      proc_state[slot] = PROC_STATE_RUNNING;
      proc_pc[slot] = 4; // skipping script header directly to first opcode
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
