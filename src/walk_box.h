#pragma once

#include <stdint.h>

struct walk_box {
  uint8_t top_y;
  uint8_t bottom_y;
  uint8_t topleft_x;
  uint8_t topright_x;
  uint8_t bottomleft_x;
  uint8_t bottomright_x;
  uint8_t mask;
  uint8_t classes;
};

enum walk_box_class {
  WALKBOX_CLASS_BOX_LOCKED    = 0x40,
  WALKBOX_CLASS_BOX_INVISIBLE = 0x80
};

extern uint8_t          num_walk_boxes;
extern struct walk_box *walk_boxes;
extern uint8_t         *walk_box_matrix;

// main functions
uint8_t walkbox_get_next_box(uint8_t cur_box, uint8_t target_box);
uint8_t walkbox_get_box_masking(uint8_t box_id);
uint8_t walkbox_correct_position_to_closest_box(uint8_t *x, uint8_t *y);
uint16_t walkbox_get_corrected_box_position(struct walk_box *box, uint8_t *x, uint8_t *y);
void walkbox_find_closest_box_point(uint8_t box_id, uint8_t *px, uint8_t *py);
