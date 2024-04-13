#ifndef __ACTOR_H
#define __ACTOR_H

#include "vm.h"
#include <stdint.h>

#define NUM_ACTORS         25
#define MAX_LOCAL_ACTORS    6
#define ACTOR_NAME_LEN     16

enum {
  ANIM_STAND = 1,
  ANIM_WALK = 2
};

typedef struct {
  uint8_t       sound[NUM_ACTORS];
  uint8_t       palette_idx[NUM_ACTORS];
  uint8_t       palette_color[NUM_ACTORS];
  char          name[NUM_ACTORS][ACTOR_NAME_LEN];
  uint8_t       costume[NUM_ACTORS];
  uint8_t       talk_color[NUM_ACTORS];
  uint8_t       room[NUM_ACTORS];
  uint8_t       local_id[NUM_ACTORS];
  uint8_t       x[NUM_ACTORS];
  uint8_t       y[NUM_ACTORS];
  uint8_t       z[NUM_ACTORS]; // distance above ground
  uint8_t       dir[NUM_ACTORS];
} actors_t;

typedef struct {
  uint8_t       global_id[MAX_LOCAL_ACTORS];
  uint8_t       res_slot[MAX_LOCAL_ACTORS];
  uint8_t       direction[MAX_LOCAL_ACTORS];
  uint8_t       cost_anim[MAX_LOCAL_ACTORS];
  uint8_t      *cel_level_cmd_ptr[MAX_LOCAL_ACTORS][16];
  uint8_t       cel_level_cur_cmd[MAX_LOCAL_ACTORS][16];
  uint8_t       cel_level_last_cmd[MAX_LOCAL_ACTORS][16];
  uint8_t       animation_just_started[MAX_LOCAL_ACTORS];
  uint8_t       walking[MAX_LOCAL_ACTORS];
  uint16_t      x_fraction[MAX_LOCAL_ACTORS];
  uint16_t      y_fraction[MAX_LOCAL_ACTORS];
  uint8_t       walk_to_x[MAX_LOCAL_ACTORS];
  uint8_t       walk_to_y[MAX_LOCAL_ACTORS];
  int32_t       walk_step_x[MAX_LOCAL_ACTORS];
  int32_t       walk_step_y[MAX_LOCAL_ACTORS];
} local_actors_t;

//-----------------------------------------------------------------------------------------------

extern actors_t actors;
extern local_actors_t local_actors;

//-----------------------------------------------------------------------------------------------

// init functions
void actor_init(void);

// main functions
void actor_put_in_room(uint8_t actor_id, uint8_t room_no);
void actor_room_changed(void);
void actor_walk_to(uint8_t actor_id, uint8_t x, uint8_t y);
uint8_t actor_next_step(uint8_t local_id);
void actor_start_animation(uint8_t actor_id, uint8_t animation);
uint8_t actor_update_animation(uint8_t local_id);
void actor_draw(uint8_t local_id);

// inline main functions
#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"
static inline uint8_t actor_is_in_current_room(uint8_t actor_id)
{
    return actors.local_id[actor_id] != 0xff;
}


#endif // __ACTOR_H
