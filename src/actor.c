#include "actor.h"
#include "error.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdlib.h>

//-----------------------------------------------------------------------------------------------

actors_t actors;
local_actors_t local_actors;

//-----------------------------------------------------------------------------------------------

// private functions
static uint8_t get_free_local_id(void);
static void activate_costume(uint8_t actor_id);
static void deactivate_costume(uint8_t actor_id);
static uint8_t is_walk_to_done(uint8_t local_id);
static void calculate_step(uint8_t local_id);

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

void actor_place_in_room(uint8_t actor_id, uint8_t room_no)
{
  if (actors.room[actor_id] == room_no) {
    return;
  }

  if (actor_is_in_current_room(actor_id)) {
    map_cs_gfx();
    //gfx_remove_actor(actor_id);
    unmap_cs();
    deactivate_costume(actor_id);
    local_actors.global_id[actors.local_id[actor_id]] = 0xff;
    actors.local_id[actor_id] = 0xff;
    debug_out("Actor %d is no longer in current room", actor_id);
  }

  actors.room[actor_id] = room_no;
  if (room_no == vm_read_var8(VAR_SELECTED_ROOM)) {
    uint8_t local_id = get_free_local_id();
    actors.local_id[actor_id] = local_id;
    local_actors.global_id[local_id] = actor_id;
    activate_costume(actor_id);
    debug_out("Actor %d is now in current room with local id %d", actor_id, local_id);
    map_cs_gfx();
    //gfx_add_actor(actor_id);
    unmap_cs();
  }
}

void actor_walk_to(uint8_t actor_id, uint8_t x, uint8_t y)
{
  if (!actor_is_in_current_room(actor_id)) {
    actors.x[actor_id] = x;
    actors.y[actor_id] = y;
    return;
  }

  uint8_t local_id = actors.local_id[actor_id];
  local_actors.x_fraction[local_id] = 0;
  local_actors.y_fraction[local_id] = 0;
  local_actors.walk_to_x[local_id]  = x;
  local_actors.walk_to_y[local_id]  = y;
  local_actors.walking[local_id]    = !is_walk_to_done(local_id);

  if (local_actors.walking[local_id]) {
    calculate_step(local_id);
  }
}

void actor_next_step(uint8_t local_id)
{
  if (!local_actors.walking[local_id]) {
    return;
  }

  uint8_t actor_id = local_actors.global_id[local_id];
  int32_t x = actors.x[actor_id] * 0x10000 + local_actors.x_fraction[local_id];
  int32_t y = actors.y[actor_id] * 0x10000 + local_actors.y_fraction[local_id];

  if (actors.x[actor_id] != local_actors.walk_to_x[local_id]) {
    int32_t new_x = x + local_actors.walk_step_x[local_id];
    actors.x[actor_id] = (uint8_t)(new_x >> 16);
    local_actors.x_fraction[local_id] = (uint16_t)new_x;
  }
  if (actors.y[actor_id] != local_actors.walk_to_y[local_id]) {
    int32_t new_y = y + local_actors.walk_step_y[local_id];
    actors.y[actor_id] = (uint8_t)(new_y >> 16);
    local_actors.y_fraction[local_id] = (uint16_t)new_y;
  }
  
  debug_out("Actor %u position: %u.%u, %u.%u", actor_id, actors.x[actor_id], local_actors.x_fraction[local_id], actors.y[actor_id], local_actors.y_fraction[local_id]);

  if (is_walk_to_done(local_id)) {
    local_actors.x_fraction[local_id] = 0;
    local_actors.y_fraction[local_id] = 0;
    local_actors.walking[local_id] = 0;
  }
}

void actor_start_animation(uint8_t actor_id, uint8_t animation)
{
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
  res_set_flags(res_slot, RES_ACTIVE_MASK);
  local_actors.res_slot[local_id] = res_slot;
}

static void deactivate_costume(uint8_t actor_id)
{
  uint8_t local_id = actors.local_id[actor_id];
  res_clear_flags(local_actors.res_slot[local_id], RES_ACTIVE_MASK);
}

static uint8_t is_walk_to_done(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  return (actors.x[actor_id] == local_actors.walk_to_x[local_id] &&
          actors.y[actor_id] == local_actors.walk_to_y[local_id]);
}

static void calculate_step(uint8_t local_id)
{
  uint8_t actor_id = local_actors.global_id[local_id];
  uint8_t x = actors.x[actor_id];
  uint8_t y = actors.y[actor_id];
  uint8_t walk_to_x = local_actors.walk_to_x[local_id];
  uint8_t walk_to_y = local_actors.walk_to_y[local_id];

  // Take the diff between the current position and the walk_to position. Then, calculate the
  // fraction of the diff that the actor will move in this step. The dominant direction is always
  // increasing by 1, and the other direction is increased by the fraction of the diff, so that
  // the actor moves in a straight line from the current position to the walk_to position. The fraction 
  // is used with 16 bits of precision to avoid rounding errors.

  int8_t x_diff = walk_to_x - x;
  int8_t y_diff = walk_to_y - y;
  int32_t x_step = 0;
  int32_t y_step = 0;

  debug_out("From %d, %d to %d, %d", x, y, walk_to_x, walk_to_y);
  debug_out("Diff: %d, %d", x_diff, y_diff);

  if (x_diff == 0 && y_diff == 0) {
    local_actors.walking[local_id] = 0;
    return;
  }

  if (x_diff == 0) {
    local_actors.walk_step_x[local_id] = 0;
    local_actors.walk_step_y[local_id] = (y_diff > 0) ? 0x10000 : -0x10000;
  } else if (y_diff == 0) {
    local_actors.walk_step_x[local_id] = (x_diff > 0) ? 0x10000 : -0x10000;
    local_actors.walk_step_y[local_id] = 0;
  } else {
    int8_t abs_x_diff = abs(x_diff);
    int8_t abs_y_diff = abs(y_diff);
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
  debug_out("Actor %d step: %ld, %ld", actor_id, x_step, y_step);
}

/// @} // actor_private

//-----------------------------------------------------------------------------------------------