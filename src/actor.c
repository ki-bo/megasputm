#include "actor.h"
#include "costume.h"
#include "error.h"
#include "gfx.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------------------------

actors_t actors;
local_actors_t local_actors;

static uint16_t            level_pos_x[16];
static uint8_t             level_pos_y[16];
static struct costume_cel *cel_data[16];

//-----------------------------------------------------------------------------------------------

enum {
  WALK_DIR_LEFT,
  WALK_DIR_RIGHT,
  WALK_DIR_DOWN,
  WALK_DIR_UP
};

//-----------------------------------------------------------------------------------------------

// private functions
static uint8_t get_free_local_id(void);
static void activate_costume(uint8_t actor_id);
static void deactivate_costume(uint8_t actor_id);
static uint8_t is_walk_to_done(uint8_t local_id);
static uint8_t is_next_walk_to_point_reached(uint8_t local_id);
static void calculate_next_box_point(uint8_t local_id);
static void calculate_step(uint8_t local_id);
static void add_local_actor(uint8_t actor_id);
static void remove_local_actor(uint8_t actor_id);
static void reset_animation(uint8_t local_id);
static void turn_to_walk_to_direction(uint8_t local_id);
static void turn(uint8_t local_id);
static uint8_t get_next_box(uint8_t cur_box, uint8_t target_box);
static uint8_t get_box_masking(uint8_t box_id);
static uint8_t correct_position_to_closest_box(uint8_t *x, uint8_t *y);
static uint16_t get_corrected_box_position(struct walk_box *box, uint8_t *x, uint8_t *y);
static uint8_t binary_search_xy(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t yc);
static void find_closest_point_on_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t *x, uint8_t *y);
static void find_closest_box_point(uint8_t box_id, uint8_t *px, uint8_t *py);

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup actor_init Actor Init Functions
 * @{
 */
#pragma clang section text="code_init" data="data_init" rodata="cdata_init" bss="bss_init"

void actor_init(void)
{
  for (uint8_t i = 0; i < NUM_ACTORS; ++i) {
    actors.local_id[i] = 0xff;
    if (i < MAX_LOCAL_ACTORS) {
      local_actors.global_id[i] = 0xff;
    }
  }
}

/// @} // actor_init

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup actor_public Actor Public Functions
 * @{
 */
#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"

void actor_put_in_room(uint8_t actor_id, uint8_t room_no)
{
  if (actors.room[actor_id] == room_no) {
    return;
  }

  if (actor_is_in_current_room(actor_id)) {
    remove_local_actor(actor_id);
  }

  actors.room[actor_id] = room_no;
  if (room_no == vm_read_var8(VAR_SELECTED_ROOM)) {
    add_local_actor(actor_id);
  }
}

void actor_room_changed(void)
{
  uint16_t save_ds = map_get_ds();

  uint8_t new_room = vm_read_var8(VAR_SELECTED_ROOM);
  for (uint8_t local_id = 0; local_id < MAX_LOCAL_ACTORS; ++local_id) {
    uint8_t global_id = local_actors.global_id[local_id];
    if (global_id != 0xff && actors.room[global_id] != new_room) {
      remove_local_actor(global_id);
    }
  }

  if (new_room == 0) {
    return;
  }

  uint8_t num_actors = vm_read_var8(VAR_NUMBER_OF_ACTORS);
  for (uint8_t actor_id = 0; actor_id < num_actors; ++actor_id) {
    if (actors.room[actor_id] == new_room && actors.local_id[actor_id] == 0xff) {
      add_local_actor(actor_id);
    }
  }

  map_set_ds(save_ds);
}

void actor_place_at(uint8_t actor_id, uint8_t x, uint8_t y)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    uint8_t cur_box = correct_position_to_closest_box(&x, &y);
    local_actors.walk_to_box[local_id] = cur_box;
    local_actors.cur_box[local_id]     = cur_box;
    local_actors.walk_to_x[local_id]   = x;
    local_actors.walk_to_y[local_id]   = y;
    local_actors.next_x[local_id]      = x;
    local_actors.next_y[local_id]      = y;
    local_actors.x_fraction[local_id]  = 0;
    local_actors.y_fraction[local_id]  = 0;
    local_actors.walk_step_x[local_id] = 0;
    local_actors.walk_step_y[local_id] = 0;
    local_actors.masking[local_id]     = get_box_masking(cur_box);
    local_actors.walking[local_id]     = WALKING_STATE_STOPPED;
    actors.x[actor_id]                 = local_actors.walk_to_x[local_id];
    actors.y[actor_id]                 = local_actors.walk_to_y[local_id];
  
    vm_update_actors();
  }
  else {
    actors.x[actor_id] = x;
    actors.y[actor_id] = y;
  }
  
}

