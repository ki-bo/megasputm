/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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

#pragma clang section data="data_main" rodata="cdata_main" bss="zdata"

actors_t actors;
local_actors_t local_actors;

static uint16_t            level_pos_x[16];
static uint8_t             level_pos_y[16];
static struct costume_cel *cel_data[16];

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
static uint8_t get_target_direction(uint8_t local_id);
static void turn_to_target_direction(uint8_t local_id);
static void turn_to_direction(uint8_t local_id, uint8_t target_dir);
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

/**
  * @brief Map a palette from one index to another for the given actor.
  *
  * This function maps a palette entry for the given actor. A new custom palette is created if the
  * if the actor is not yet using one, as a copy of the default palette for actors. The palette index
  * dest_idx will be overwritten with the default palette entry at src_idx.
  *
  * Actors are using a slightly different palette than the background and object rendering, as index
  * zero needs to stay reserved for the transparent color. The default palette for actors is using
  * index 1 for black, so the color at index 1 (blue for EGA and Amiga style rendering) is not
  * available for actors. Therefore, actors that need blue in their costume need to map blue to one
  * of the other palette entries (eg. for Dave in Maniac Mansion).
  *
  * @param actor_id The id of the actor to map the palette for.
  * @param dest_idx The destination palette index to map the source palette to.
  * @param src_idx The source palette index to map to the destination index.
  */
void actor_map_palette(uint8_t actor_id, uint8_t dest_idx, uint8_t src_idx)
{
  uint8_t actor_palette = actors.palette_idx[actor_id];
  if (actor_palette == 1) {
    // actor not yet using custom palette
    if (vm_state.num_actor_palettes == 14) {
      fatal_error(ERR_TOO_MANY_ACTOR_PALETTES);
    }
    actor_palette = ++vm_state.num_actor_palettes;
    actors.palette_idx[actor_id] = actor_palette;
  }

  SAVE_CS_AUTO_RESTORE
  MAP_CS_GFX
  uint8_t r, g, b;
  gfx_get_palette(0, src_idx, &r, &g, &b);
  gfx_set_palette(actor_palette, dest_idx, r, g, b);
}

void actor_put_in_room(uint8_t actor_id, uint8_t room_no)
{
  if (actors.room[actor_id] == room_no) {
    return;
  }

  if (actor_is_in_current_room(actor_id)) {
    remove_local_actor(actor_id);
    vm_update_actors();
  }

  actors.room[actor_id] = room_no;
  if (room_no == vm_read_var8(VAR_SELECTED_ROOM)) {
    add_local_actor(actor_id);
    vm_update_actors();
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
  
  vm_update_actors();

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

void actor_change_costume(uint8_t actor_id, uint8_t costume_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    deactivate_costume(actor_id);
    actors.costume[actor_id] = costume_id;
    activate_costume(actor_id);
    reset_animation(local_id);
    vm_update_actors();
  }
  else {
    actors.costume[actor_id] = costume_id;
  }
}

/**
  * @brief Find the actor at the given position.
  *
  * This function is used to find the actor at the given position. The actor is found by checking
  * the current position and bounding box of all local actors. If the actor is found, the actor's
  * id is returned. If no actor is found at the given position, the function returns 0.
  *
  * @param x The x position to check for an actor.
  * @param y The y position to check for an actor.
  * @return The id of the actor at the given position, or 0 if no actor is found.
  */
uint8_t actor_find(uint8_t x, uint8_t y)
{
  for (uint8_t local_id = 0; local_id < MAX_LOCAL_ACTORS; ++local_id) {
    uint8_t actor_id = local_actors.global_id[local_id];
    if (actor_id == 0xff || actor_id == last_selected_actor || !actors.costume[actor_id]) {
      continue;
    }

    uint8_t actor_x = actors.x[actor_id];
    uint8_t actor_y = actors.y[actor_id];
    actor_x >>= 3;
    actor_y >>= 2;
    uint8_t width = local_actors.bounding_box_width[local_id];
    uint8_t height = local_actors.bounding_box_height[local_id];
    uint8_t x1 = local_actors.bounding_box_x[local_id];
    int16_t y1 = local_actors.bounding_box_y[local_id] + actors.elevation[actor_id];
    uint8_t x2 = x1 + width;
    int16_t y2 = y1 + height + actors.elevation[actor_id];
    if (x >= x1 && x < x2 && y >= y1 && y < y2) {
      return actor_id;
    }
  }
  return 0;
}

void actor_place_at(uint8_t actor_id, uint8_t x, uint8_t y)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (local_id != 0xff) {
    uint8_t cur_box = walkbox_correct_position_to_closest_box(&x, &y);
    local_actors.walk_to_box[local_id] = cur_box;
    local_actors.cur_box[local_id]     = cur_box;
    local_actors.next_box[local_id]    = cur_box;
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
    actors.dir[actor_id]               = get_target_direction(local_id);
  
    vm_update_actors();
  }
  else {
    actors.x[actor_id] = x;
    actors.y[actor_id] = y;
  }
}

