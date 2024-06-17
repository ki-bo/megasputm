#pragma once

#include <stdint.h>

//-----------------------------------------------------------------------------------------------

enum {
  ANIM_WALKING    =  0,
  ANIM_STANDING   =  4,
  ANIM_HEAD       =  8,
  ANIM_MOUTH_OPEN = 12,
  ANIM_MOUTH_SHUT = 16,
  ANIM_TALKING    = 20
};

//-----------------------------------------------------------------------------------------------
struct costume_header {
    uint16_t chunk_size;
    uint16_t unused1;
    uint8_t  num_animations;
    uint8_t  disable_mirroring_and_format;
    uint8_t  color;
    uint16_t animation_commands_offset;
    uint16_t level_table_offsets[16];
    uint16_t animation_offsets[];
};

struct costume_cel{
  uint16_t width;
  uint16_t height;
  int16_t offset_x;
  int16_t offset_y;
  int16_t move_x;
  int16_t move_y;
};