void actor_walk_to(uint8_t actor_id, uint8_t x, uint8_t y)
{
  if (!actor_is_in_current_room(actor_id)) {
    actors.x[actor_id] = x;
    actors.y[actor_id] = y;
    return;
  }

  uint8_t cur_box     = correct_position_to_closest_box(&(actors.x[actor_id]), &(actors.y[actor_id]));
  uint8_t walk_to_box = correct_position_to_closest_box(&x, &y);

  uint8_t local_id = actors.local_id[actor_id];
  local_actors.x_fraction[local_id]  = 0;
  local_actors.y_fraction[local_id]  = 0;
  local_actors.walk_to_box[local_id] = walk_to_box;
  local_actors.walk_to_x[local_id]   = x;
  local_actors.walk_to_y[local_id]   = y;
  local_actors.cur_box[local_id]     = cur_box;
  local_actors.masking[local_id]     = get_box_masking(cur_box);
  if (cur_box == walk_to_box) {
    local_actors.next_x[local_id] = x;
    local_actors.next_y[local_id] = y;
  }
  else {
    calculate_next_box_point(local_id);
  }
  local_actors.target_dir[local_id] = 0xff;
  if (is_walk_to_done(local_id)) {
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    return;
  }

  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    local_actors.walking[local_id] = WALKING_STATE_STARTING;
  }
  calculate_step(local_id);
}

void actor_walk_to_object(uint8_t actor_id, uint16_t object_id)
{
  uint16_t save_ds = map_get_ds();

  __auto_type object_hdr = vm_get_object_hdr(object_id);
  if (!object_hdr) {
    return;
  }

  uint8_t x = object_hdr->walk_to_x;
  uint8_t y = (object_hdr->walk_to_y & 0x1f) << 2;

  actor_walk_to(actor_id, x, y);

  uint8_t local_id = actors.local_id[actor_id];
  uint8_t actor_dir = object_hdr->height_and_actor_dir & 0x03;
  local_actors.target_dir[local_id] = actor_dir;
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    actor_change_direction(local_id, actor_dir);
  }

  map_set_ds(save_ds);
}

void actor_next_step(uint8_t local_id)
{
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    return;
  }

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t walk_dir = local_actors.walk_dir[local_id];
  if (walk_dir < 2 && actors.x[actor_id] == local_actors.next_x[local_id]) {
    //debug_out("Reached x, correct y %u.%05u to %u", actors.y[actor_id], local_actors.y_fraction[local_id], local_actors.next_y[local_id]);
    actors.y[actor_id] = local_actors.next_y[local_id];
  }
  else if (walk_dir >= 2 && actors.y[actor_id] == local_actors.next_y[local_id]) {
    //debug_out("Reached y, correct x %u.%05u to %u", actors.x[actor_id], local_actors.x_fraction[local_id], local_actors.next_x[local_id]);
    actors.x[actor_id] = local_actors.next_x[local_id];
  }

  if (local_actors.walking[local_id] == WALKING_STATE_STARTING) {
    actor_start_animation(local_id, ANIM_WALKING + actors.dir[actor_id]);
    local_actors.walking[local_id] = WALKING_STATE_CONTINUE;
  }
  else if (local_actors.walk_dir[local_id] != actors.dir[actor_id]) {
    // turn but keep on walking
    turn_to_walk_to_direction(local_id);
  }
  else if (is_walk_to_done(local_id)) {
    uint8_t target_dir = local_actors.target_dir[local_id];
    if (target_dir != 0xff && target_dir != actors.dir[actor_id]) {
      local_actors.walk_dir[local_id] = target_dir;
      turn_to_walk_to_direction(local_id);
    }
    else {
      local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    }
    local_actors.x_fraction[local_id] = 0;
    local_actors.y_fraction[local_id] = 0;
    actor_start_animation(local_id, ANIM_STANDING + actors.dir[actor_id]);
  }
  else {
    uint8_t cur_x = actors.x[actor_id];
    uint8_t cur_y = actors.y[actor_id];
    //debug_out("cur %d, %d next %d, %d", cur_x, cur_y, local_actors.next_x[local_id], local_actors.next_y[local_id]);
    //if (cur_x == local_actors.next_x[local_id] &&
    //    cur_y == local_actors.next_y[local_id]) {
    if (is_next_walk_to_point_reached(local_id)) {
      uint8_t cur_box = local_actors.next_box[local_id];
      uint8_t target_box = local_actors.walk_to_box[local_id];
      local_actors.cur_box[local_id] = cur_box;
      local_actors.masking[local_id] = get_box_masking(cur_box);
      if (cur_box == target_box) {
        local_actors.next_x[local_id] = local_actors.walk_to_x[local_id];
        local_actors.next_y[local_id] = local_actors.walk_to_y[local_id];
      }
      else {
        calculate_next_box_point(local_id);
      }
      calculate_step(local_id);
    }

    int32_t x = actors.x[actor_id] * 0x10000 + local_actors.x_fraction[local_id];
    int32_t y = actors.y[actor_id] * 0x10000 + local_actors.y_fraction[local_id];

    int32_t new_x = x + local_actors.walk_step_x[local_id];
    actors.x[actor_id] = (uint8_t)(new_x >> 16);
    local_actors.x_fraction[local_id] = (uint16_t)new_x;
    int32_t new_y = y + local_actors.walk_step_y[local_id];
    actors.y[actor_id] = (uint8_t)(new_y >> 16);
    local_actors.y_fraction[local_id] = (uint16_t)new_y;
    
    //debug_out("Actor %u position: %u.%05u, %u.%05u", actor_id, actors.x[actor_id], local_actors.x_fraction[local_id], actors.y[actor_id], local_actors.y_fraction[local_id]);
  }

  vm_update_actors();
}

