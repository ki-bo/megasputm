#include "vm.h"
#include "diskio.h"
#include "error.h"
#include "gfx.h"
#include "map.h"
#include "memory.h"

#include "resource.h"
#include "script.h"
#include "util.h"
#include <stdint.h>
#include <string.h>

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

uint8_t active_script_slot;
uint8_t jiffy_counter;
uint8_t dialog_speed = 6;
uint16_t dialog_timer;
uint8_t proc_state[NUM_SCRIPT_SLOTS];
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
uint8_t room_res_slot;
uint8_t num_objects;
uint8_t obj_page[57];
uint8_t obj_offset[57];


// Private functions
static void process_dialog(uint8_t jiffies_elapsed);
static void load_script(uint8_t script_id);
static uint8_t wait_for_jiffy(void);
static void read_object_offsets(void);

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
    uint8_t elapsed_jiffies = wait_for_jiffy();
    map_cs_diskio();
    diskio_check_motor_off(elapsed_jiffies);
    unmap_cs();

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

void vm_switch_room(uint8_t room_no)
{
  // save DS
  uint8_t ds_save = map_get_ds();

  // exit and free old room
  res_deactivate(RES_TYPE_ROOM, vm_read_var8(VAR_ROOM_NO), 0);

  // activate new room data
  room_res_slot = res_provide(RES_TYPE_ROOM, room_no, 0);
  res_set_flags(room_res_slot, RES_ACTIVE_MASK);
  map_ds_resource(room_res_slot);

  __auto_type room_hdr = (struct room_header *)RES_MAPPED;

  map_cs_gfx();
  gfx_fade_out();
  gfx_decode_bg_image(NEAR_U8_PTR(RES_MAPPED + room_hdr->bg_data_offset), 
                      room_hdr->bg_width);
  gfx_fade_in();
  unmap_cs();

  vm_write_var(VAR_ROOM_NO, room_no);

  read_object_offsets();

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
  uint8_t talk_color = actor_id != 0xff ? actor_talk_colors[actor_id] : 0x09;
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

  proc_res_slot[slot] = res_provide(RES_TYPE_SCRIPT, script_id, 0);
  res_set_flags(proc_res_slot[slot], RES_ACTIVE_MASK);
}

static uint8_t wait_for_jiffy(void)
{
  map_cs_gfx();
  uint8_t num_jiffies_elapsed = gfx_wait_for_jiffy_timer();
  unmap_cs();
  return num_jiffies_elapsed;
}

static void read_object_offsets(void)
{
  __auto_type room_hdr = (struct room_header *)RES_MAPPED;
  num_objects = room_hdr->num_objects;
  uint8_t *data_ptr = NEAR_U8_PTR(RES_MAPPED + 0x1c + num_objects * 2);
  for (uint8_t i = 0; i < num_objects; ++i)
  {
    obj_offset[i] = *data_ptr++;
    obj_page[i] = room_res_slot + *data_ptr++;
  }
}
