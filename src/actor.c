#include "actor.h"
#include "costume.h"
#include "error.h"
#include "gfx.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include "walk_box.h"
#include <stdint.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------------------------

#pragma clang section data="data_main" rodata="cdata_main" bss="zdata"

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
static void stop_walking(uint8_t local_id);
static void calculate_next_box_point(uint8_t local_id);
static void calculate_step(uint8_t local_id);
static void add_local_actor(uint8_t actor_id);
static void remove_local_actor(uint8_t actor_id);
static void reset_animation(uint8_t local_id);
static void turn_to_walk_to_direction(uint8_t local_id);
static void turn(uint8_t local_id);

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
  SAVE_DS_AUTO_RESTORE

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
}

void actor_place_at(uint8_t actor_id, uint8_t x, uint8_t y)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    uint8_t cur_box = walkbox_correct_position_to_closest_box(&x, &y);
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
    local_actors.masking[local_id]     = walkbox_get_box_masking(cur_box);
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

  uint8_t cur_box     = walkbox_correct_position_to_closest_box(&(actors.x[actor_id]), &(actors.y[actor_id]));
  uint8_t walk_to_box = walkbox_correct_position_to_closest_box(&x, &y);

  uint8_t local_id = actors.local_id[actor_id];
  local_actors.x_fraction[local_id]  = 0;
  local_actors.y_fraction[local_id]  = 0;
  local_actors.walk_to_box[local_id] = walk_to_box;
  local_actors.walk_to_x[local_id]   = x;
  local_actors.walk_to_y[local_id]   = y;
  local_actors.cur_box[local_id]     = cur_box;
  local_actors.masking[local_id]     = walkbox_get_box_masking(cur_box);
  if (cur_box == walk_to_box) {
    local_actors.next_x[local_id] = x;
    local_actors.next_y[local_id] = y;
  }
  else {
    calculate_next_box_point(local_id);
  }
  local_actors.target_dir[local_id] = 0xff;
  if (is_walk_to_done(local_id)) {
    stop_walking(local_id);
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    return;
  }

  if (local_actors.walking[local_id] != WALKING_STATE_CONTINUE) {
    local_actors.walking[local_id] = WALKING_STATE_STARTING;
  }
  calculate_step(local_id);
}

void actor_walk_to_object(uint8_t actor_id, uint16_t object_id)
{
  SAVE_DS_AUTO_RESTORE

  if (!actor_is_in_current_room(actor_id)) {
    return;
  }
  __auto_type object_hdr = vm_get_room_object_hdr(object_id);
  if (!object_hdr) {
    return;
  }

  uint8_t x = object_hdr->walk_to_x;
  uint8_t y = (object_hdr->walk_to_y_and_preposition & 0x1f) << 2;

  actor_walk_to(actor_id, x, y);

  uint8_t local_id = actors.local_id[actor_id];
  uint8_t actor_dir = object_hdr->height_and_actor_dir & 0x03;
  local_actors.target_dir[local_id] = actor_dir;
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    actor_change_direction(local_id, actor_dir);
  }
}

void actor_next_step(uint8_t local_id)
{
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    return;
  }

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t walk_dir = local_actors.walk_dir[local_id];
  if (walk_dir < 2 && actors.x[actor_id] == local_actors.next_x[local_id]) {
    //debug_out("Reached x, correct y %u.%u to %u", actors.y[actor_id], local_actors.y_fraction[local_id], local_actors.next_y[local_id]);
    actors.y[actor_id] = local_actors.next_y[local_id];
  }
  else if (walk_dir >= 2 && actors.y[actor_id] == local_actors.next_y[local_id]) {
    //debug_out("Reached y, correct x %u.%u to %u", actors.x[actor_id], local_actors.x_fraction[local_id], local_actors.next_x[local_id]);
    actors.x[actor_id] = local_actors.next_x[local_id];
  }

    // uint8_t cur_x = actors.x[actor_id];
    // uint8_t cur_y = actors.y[actor_id];
    //debug_out("cur %d, %d next %d, %d", cur_x, cur_y, local_actors.next_x[local_id], local_actors.next_y[local_id]);
    //if (cur_x == local_actors.next_x[local_id] &&
    //    cur_y == local_actors.next_y[local_id]) {
  if (is_next_walk_to_point_reached(local_id)) {
    uint8_t cur_box = local_actors.next_box[local_id];
    uint8_t target_box = local_actors.walk_to_box[local_id];
    local_actors.cur_box[local_id] = cur_box;
    local_actors.masking[local_id] = walkbox_get_box_masking(cur_box);
    if (cur_box == target_box) {
      //debug_out("Reached target box %d", cur_box);
      local_actors.next_x[local_id] = local_actors.walk_to_x[local_id];
      local_actors.next_y[local_id] = local_actors.walk_to_y[local_id];
    }
    else {
      calculate_next_box_point(local_id);
    }
    calculate_step(local_id);
  }

  if (is_walk_to_done(local_id)) {
    //debug_out("stop walking");
    stop_walking(local_id);
  }
  else {
    if (local_actors.walking[local_id] == WALKING_STATE_STARTING) {
      //debug_out("Start walking");
      actor_start_animation(local_id, ANIM_WALKING + actors.dir[actor_id]);
      local_actors.walking[local_id] = WALKING_STATE_CONTINUE;
    }
    else if (local_actors.walk_dir[local_id] != actors.dir[actor_id]) {
      //debug_out("Turn");
      // turn but keep on walking
      turn_to_walk_to_direction(local_id);
    }
    
    if (local_actors.walk_dir[local_id] == actors.dir[actor_id]) {
      int32_t x = actors.x[actor_id] * 0x10000 + local_actors.x_fraction[local_id];
      int32_t y = actors.y[actor_id] * 0x10000 + local_actors.y_fraction[local_id];

      int32_t new_x = x + local_actors.walk_step_x[local_id];
      actors.x[actor_id] = (uint8_t)(new_x >> 16);
      local_actors.x_fraction[local_id] = (uint16_t)new_x;
      int32_t new_y = y + local_actors.walk_step_y[local_id];
      actors.y[actor_id] = (uint8_t)(new_y >> 16);
      local_actors.y_fraction[local_id] = (uint16_t)new_y;

      if (is_next_walk_to_point_reached(local_id)) {
        uint8_t cur_box = local_actors.next_box[local_id];
        local_actors.cur_box[local_id] = cur_box;
        local_actors.masking[local_id] = walkbox_get_box_masking(cur_box);
      }
      
      //debug_out("Actor %u position: %u.%u, %u.%u", actor_id, actors.x[actor_id], local_actors.x_fraction[local_id], actors.y[actor_id], local_actors.y_fraction[local_id]);
    }
  }

  vm_update_actors();
}