void actor_start_animation(uint8_t local_id, uint8_t animation)
{
  switch (animation & 0xfc) {
    case 0xf8:
      // keep animation but just change direction
      actor_change_direction(local_id, animation & 0x03);
      return;
  }

  __auto_type cel_anim           = local_actors.cel_anim[local_id];
  __auto_type cel_level_cur_cmd  = local_actors.cel_level_cur_cmd[local_id];
  __auto_type cel_level_cmd_ptr  = local_actors.cel_level_cmd_ptr[local_id];
  __auto_type cel_level_last_cmd = local_actors.cel_level_last_cmd[local_id];

  uint16_t save_ds = map_get_ds();
  map_ds_resource(local_actors.res_slot[local_id]);

  __auto_type costume_hdr = (struct costume_header *)RES_MAPPED;
  //debug_out("Local actor %d start animation %d", local_id, animation);
  if (animation >= costume_hdr->num_animations + 1) {
    map_set_ds(save_ds);
    return;
  }
  __auto_type anim_ptr = NEAR_U8_PTR(RES_MAPPED + costume_hdr->animation_offsets[animation]);
  uint16_t cel_level_mask = *((uint16_t *)anim_ptr);
  anim_ptr += 2;
  for (int8_t level = 0; level < 16; ++level) {
    if (cel_level_mask & 0x8000) {
      uint8_t cmd_offset = *anim_ptr++;
      //debug_out("  cel level: %d", level);
      //debug_out("    cmd offset: %02x", cmd_offset);
      if (cmd_offset == 0xff) {
        *cel_level_cur_cmd  = 0xff;
        *cel_level_cmd_ptr  = 0;
        *cel_level_last_cmd = 0;
      }
      else {
        *cel_level_cur_cmd  = 0;
        *cel_level_cmd_ptr  = NEAR_U8_PTR(RES_MAPPED + costume_hdr->animation_commands_offset + cmd_offset);
        *cel_level_last_cmd = *anim_ptr++;
      }
      *cel_anim = animation;
      //debug_out("    cur %d last %d cmd_ptr %04x", *cel_level_cur_cmd, *cel_level_last_cmd, (uint16_t)*cel_level_cmd_ptr);
    }  
    cel_level_mask <<= 1;
    ++cel_anim;
    ++cel_level_cur_cmd;
    ++cel_level_cmd_ptr;
    ++cel_level_last_cmd;
  }

  vm_update_actors();

  map_set_ds(save_ds);
}

void actor_update_animation(uint8_t local_id)
{
  uint8_t redraw_needed = 0;

  map_ds_resource(local_actors.res_slot[local_id]);
  __auto_type cel_level_cur_cmd  = local_actors.cel_level_cur_cmd[local_id];
  __auto_type cel_level_last_cmd = local_actors.cel_level_last_cmd[local_id];
  for (uint8_t level = 0; level < 16; ++level) {
    uint8_t cmd_offset = *cel_level_cur_cmd;
    if (cmd_offset != 0xff) {
      while (1) {
        uint8_t last_cmd_offset = *cel_level_last_cmd;
        if (cmd_offset == (last_cmd_offset & 0x7f)) {
          if (!(last_cmd_offset & 0x80)) {
            //debug_out("  loop to 0");
            *cel_level_cur_cmd = 0;
            if (cmd_offset != 0) {
              redraw_needed = 1;
            }
          }
          break;
        }
        else {
          (*cel_level_cur_cmd)++;
          //debug_out("  advance to next cmd %d", *cel_level_cur_cmd);
          redraw_needed = 1;
          break;
        }
      }
    }
    ++cel_level_cur_cmd;
    ++cel_level_last_cmd;
  }
  
  if (redraw_needed) {
    vm_update_actors();
  }
}