void actor_walk_to(uint8_t actor_id, uint8_t x, uint8_t y, uint8_t target_dir)
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
    local_actors.next_box[local_id] = walk_to_box;
  }
  else {
    calculate_next_box_point(local_id);
  }
  local_actors.target_dir[local_id] = target_dir;
  if (is_walk_to_done(local_id)) {
    stop_walking(local_id);
    if (target_dir == 0xff) {
      local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    }
    return;
  }
  else if (local_actors.walking[local_id] != WALKING_STATE_CONTINUE) {
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

  uint8_t x             = object_hdr->walk_to_x;
  uint8_t y             = (object_hdr->walk_to_y_and_preposition & 0x1f) << 2;
  uint8_t obj_actor_dir = object_hdr->height_and_actor_dir & 0x03;

  actor_walk_to(actor_id, x, y, obj_actor_dir);
}

void actor_next_step(uint8_t local_id)
{
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPED) {
    return;
  }

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t walk_dir = local_actors.walk_dir[local_id];
  if (walk_dir < 2 && actors.x[actor_id] == local_actors.next_x[local_id]) {
    // debug_out("Reached x, correct y %u.%u to %u", actors.y[actor_id], local_actors.y_fraction[local_id], local_actors.next_y[local_id]);
    actors.y[actor_id] = local_actors.next_y[local_id];
  }
  else if (walk_dir >= 2 && actors.y[actor_id] == local_actors.next_y[local_id]) {
    // debug_out("Reached y, correct x %u.%u to %u", actors.x[actor_id], local_actors.x_fraction[local_id], local_actors.next_x[local_id]);
    actors.x[actor_id] = local_actors.next_x[local_id];
  }

  if (is_next_walk_to_point_reached(local_id)) {
    uint8_t cur_box = local_actors.next_box[local_id];
    uint8_t target_box = local_actors.walk_to_box[local_id];
    local_actors.cur_box[local_id] = cur_box;
    local_actors.masking[local_id] = walkbox_get_box_masking(cur_box);
    if (cur_box == target_box) {
      // debug_out("Reached target box %d", cur_box);
      local_actors.next_x[local_id] = local_actors.walk_to_x[local_id];
      local_actors.next_y[local_id] = local_actors.walk_to_y[local_id];
    }
    else {
      calculate_next_box_point(local_id);
    }
    calculate_step(local_id);
  }

  uint8_t target_dir = get_target_direction(local_id);

  if (is_walk_to_done(local_id)) {
    // debug_out("stop walking");
    stop_walking(local_id);
  }
  else {
    uint8_t do_step;
    uint8_t actor_dir = actors.dir[actor_id];
    if (local_actors.walking[local_id] == WALKING_STATE_STARTING) {
      // debug_out("Start walking");
      actor_start_animation(local_id, ANIM_WALKING + actors.dir[actor_id]);
      local_actors.walking[local_id] = WALKING_STATE_CONTINUE;
      do_step = 1;
    }
    else if (actor_dir != target_dir) {
      // debug_out("Turn");
      // turn but keep on walking
      turn_to_direction(local_id, target_dir);
      do_step = 0;
    }
    else {
      do_step = 1;
    }
    
    if (do_step && actor_dir == target_dir) {
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
      
      // debug_out("Actor %u position: %u.%u, %u.%u", actor_id, actors.x[actor_id], local_actors.x_fraction[local_id], actors.y[actor_id], local_actors.y_fraction[local_id]);
    }
  }

  vm_update_actors();
}

