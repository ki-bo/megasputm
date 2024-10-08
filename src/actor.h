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

#pragma once

#include "vm.h"
#include <stdint.h>

#define NUM_ACTORS         25
#define MAX_LOCAL_ACTORS    6
#define ACTOR_NAME_LEN     16

typedef struct {
  uint8_t       sound[NUM_ACTORS];
  uint8_t       palette_idx[NUM_ACTORS];
  char          name[NUM_ACTORS][ACTOR_NAME_LEN];
  uint8_t       costume[NUM_ACTORS];
  uint8_t       talk_color[NUM_ACTORS];
  uint8_t       room[NUM_ACTORS];
  uint8_t       local_id[NUM_ACTORS];
  uint8_t       x[NUM_ACTORS];
  uint8_t       y[NUM_ACTORS];
  uint8_t       elevation[NUM_ACTORS]; // distance above ground
  uint8_t       dir[NUM_ACTORS];
} actors_t;

typedef struct {
  uint8_t       global_id[MAX_LOCAL_ACTORS];
  uint8_t       local_id[MAX_LOCAL_ACTORS];
  uint8_t       res_slot[MAX_LOCAL_ACTORS];
  uint8_t       bounding_box_x[MAX_LOCAL_ACTORS];
  uint8_t       bounding_box_y[MAX_LOCAL_ACTORS];
  uint8_t       bounding_box_width[MAX_LOCAL_ACTORS];
  uint8_t       bounding_box_height[MAX_LOCAL_ACTORS];
  uint8_t       cel_anim[MAX_LOCAL_ACTORS][16];
  uint8_t      *cel_level_cmd_ptr[MAX_LOCAL_ACTORS][16];
  uint8_t       cel_level_cur_cmd[MAX_LOCAL_ACTORS][16];
  uint8_t       cel_level_last_cmd[MAX_LOCAL_ACTORS][16];
  uint8_t       walking[MAX_LOCAL_ACTORS];
  uint8_t       x_accum[MAX_LOCAL_ACTORS];
  uint8_t       y_accum[MAX_LOCAL_ACTORS];
  uint8_t       x_inc[MAX_LOCAL_ACTORS];
  uint8_t       y_inc[MAX_LOCAL_ACTORS];
  uint8_t       walk_diff[MAX_LOCAL_ACTORS];
  int8_t        walk_step_x[MAX_LOCAL_ACTORS];
  int8_t        walk_step_y[MAX_LOCAL_ACTORS];
  uint8_t       cur_box[MAX_LOCAL_ACTORS];
  uint8_t       target_dir[MAX_LOCAL_ACTORS];
  uint8_t       walk_dir[MAX_LOCAL_ACTORS];
  uint8_t       walk_to_box[MAX_LOCAL_ACTORS];
  uint8_t       walk_to_x[MAX_LOCAL_ACTORS];
  uint8_t       walk_to_y[MAX_LOCAL_ACTORS];
  uint8_t       next_box[MAX_LOCAL_ACTORS];
  uint8_t       next_x[MAX_LOCAL_ACTORS];
  uint8_t       next_y[MAX_LOCAL_ACTORS];
  uint8_t       masking[MAX_LOCAL_ACTORS];
} local_actors_t;

//-----------------------------------------------------------------------------------------------

enum {
  WALKING_STATE_STOPPED  = 0,
  WALKING_STATE_TURNING  = 1,
  WALKING_STATE_CONTINUE = 2,
  WALKING_STATE_STOPPING = 3,
  WALKING_STATE_FINISHED = 4,
  WALKING_STATE_RESTART  = 0x80
};

enum {
  FACING_LEFT  = 0,
  FACING_RIGHT = 1,
  FACING_FRONT = 2,
  FACING_BACK  = 3
};

//-----------------------------------------------------------------------------------------------

extern actors_t actors;
extern local_actors_t local_actors;

//-----------------------------------------------------------------------------------------------

// init functions
void actor_init(void);

// main functions
void actor_map_palette(uint8_t actor_id, uint8_t dest_idx, uint8_t src_idx);
void actor_put_in_room(uint8_t actor_id, uint8_t room_no);
void actor_room_changed(void);
void actor_change_costume(uint8_t actor_id, uint8_t costume_id);
uint8_t actor_find(uint8_t x, uint8_t y);
void actor_place_at(uint8_t actor_id, uint8_t x, uint8_t y);
void actor_walk_to(uint8_t actor_id, uint8_t x, uint8_t y, uint8_t target_dir);
void actor_walk_to_object(uint8_t actor_id, uint16_t object_id);
void actor_stop_and_turn(uint8_t actor_id, uint8_t dir);
void actor_next_step(uint8_t local_id);
void actor_start_animation(uint8_t actor_id, uint8_t animation);
void actor_update_animation(uint8_t local_id);
void actor_sort_and_draw_all(void);
void actor_draw(uint8_t local_id);
void actor_start_talking(uint8_t actor_id);
void actor_stop_talking(uint8_t actor_id);
uint8_t actor_invert_direction(uint8_t dir);
void actor_change_direction(uint8_t local_id, uint8_t dir);

// inline main functions
#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"
static inline uint8_t actor_is_in_current_room(uint8_t actor_id)
{
    return actors.local_id[actor_id] != 0xff;
}