void actor_sort_and_draw_all(void)
{
  uint32_t map_save = map_get();
  map_cs_gfx();
  gfx_reset_actor_drawing();

  // sorting all local actors
  uint8_t sorted_actors[MAX_LOCAL_ACTORS];
  uint8_t num_local_actors = 0;
  for (uint8_t i = 0; i < NUM_ACTORS; ++i) {
    if (actors.local_id[i] != 0xff) {
      sorted_actors[num_local_actors++] = i;
    }
  }

  for (uint8_t i = 0; i < num_local_actors; ++i) {
    for (uint8_t j = 0; j < num_local_actors; ++j) {
      uint8_t actor_id_i = sorted_actors[i];
      uint8_t actor_id_j = sorted_actors[j];
      if (actors.y[actor_id_i] < actors.y[actor_id_j]) {
        uint8_t tmp = sorted_actors[i];
        sorted_actors[i] = sorted_actors[j];
        sorted_actors[j] = tmp;
      }
    }
  }

  // iterate over all sorted actors and draw their current cels on all cel levels
  for (uint8_t i = 0; i < num_local_actors; ++i) {
    uint8_t global_id = sorted_actors[i];
    uint8_t local_id = actors.local_id[global_id];
    actor_draw(local_id);
  }

  gfx_finalize_actor_drawing();
  map_set(map_save);
}

void actor_draw(uint8_t local_id)
{
  uint8_t global_id = local_actors.global_id[local_id];
  
  uint16_t pos_x = actors.x[global_id];
  if (local_actors.x_fraction[local_id] >= 0x8000) { // rounding x position
    ++pos_x;
  }
  pos_x <<= 3; // convert to pixel position

  uint8_t pos_y = actors.y[global_id];
  if (local_actors.y_fraction[local_id] >= 0x8000) { // rounding y position
    ++pos_y;
  }
  pos_y <<= 1; // convert to pixel position

  uint8_t masking = local_actors.masking[local_id];
  int16_t min_x   = 0x7fff;
  uint8_t min_y   = 0xff;
  int16_t max_x   = 0;
  uint8_t max_y   = 0;

  map_ds_resource(local_actors.res_slot[local_id]);
  __auto_type hdr = (struct costume_header *)RES_MAPPED;
  __auto_type cel_level_cmd_ptr = local_actors.cel_level_cmd_ptr[local_id];
  __auto_type cel_level_cur_cmd = local_actors.cel_level_cur_cmd[local_id];
  __auto_type cel_level_table_offset = hdr->level_table_offsets;

  uint8_t mirror = actors.dir[global_id] == 0 && !(hdr->disable_mirroring_and_format & 0x80);
  int16_t dx = -72;
  int16_t dy = -100;

  //debug_out("actor %d local_id %d", global_id, local_id);
  for (uint8_t level = 0; level < 16; ++level) {
    //debug_out("level %d", level);
    
    uint8_t cmd_offset = *cel_level_cur_cmd;
    cel_data[level] = NULL;

    if (cmd_offset != 0xff) {
      uint8_t *cmd_ptr = *cel_level_cmd_ptr;
      uint8_t cmd = cmd_ptr[cmd_offset];

      if (cmd < 0x79) {
        __auto_type cel_ptrs_for_cur_level = NEAR_U16_PTR(RES_MAPPED + *cel_level_table_offset);
        __auto_type cur_cel_data = (struct costume_cel*)(RES_MAPPED + cel_ptrs_for_cur_level[cmd]);
        cel_data[level] = cur_cel_data;

        // calculate scene x position in pixels for this actor cel image
        uint16_t cel_x    = pos_x;
        int16_t  dx_level = dx + cur_cel_data->offset_x;
        if (mirror) {
          cel_x -= dx_level + cur_cel_data->width - 16;
        }
        else {
          cel_x += dx_level + 8;
        }

        // calculate scene y position in pixels for this actor cel image
        int16_t dy_level = dy + cur_cel_data->offset_y;
        uint8_t cel_y    = pos_y + dy_level;
        
        // adjust bounding box for all cels
        min_x = min(min_x, cel_x);
        min_y = min(min_y, cel_y);
        max_x = max(max_x, cel_x + cur_cel_data->width);
        max_y = max(max_y, cel_y + cur_cel_data->height);

        // remember position for this cel level
        level_pos_x[level] = cel_x;
        level_pos_y[level] = cel_y;

        // adjust actor global cel offsets (for offsetting subsequent cels)
        dx += cur_cel_data->move_x;
        dy -= cur_cel_data->move_y;
      }

    }
    ++cel_level_cmd_ptr;
    ++cel_level_cur_cmd;
    ++cel_level_table_offset;
  }

  uint8_t width  = max_x - min_x;
  uint8_t height = max_y - min_y;

  if (!gfx_prepare_actor_drawing(min_x, min_y, width, height)) {
    // actor is outside of screen
    return;
  }

  // pass 2: draw to canvas
  for (uint8_t level = 0; level < 16; ++level) {
    if (cel_data[level] != NULL) {
      uint8_t x = level_pos_x[level] - min_x;
      uint8_t y = level_pos_y[level] - min_y;
      gfx_draw_actor_cel(x, y, cel_data[level], mirror);
    }
  }

  // pass 3: apply masking
  if (masking) {
    gfx_apply_actor_masking(min_x, min_y, masking);
  }
}