void actor_start_animation(uint8_t local_id, uint8_t animation)
{
  switch (animation & 0xfc) {
    case 0xf8:
      // keep animation but just change direction
      actor_change_direction(local_id, animation & 0x03);
      local_actors.walking[local_id] = WALKING_STATE_STOPPED;
      return;
    case 0xfc:
      // stop walk-to animation
      local_actors.target_dir[local_id] = 0xff;   // keep direction as is when stopping
      stop_walking(local_id);
      return;
  }

  __auto_type cel_anim           = local_actors.cel_anim[local_id];
  __auto_type cel_level_cur_cmd  = local_actors.cel_level_cur_cmd[local_id];
  __auto_type cel_level_cmd_ptr  = local_actors.cel_level_cmd_ptr[local_id];
  __auto_type cel_level_last_cmd = local_actors.cel_level_last_cmd[local_id];

  SAVE_DS_AUTO_RESTORE
  map_ds_resource(local_actors.res_slot[local_id]);

  __auto_type costume_hdr = (struct costume_header *)RES_MAPPED;
  //debug_out("Local actor %d start animation %d", local_id, animation);
  if (animation >= costume_hdr->num_animations + 1) {
    return;
  }
  __auto_type anim_ptr = NEAR_U8_PTR(RES_MAPPED + costume_hdr->animation_offsets[animation]);
  uint16_t cel_level_mask = *((uint16_t *)anim_ptr);
  anim_ptr += 2;
  for (int8_t level = 0; level < 16; ++level) {
    if (cel_level_mask & 0x8000) {
      uint8_t cmd_offset = *anim_ptr++;
      //debug_out("  cel level: %d", level);
      //debug_out("    cmd offset: %x", cmd_offset);
      //debug_out("    prev anim: %x", *cel_anim);
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
      //debug_out("    cur %d last %d cmd_ptr %x", *cel_level_cur_cmd, *cel_level_last_cmd, (uint16_t)*cel_level_cmd_ptr);
    }  
    cel_level_mask <<= 1;
    ++cel_anim;
    ++cel_level_cur_cmd;
    ++cel_level_cmd_ptr;
    ++cel_level_last_cmd;
  }

  vm_update_actors();
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

/**
 * @brief Draw the actor with the given local id.
 * 
 * This function draws the actor with the given local id to the backbuffer, allocating a canvas
 * of character data for the actor. The actor is drawn at its current position, using the current
 * cel animation for each cel level. The actor is drawn with the current direction and masking.
 *
 * Drawing the actor is done in the following steps:
 * 1. Determine the bounding box for all cels of the actor, relative to the actor's position.
 * 2. Allocate an empty canvas for the actor, if the actor is visible on screen.
 * 3. Draw all cels to the allocated canvas.
 * 4. Apply background and object maskings to the actor canvas.
 * 
 * @param local_id The local id of the actor to draw.
 */
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

  // step 1: determine bounding box, relative position and image pointers for all cels
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

  // step 2: allocate an empty canvas for the actor 
  if (!gfx_prepare_actor_drawing(min_x, min_y, width, height)) {
    // actor is outside of screen
    return;
  }

  // step 3: draw all cels to the allocated canvas
  for (uint8_t level = 0; level < 16; ++level) {
    if (cel_data[level] != NULL) {
      uint8_t x = level_pos_x[level] - min_x;
      uint8_t y = level_pos_y[level] - min_y;
      gfx_draw_actor_cel(x, y, cel_data[level], mirror);
    }
  }

  // step 4: apply background and object maskings to actor canvas
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

  if (actors.x[actor_id] == local_actors.next_x[local_id] &&
      actors.y[actor_id] == local_actors.next_y[local_id]) {
    return 1;
  }

  return 0;
}