void actor_start_animation(uint8_t local_id, uint8_t animation)
{
  switch (animation & 0xfc) {
    case 0xf8: {
      uint8_t new_dir = animation & 0x03;
      // keep animation but just change direction
      if (local_actors.walk_dir[local_id] != new_dir) {
        stop_walking(local_id);
      }
      actor_change_direction(local_id, new_dir);
      local_actors.walking[local_id] = WALKING_STATE_STOPPED;
      return;
    }
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
  // debug_out("Local actor %d start animation %d", local_id, animation);
  if (animation >= costume_hdr->num_animations + 1 || !costume_hdr->animation_offsets[animation]) {
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
      // debug_out("    cur %d last %d cmd_ptr %x", *cel_level_cur_cmd, *cel_level_last_cmd, (uint16_t)*cel_level_cmd_ptr);
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

/**
  * @brief Sorts all local actors by their y position and draws them to the backbuffer.
  *
  * @note The function will change CS and DS and not restore them.
  */
void actor_sort_and_draw_all(void)
{
  MAP_CS_GFX
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
    if (actors.costume[global_id]) {
      uint8_t local_id = actors.local_id[global_id];
      actor_draw(local_id);
    }
  }

  gfx_finalize_actor_drawing();
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

  uint8_t pos_y = actors.y[global_id] - actors.elevation[global_id];
  if (local_actors.y_fraction[local_id] >= 0x8000) { // rounding y position
    ++pos_y;
  }
  pos_y <<= 1; // convert to pixel position

  uint8_t masking = local_actors.masking[local_id];
  int16_t min_x   = 0x7fff;
  int16_t min_y   = 0xff;
  int16_t max_x   = 0;
  int16_t max_y   = 0;

  map_ds_resource(local_actors.res_slot[local_id]);
  __auto_type hdr = (struct costume_header *)RES_MAPPED;
  __auto_type cel_level_cmd_ptr = local_actors.cel_level_cmd_ptr[local_id];
  __auto_type cel_level_cur_cmd = local_actors.cel_level_cur_cmd[local_id];
  __auto_type cel_level_table_offset = hdr->level_table_offsets;

  // step 1: determine bounding box, relative position and image pointers for all cels
  uint8_t mirror = actors.dir[global_id] == 0 && !(hdr->disable_mirroring_and_format & 0x80);
  int16_t dx = -72;
  int16_t dy = -100;

  // debug_out("actor %d local_id %d elevation %d", global_id, local_id, actors.elevation[global_id]);
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
        int16_t cel_x    = pos_x;
        int16_t dx_level = dx + cur_cel_data->offset_x;
        if (mirror) {
          cel_x -= dx_level + cur_cel_data->width - 16;
        }
        else {
          cel_x += dx_level + 8;
        }

        // calculate scene y position in pixels for this actor cel image
        int16_t dy_level = dy + cur_cel_data->offset_y;
        int16_t cel_y    = pos_y + dy_level;
        
        // adjust bounding box for all cels
        // debug_out("min_y %d, cel_y %d, max_y %d", min_y, cel_y, max_y);
        min_x = min(min_x, cel_x);
        min_y = min(min_y, cel_y);
        max_x = max(max_x, cel_x + (int16_t)cur_cel_data->width);
        max_y = max(max_y, cel_y + (int16_t)cur_cel_data->height);

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

  local_actors.bounding_box_x[local_id]      = min_x >> 3;
  local_actors.bounding_box_y[local_id]      = min_y >> 1;
  local_actors.bounding_box_width[local_id]  = (width + 7) >> 3;
  local_actors.bounding_box_height[local_id] = (max_y - min_y + 1) >> 1;

  // step 2: allocate an empty canvas for the actor 
  //  debug_out("prepare min_x %d, min_y %d, width %d, height %d", min_x, min_y, width, height);
  uint8_t palette;
  if (vm_read_var8(VAR_CURRENT_LIGHTS) >= 11) {
    palette = actors.palette_idx[global_id];
  }
  else {
    palette = 15; // palette 15 is used for actor drawing in dark rooms
  }
  if (!gfx_prepare_actor_drawing(min_x, min_y, width, height, palette)) {
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
  // debug_out("actor done");
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
  static const uint8_t dir_invert[4] = {FACING_RIGHT, FACING_LEFT, FACING_BACK, FACING_FRONT};
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
  if (actors.costume[actor_id]) {
    uint8_t res_slot = res_provide(RES_TYPE_COSTUME, actors.costume[actor_id], 0);
    res_activate_slot(res_slot);
    local_actors.res_slot[local_id] = res_slot;
  }
}

static void deactivate_costume(uint8_t actor_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  if (actors.costume[actor_id]) {
    res_deactivate_slot(local_actors.res_slot[local_id]);
  }
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
    // debug_out(" stopping");
    actor_start_animation(local_id, ANIM_STANDING + actors.dir[actor_id]);
    local_actors.x_fraction[local_id]  = 0;
    local_actors.y_fraction[local_id]  = 0;
    local_actors.walk_to_x[local_id]   = actors.x[actor_id];
    local_actors.walk_to_y[local_id]   = actors.y[actor_id];
    local_actors.walk_to_box[local_id] = local_actors.cur_box[local_id];
    local_actors.next_box[local_id]    = local_actors.cur_box[local_id];
    return;
  }
  if (target_dir != 0xff && target_dir != actors.dir[actor_id]) {
    if (local_actors.walk_dir[local_id] != target_dir) {
      local_actors.walk_dir[local_id] = target_dir;
    }
    // debug_out(" turning to %d", target_dir);
    local_actors.walking[local_id] = WALKING_STATE_STOPPING;
    turn_to_target_direction(local_id);
  }
  if (local_actors.walking[local_id] == WALKING_STATE_STOPPING && 
      (target_dir == 0xff || target_dir == actors.dir[actor_id])) {
    // debug_out(" walking finished");
    local_actors.walking[local_id] = WALKING_STATE_FINISHED;
    return;
  }
  if (local_actors.walking[local_id] == WALKING_STATE_FINISHED) {
    // debug_out(" stopped");
    local_actors.walking[local_id] = WALKING_STATE_STOPPED;
    actor_start_animation(local_id, ANIM_STANDING + actors.dir[actor_id]);
  }
}

static void calculate_next_box_point(uint8_t local_id)
{
  SAVE_DS_AUTO_RESTORE
  // need to map in room data for walkbox access
  map_ds_resource(room_res_slot);

  uint8_t cur_box    = local_actors.cur_box[local_id];
  uint8_t target_box = local_actors.walk_to_box[local_id];
  uint8_t next_box   = walkbox_get_next_box(cur_box, target_box);
  
  // There are boxes you can't reach (either locked or no transition to them).
  // In case of no transition, we will be told to stay in the current box.
  // Go to the closest point to the target that we can find in the current box.
  if (next_box == cur_box || walk_boxes[next_box].classes & WALKBOX_CLASS_BOX_LOCKED) {
    uint8_t target_x = local_actors.walk_to_x[local_id];
    uint8_t target_y = local_actors.walk_to_y[local_id];

    walkbox_find_closest_box_point(cur_box, &target_x, &target_y);
    
    local_actors.walk_to_x[local_id] = target_x;
    local_actors.next_x[local_id]    = target_x;
    local_actors.walk_to_y[local_id] = target_y;
    local_actors.next_y[local_id]    = target_y;
    local_actors.next_box[local_id]  = cur_box;
    return;
  }

  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t cur_x    = actors.x[actor_id];
  uint8_t cur_y    = actors.y[actor_id];

  // debug_out("Cur %d Next %d Target %d", cur_box, next_box, target_box);
  walkbox_find_closest_box_point(next_box, &cur_x, &cur_y);
  // debug_out("Next box point %d, %d", cur_x, cur_y);
  uint8_t next_box_x = cur_x;
  uint8_t next_box_y = cur_y;
  walkbox_find_closest_box_point(cur_box, &cur_x, &cur_y);
  // debug_out("Current box point %d, %d", cur_x, cur_y);
  local_actors.next_x[local_id]   = cur_x;
  local_actors.next_y[local_id]   = cur_y;
  local_actors.next_box[local_id] = next_box;
  // debug_out("Adapted next box point %d, %d", cur_x, cur_y);
}

static void calculate_step(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t x        = actors.x[actor_id];
  uint8_t y        = actors.y[actor_id];
  uint8_t next_x   = local_actors.next_x[local_id];
  uint8_t next_y   = local_actors.next_y[local_id];

  // Take the diff between the current position and the walk_to position. Then, calculate the
  // fraction of the diff that the actor will move in this step. The dominant direction is always
  // increasing by 1, and the other direction is increased by the fraction of the diff, so that
  // the actor moves in a straight line from the current position to the walk_to position. The fraction 
  // is used with 16 bits of precision to avoid rounding errors.

  int8_t x_diff  = next_x - x;
  int8_t y_diff  = next_y - y;
  int32_t x_step = 0;
  int32_t y_step = 0;

  // debug_out("From %d, %d (b%d) to %d, %d (b%d) x_diff %d y_diff %d", x, y, local_actors.cur_box[local_id], next_x, next_y, local_actors.next_box[local_id], x_diff, y_diff);

  if (x_diff == 0 && y_diff == 0) {
    local_actors.walk_step_x[local_id] = 0;
    local_actors.walk_step_y[local_id] = 0;
    return;
  }

  int8_t abs_x_diff = abs8(x_diff);
  int8_t abs_y_diff = abs8(y_diff);

  if (abs_x_diff < abs_y_diff) {
    if (y_diff < 0) {
      local_actors.walk_dir[local_id] = FACING_BACK;
    }
    else {
      local_actors.walk_dir[local_id] = FACING_FRONT;
    }
  }
  else {
    if (x_diff < 0) {
      local_actors.walk_dir[local_id] = FACING_LEFT;
    }
    else {
      local_actors.walk_dir[local_id] = FACING_RIGHT;
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
  uint8_t local_id                 = get_free_local_id();
  actors.local_id[actor_id]        = local_id;
  local_actors.global_id[local_id] = actor_id;

  activate_costume(actor_id);
  
  actor_place_at(actor_id, actors.x[actor_id], actors.y[actor_id]);

  reset_animation(local_id);
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
  uint8_t global_id = local_actors.global_id[local_id];
  
  if (!actors.costume[global_id]) {
    return;
  }

  __auto_type cel_anim          = local_actors.cel_anim[local_id];
  __auto_type cel_level_cur_cmd = local_actors.cel_level_cur_cmd[local_id];
  uint8_t dir = actors.dir[global_id];

  for (uint8_t i = 0; i < 16; ++i) {
    cel_anim[i]          = 0xff;
    cel_level_cur_cmd[i] = 0xff;
  }
  actor_start_animation(local_id, ANIM_STANDING   + dir);
  actor_start_animation(local_id, ANIM_HEAD       + dir);
  actor_start_animation(local_id, ANIM_MOUTH_SHUT + dir);
}

/**
  * @brief Get the direction the actor should face to walk to the target position.
  *
  * This function calculates the direction the actor should face to walk to the target position.
  * The function takes into account the current direction of the actor and the walk-to direction.
  *
  * The function needs to have the current room mapped to DS when being called in order to access
  * the walk boxes.
  *
  * @param local_id The local id of the actor.
  * @return The direction the actor should face to walk to the target position.
  */
static uint8_t get_target_direction(uint8_t local_id)
{
  uint8_t target_dir;
  uint8_t walk_box_direction = walkbox_get_box_classes(local_actors.cur_box[local_id]) & 0x07;
  
  switch (walk_box_direction) {
    case 5:
      target_dir = FACING_BACK;
      break;
    default:
      if (local_actors.walking[local_id]) {
        target_dir = local_actors.walk_dir[local_id];
      }
      else {
        uint8_t actor_id = local_actors.global_id[local_id];
        target_dir = actors.dir[actor_id];
      }
  }

  return target_dir;
}

static void turn_to_target_direction(uint8_t local_id)
{
  uint8_t target_dir = get_target_direction(local_id);
  turn_to_direction(local_id, target_dir);
}

static void turn_to_direction(uint8_t local_id, uint8_t target_dir)
{
  uint8_t actor_id   = local_actors.global_id[local_id];

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
  static const uint8_t turn_dir[4] = {FACING_FRONT, FACING_FRONT, FACING_RIGHT, FACING_LEFT};
  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t current_dir = actors.dir[actor_id];
  actor_change_direction(local_id, turn_dir[current_dir]);
}

/// @} // actor_private

//-----------------------------------------------------------------------------------------------