void actor_start_talking(uint8_t actor_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    actor_start_animation(local_id, ANIM_TALKING + actors.dir[actor_id]);
  }
}

void actor_stop_talking(uint8_t actor_id)
{
  if (actor_id != 0xff) {
    uint8_t local_id = actors.local_id[actor_id];
    actor_start_animation(local_id, ANIM_MOUTH_SHUT + actors.dir[actor_id]);
  }
  else {
    // stop talking animation for all local actors
    for (uint8_t local_id = 0; local_id < MAX_LOCAL_ACTORS; ++local_id) {
      actor_id = local_actors.global_id[local_id];
      if (actor_id != 0xff) {
        actor_start_animation(local_id, ANIM_MOUTH_SHUT + actors.dir[actor_id]);
      }
    }
  }
}

uint8_t actor_invert_direction(uint8_t dir)
{
  static const uint8_t dir_invert[4] = {1, 0, 3, 2};
  return dir_invert[dir];
}

void actor_change_direction(uint8_t local_id, uint8_t dir)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  actors.dir[actor_id] = dir;

  __auto_type cel_anim = local_actors.cel_anim[local_id];
  for (uint8_t level = 0; level < 16; ++level) {
    if (cel_anim[level] != 0xff) {
      uint8_t cur_anim = cel_anim[level];
      if ((cur_anim & 3) != dir) {
        uint8_t new_anim = (cel_anim[level] & 0xfc) | dir;
        actor_start_animation(local_id, new_anim);
      }
    }
  }
}

/// @} // actor_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup actor_private Actor Private Functions
 * @{
 */

static uint8_t get_free_local_id(void)
{
  for (uint8_t i = 0; i < MAX_LOCAL_ACTORS; i++) {
    if (local_actors.global_id[i] == 0xff) {
      return i;
    }
  }

  fatal_error(ERR_TOO_MANY_LOCAL_ACTORS);
  return 0xff;
}

static void activate_costume(uint8_t actor_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  uint8_t res_slot = res_provide(RES_TYPE_COSTUME, actors.costume[actor_id], 0);
  res_activate_slot(res_slot);
  local_actors.res_slot[local_id] = res_slot;
}

static void deactivate_costume(uint8_t actor_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  res_deactivate_slot(local_actors.res_slot[local_id]);
}

static uint8_t is_walk_to_done(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  return (actors.x[actor_id] == local_actors.walk_to_x[local_id] &&
          actors.y[actor_id] == local_actors.walk_to_y[local_id]);
}

static uint8_t is_next_walk_to_point_reached(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];

  if (labs(local_actors.walk_step_x[local_id]) > 65535 && actors.x[actor_id] == local_actors.next_x[local_id]) {
    return 1;
  }
  if (labs(local_actors.walk_step_y[local_id]) > 65535 && actors.y[actor_id] == local_actors.next_y[local_id]) {
    return 1;
  }
  return 0;
}

static void calculate_next_box_point(uint8_t local_id)
{
  uint16_t save_ds = map_get_ds();
  map_ds_resource(room_res_slot);

  uint8_t cur_box = local_actors.cur_box[local_id];
  uint8_t target_box = local_actors.walk_to_box[local_id];
  uint8_t next_box = get_next_box(cur_box, target_box);

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t cur_x = actors.x[actor_id];
  uint8_t cur_y = actors.y[actor_id];

  //debug_out("Next box %d", next_box);
  find_closest_box_point(next_box, &cur_x, &cur_y);
  //debug_out("Next box point %d, %d", cur_x, cur_y);
  uint8_t next_box_x = cur_x;
  uint8_t next_box_y = cur_y;
  find_closest_box_point(cur_box, &cur_x, &cur_y);
  //debug_out("Current box point %d, %d", cur_x, cur_y);
  if (abs(cur_x - next_box_x) > 1 || abs(cur_y - next_box_y) > 2) {
    find_closest_box_point(next_box, &cur_x, &cur_y);
    next_box_x = cur_x;
    next_box_y = cur_y;
    //debug_out("Corrected next box point %d, %d", cur_x, cur_y);
  }
  local_actors.next_x[local_id] = next_box_x;
  local_actors.next_y[local_id] = next_box_y;
  local_actors.next_box[local_id] = next_box;
  //debug_out("Adapted next box point %d, %d", cur_x, cur_y);

  map_set_ds(save_ds);
}