static void stop_walking(uint8_t local_id)
{
  uint8_t actor_id   = local_actors.global_id[local_id];
  uint8_t target_dir = local_actors.target_dir[local_id];
  uint8_t walk_state = local_actors.walking[local_id];

  if (walk_state != WALKING_STATE_STOPPING && walk_state != WALKING_STATE_FINISHED) {
    local_actors.walking[local_id] = WALKING_STATE_STOPPING;
    //debug_out(" stopping");
    actor_start_animation(local_id, ANIM_STANDING + actors.dir[actor_id]);
    local_actors.x_fraction[local_id] = 0;
    local_actors.y_fraction[local_id] = 0;
    return;
  }
  if (target_dir != 0xff && target_dir != actors.dir[actor_id]) {
    if (local_actors.walk_dir[local_id] != target_dir) {
      local_actors.walk_dir[local_id] = target_dir;
    }
    //debug_out(" turning to %d", target_dir);
    turn_to_walk_to_direction(local_id);
  }
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPING && 
      (target_dir == 0xff || target_dir == actors.dir[actor_id])) {
    //debug_out(" walking finished");
    local_actors.walking[local_id] = WALKING_STATE_FINISHED;
    return;
  }
  if (local_actors.walking[local_id] == WALKING_STATE_FINISHED) {
    //debug_out(" stopped");
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    actor_start_animation(local_id, ANIM_STANDING + actors.dir[actor_id]);
  }
}

static void calculate_next_box_point(uint8_t local_id)
{
  SAVE_DS_AUTO_RESTORE
  map_ds_resource(room_res_slot);

  uint8_t cur_box = local_actors.cur_box[local_id];
  uint8_t target_box = local_actors.walk_to_box[local_id];
  uint8_t next_box = walkbox_get_next_box(cur_box, target_box);

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t cur_x = actors.x[actor_id];
  uint8_t cur_y = actors.y[actor_id];

  //debug_out("Next box %d", next_box);
  walkbox_find_closest_box_point(next_box, &cur_x, &cur_y);
  //debug_out("Next box point %d, %d", cur_x, cur_y);
  uint8_t next_box_x = cur_x;
  uint8_t next_box_y = cur_y;
  walkbox_find_closest_box_point(cur_box, &cur_x, &cur_y);
  //debug_out("Current box point %d, %d", cur_x, cur_y);
  local_actors.next_x[local_id] = cur_x;
  local_actors.next_y[local_id] = cur_y;
  local_actors.next_box[local_id] = next_box;
  //debug_out("Adapted next box point %d, %d", cur_x, cur_y);
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

  // debug_out("From %d, %d (b%d) to %d, %d (b%d) x_diff %d y_diff %d", x, y, local_actors.cur_box[local_id], next_x, next_y, local_actors.next_box[local_id], x_diff, y_diff);

  if (x_diff == 0 && y_diff == 0) {
    local_actors.walk_step_x[local_id] = 0;
    local_actors.walk_step_y[local_id] = 0;
    return;
  }

  int8_t abs_x_diff = abs(x_diff);
  int8_t abs_y_diff = abs(y_diff);

  if (abs_x_diff < abs_y_diff) {
    if (y_diff < 0) {
      local_actors.walk_dir[local_id] = 3;
    }
    else {
      local_actors.walk_dir[local_id] = 2;
    }
  }
  else {
    if (x_diff < 0) {
      local_actors.walk_dir[local_id] = 0;
    }
    else {
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
  //debug_out("Actor %d step: %ld, %ld direction: %d", actor_id, x_step, y_step, local_actors.walk_dir[actor_id]);
}

static void add_local_actor(uint8_t actor_id)
{
  uint8_t local_id = get_free_local_id();
  actors.local_id[actor_id] = local_id;
  local_actors.global_id[local_id] = actor_id;
  activate_costume(actor_id);
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
  uint8_t dir = actors.dir[local_actors.global_id[local_id]];

  for (uint8_t i = 0; i < 16; ++i) {
    cel_anim[i]          = 0xff;
    cel_level_cur_cmd[i] = 0xff;
  }
  actor_start_animation(local_id, ANIM_STANDING   + dir);
  actor_start_animation(local_id, ANIM_HEAD       + dir);
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

/// @} // actor_private

//-----------------------------------------------------------------------------------------------