static void calculate_step(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t x = actors.x[actor_id];
  uint8_t y = actors.y[actor_id];
  uint8_t next_x = local_actors.next_x[local_id];
  uint8_t next_y = local_actors.next_y[local_id];

  // Take the diff between the current position and the walk_to position. Then, calculate the
  // fraction of the diff that the actor will move in this step. The dominant direction is always
  // increasing by 1, and the other direction is increased by the fraction of the diff, so that
  // the actor moves in a straight line from the current position to the walk_to position. The fraction 
  // is used with 16 bits of precision to avoid rounding errors.

  int8_t x_diff = next_x - x;
  int8_t y_diff = next_y - y;
  int32_t x_step = 0;
  int32_t y_step = 0;

  //debug_out("From %d, %d (b%d) to %d, %d (b%d) x_diff %d y_diff %d", x, y, local_actors.cur_box[local_id], next_x, next_y, local_actors.next_box[local_id], x_diff, y_diff);

  if (x_diff == 0 && y_diff == 0) {
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    return;
  }

  int8_t abs_x_diff = abs(x_diff);
  int8_t abs_y_diff = abs(y_diff);

  if (abs_x_diff < abs_y_diff) {
    if (y_diff < 0) {
      //debug_out("  dir 3")
      local_actors.walk_dir[local_id] = 3;
    }
    else {
      //debug_out("  dir 2")
      local_actors.walk_dir[local_id] = 2;
    }
  }
  else {
    if (x_diff < 0) {
      //debug_out("  dir 0")
      local_actors.walk_dir[local_id] = 0;
    }
    else {
      //debug_out("  dir 1")
      local_actors.walk_dir[local_id] = 1;
    }
  }

  if (x_diff == 0) {
    x_step = 0;
    y_step = (y_diff > 0) ? 0x10000 : -0x10000;
  } else if (y_diff == 0) {
    x_step = (x_diff > 0) ? 0x10000 : -0x10000;
    y_step = 0;
  } else {
    if (abs_x_diff > abs_y_diff) {
      x_step = (x_diff < 0) ? -0x10000 : 0x10000;
      y_step = (y_diff * 0x10000) / abs_x_diff;
    } else {
      y_step = (y_diff < 0) ? -0x10000 : 0x10000;
      x_step = (x_diff * 0x10000) / abs_y_diff;
    }
  }

  local_actors.walk_step_x[local_id] = x_step;
  local_actors.walk_step_y[local_id] = y_step;
  debug_out("Actor %d step: %ld, %ld direction: %d", actor_id, x_step, y_step, local_actors.walk_dir[actor_id]);
}

static void add_local_actor(uint8_t actor_id)
{
  uint8_t local_id = get_free_local_id();
  actors.local_id[actor_id] = local_id;
  local_actors.global_id[local_id] = actor_id;
  activate_costume(actor_id);
  //debug_out("Actor %d is now in current room with local id %d", actor_id, local_id);
  uint8_t dir = actors.dir[actor_id];
  reset_animation(local_id);

  actor_place_at(actor_id, actors.x[actor_id], actors.y[actor_id]);
}

static void remove_local_actor(uint8_t actor_id)
{
  deactivate_costume(actor_id);
  local_actors.global_id[actors.local_id[actor_id]] = 0xff;
  actors.local_id[actor_id] = 0xff;
  //debug_out("Actor %d is no longer in current room", actor_id);
}

static void reset_animation(uint8_t local_id)
{
  __auto_type cel_anim          = local_actors.cel_anim[local_id];
  __auto_type cel_level_cur_cmd = local_actors.cel_level_cur_cmd[local_id];
  uint8_t dir = 2;
  actors.dir[local_actors.global_id[local_id]] = dir;

  for (uint8_t i = 0; i < 16; ++i) {
    cel_anim[i]          = 0xff;
    cel_level_cur_cmd[i] = 0xff;
  }
  actor_start_animation(local_id, ANIM_STANDING + dir);
  actor_start_animation(local_id, ANIM_HEAD + dir);
  actor_start_animation(local_id, ANIM_MOUTH_SHUT + dir);
}

static void turn_to_walk_to_direction(uint8_t local_id)
{
  uint8_t actor_id   = local_actors.global_id[local_id];
  uint8_t target_dir = local_actors.walk_dir[local_id];

  if (target_dir == actor_invert_direction(actors.dir[actor_id])) {
    // actor is facing the opposite direction of the target direction
    // use turn to turn around by just 90 degrees
    turn(local_id);
  }
  else {
    // actor is facing the wrong direction, but only 90 degrees off
    actor_change_direction(local_id, target_dir);
  }
}

static void turn(uint8_t local_id)
{
  static const uint8_t turn_dir[4] = {2, 2, 1, 0};
  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t current_dir = actors.dir[actor_id];
  actor_change_direction(local_id, turn_dir[current_dir]);
}

static uint8_t get_next_box(uint8_t cur_box, uint8_t target_box)
{
  __auto_type matrix_row = walk_box_matrix + num_walk_boxes + walk_box_matrix[cur_box];
  __auto_type box_ptr = walk_box_matrix;

  //debug_out("walk_box_matrix %p matrix_row %p", walk_box_matrix, matrix_row);
  uint8_t next_box = matrix_row[target_box];
  return next_box;
}

static uint8_t get_box_masking(uint8_t box_id)
{
  uint16_t save_ds = map_get_ds();
  map_ds_resource(room_res_slot);
  __auto_type box = walk_boxes[box_id];
  uint8_t masking = box.mask;
  map_set_ds(save_ds);
  return masking;
}

static uint8_t correct_position_to_closest_box(uint8_t *x, uint8_t *y)
{
  uint16_t save_ds = map_get_ds();
  map_ds_resource(room_res_slot);

  uint16_t min_distance = 0xffff;
  uint8_t corr_pos_x;
  uint8_t corr_pos_y;
  uint8_t dest_walk_box;

  struct walk_box *walk_box = walk_boxes;

  //debug_out("Correcting for position %d, %d", *x, *y);
  for (uint8_t box_idx = 0; box_idx < num_walk_boxes; ++box_idx) {
    uint8_t walk_box_x = *x;
    uint8_t walk_box_y = *y;
    //debug_out("Checking box %d", box_idx);
    uint16_t distance = get_corrected_box_position(walk_box, &walk_box_x, &walk_box_y);
    //debug_out("  walk_box_x,y: %d, %d distance %d", walk_box_x, walk_box_y, distance);
    if (distance <= min_distance) {
      min_distance = distance;
      corr_pos_x = walk_box_x;
      corr_pos_y = walk_box_y;
      dest_walk_box = box_idx;
    }
    ++walk_box;
  }

  //debug_out("Final corrected position %d, %d walk box %d", corr_pos_x, corr_pos_y, dest_walk_box);

  *x = corr_pos_x;
  *y = corr_pos_y;

  map_set_ds(save_ds);

  return dest_walk_box;
}

static uint16_t get_corrected_box_position(struct walk_box *box, uint8_t *x, uint8_t *y)
{
  uint8_t xc = *x;
  uint8_t yc = *y;
  uint8_t yn;
  uint8_t x_left;
  uint8_t x_right;
  uint8_t y_top = box->top_y;
  uint8_t y_bottom = box->bottom_y;

  if (yc >= y_bottom) {
    //debug_out("  below box");
    yc = y_bottom;
    x_left = box->bottomleft_x;
    x_right = box->bottomright_x;
  }
  else if (yc < y_top) {
    //debug_out("  above box");
    yc = y_top;
    x_left = box->topleft_x;
    x_right = box->topright_x;
  }
  else if (xc < box->topleft_x || xc < box->bottomleft_x) {
    //debug_out("  left of box");
    x_left = binary_search_xy(box->topleft_x, box->bottomleft_x, box->top_y, box->bottom_y, yc);
    x_right = x_left;
  }
  else if (xc > box->topright_x || xc > box->bottomright_x) {
    //debug_out("  right of box");
    x_left = binary_search_xy(box->topright_x, box->bottomright_x, box->top_y, box->bottom_y, yc);
    x_right = x_left;
  }
  else {
    //debug_out("  inside box");
    x_left = box->topleft_x;
    x_right = box->topright_x;
  }

  if (xc < x_left) {
    xc = x_left;
  }
  else if (xc > x_right) {
    xc = x_right;
  }

  //debug_out("  corrected position %d, %d", xc, yc);

  uint8_t diff_x = abs(xc - *x);
  uint8_t diff_y = abs(yc - *y) >> 2;
  //debug_out("  diff %d, %d", diff_x, diff_y);
  if (diff_x < diff_y) {
    diff_x >>= 1;
  }
  else {
    diff_y >>= 1;
  }

  *x = xc;
  *y = yc;

  return diff_x + diff_y;
}

static uint8_t binary_search_xy(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t yc)
{
  uint8_t yn = y1;
  uint8_t xn = x1;
  while (yn != yc) {
    //debug_out("yn %d yc %d y1 %d y2 %d x1 %d x2 %d", yn, yc, y1, y2, x1, x2);
    xn = (uint8_t)(x1 + x2) >> 1;
    if (yn > yc) {
      y2 = yn;
      x2 = xn;
    }
    else {
      y1 = yn;
      x1 = xn;
    }
    yn = (uint8_t)(y1 + y2) >> 1;
  }
  return xn;
}

/**
 * @brief Find the closest point on a line segment to a given point.
 * 
 * This function calculates the closest point on a line segment to a given point. The line segment is
 * defined by two points (x1, y1) and (x2, y2). The given point is (px, py). The closest point on the
 * line segment is calculated and stored in (px, py).
 *
 * If the line is horizontal you need to make sure that x1<=x2. In any case, make sure that y1<=y2.
 * 
 * @param x1 The x-coordinate of the first point on the line segment.
 * @param y1 The y-coordinate of the first point on the line segment.
 * @param x2 The x-coordinate of the second point on the line segment.
 * @param y2 The y-coordinate of the second point on the line segment.
 * @param px The x-coordinate of the given point.
 * @param py The y-coordinate of the given point.
 */
static void find_closest_point_on_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t *px, uint8_t *py)
{
  // Handle special case: vertical line
  if (x1 == x2) {
      *px = x1;
      if (*py < y1) {
          *py = y1;
      } else if (*py > y2) {
          *py = y2;
      }
      // If py is between y1 and y2, it remains unchanged.
      return;
  }

  // Handle special case: horizontal line
  if (y1 == y2) {
      *py = y1;
      if (*px < x1) {
          *px = x1;
      } else if (*px > x2) {
          *px = x2;
      }
      // If px is between x1 and x2, it remains unchanged.
      return;
  }

  // General case: non-vertical, non-horizontal line
  int16_t dx = x2 - x1;
  int16_t dy = y2 - y1;
  int16_t x_diff = *px - x1;
  int16_t y_diff = *py - y1;

  // Compute the dot product and length squared
  int16_t dot = dx * x_diff + dy * y_diff;
  int16_t len_sq = dx * dx + dy * dy;

  // Avoid division by zero
  if (len_sq == 0) {
      *px = x1;
      *py = y1;
      return;
  }

  // Calculate the projection factor t
  int32_t scaled_dot = (int32_t)dot << 8;
  int16_t t = scaled_dot / len_sq;

  // Clamp t to be within the range [0, 1] in fixed-point format
  if (t < 0) t = 0;
  else if (t > 0xff) t = 0xff;

  // Calculate the closest point using the projection factor
  *px = x1 + ((dx * t) >> 8);
  *py = y1 + ((dy * t) >> 8);

  // Ensure the point is clamped within the segment
  if (*px < (x1 < x2 ? x1 : x2)) *px = (x1 < x2 ? x1 : x2);
  if (*px > (x1 > x2 ? x2 : x1)) *px = (x1 > x2 ? x2 : x1);
  if (*py < y1) *py = y1; // y1 is always the smaller y-coordinate
  if (*py > y2) *py = y2; // y2 is always the larger y-coordinate
}

/**
 * @brief Find the closest point on the perimeter of a walk box to a given point.
 * 
 * This function calculates the closest point on the perimeter of a walk box to a given point. The
 * walk box is defined by its id. The given point is (px, py). The closest point on the perimeter of
 * the walk box is calculated and stored in (px, py).
 *
 * Be aware that this function won't work properly if the given point is inside the walk box.
 * 
 * @param box_id The id of the walk box.
 * @param px The x-coordinate of the given point.
 * @param py The y-coordinate of the given point.
 */
static void find_closest_box_point(uint8_t box_id, uint8_t *px, uint8_t *py)
{
  struct walk_box *box = &walk_boxes[box_id];

  //debug_out("Finding closest point on box %d to %d, %d", box_id, *px, *py);
  if (*py <= box->top_y) {
    // above box
    //debug_out("  above box");
    find_closest_point_on_line(box->topleft_x, box->top_y, box->topright_x, box->top_y, px, py);
  }
  else if (*py >= box->bottom_y) {
    // below box
    //debug_out("  below box");
    find_closest_point_on_line(box->bottomleft_x, box->bottom_y, box->bottomright_x, box->bottom_y, px, py);
  }
  else {
    // left of box
    if (*px < box->topright_x && *px < box->bottomright_x) {
      //debug_out("  left of box");
      uint8_t x1 = min(box->topleft_x, box->bottomleft_x);
      uint8_t x2 = max(box->topleft_x, box->bottomleft_x);
      find_closest_point_on_line(x1, box->top_y, x2, box->bottom_y, px, py);
    }
    // right of box
    else {
      //debug_out("  right of box");
      uint8_t x1 = min(box->topright_x, box->bottomright_x);
      uint8_t x2 = max(box->topright_x, box->bottomright_x);
      find_closest_point_on_line(x1, box->top_y, x2, box->bottom_y, px, py);
    }
  }
}

/// @} // actor_private

//-----------------------------------------------------------------------------------------------