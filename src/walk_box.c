#include "walk_box.h"
#include "map.h"
#include "util.h"
#include "vm.h"
#include <stdlib.h>

//----------------------------------------------------------------------------------------------

#pragma clang section data="data_main" rodata="cdata_main" bss="zdata"

uint8_t          num_walk_boxes;
struct walk_box *walk_boxes;
uint8_t         *walk_box_matrix;

//----------------------------------------------------------------------------------------------

// private functions
static uint8_t binary_search_xy(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t yc);
static void find_closest_point_on_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t *x, uint8_t *y);


//----------------------------------------------------------------------------------------------

/**
 * @defgroup walkbox_public Walk Box Public Functions
 * @{
 */
#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"

uint8_t walkbox_get_next_box(uint8_t cur_box, uint8_t target_box)
{
  __auto_type matrix_row = walk_box_matrix + num_walk_boxes + walk_box_matrix[cur_box];
  __auto_type box_ptr = walk_box_matrix;

  //debug_out("walk_box_matrix %p matrix_row %p", walk_box_matrix, matrix_row);
  uint8_t next_box = matrix_row[target_box];
  return next_box;
}

uint8_t walkbox_get_box_masking(uint8_t box_id)
{
  uint16_t save_ds = map_get_ds();
  map_ds_resource(room_res_slot);
  __auto_type box = walk_boxes[box_id];
  uint8_t masking = box.mask;
  map_set_ds(save_ds);
  return masking;
}

uint8_t walkbox_correct_position_to_closest_box(uint8_t *x, uint8_t *y)
{
  uint16_t save_ds = map_get_ds();
  map_ds_resource(room_res_slot);

  uint16_t min_distance = 0xffff;
  uint8_t corr_pos_x;
  uint8_t corr_pos_y;
  uint8_t dest_walk_box;

  struct walk_box *walk_box = walk_boxes;

  debug_out("Correct pos %d, %d", *x, *y);
  for (uint8_t box_idx = 0; box_idx < num_walk_boxes; ++box_idx) {
    uint8_t walk_box_x = *x;
    uint8_t walk_box_y = *y;
    debug_out("Checking box %d", box_idx);
    uint16_t distance = walkbox_get_corrected_box_position(walk_box, &walk_box_x, &walk_box_y);
    debug_out("  w_x,y: %d, %d d %d", walk_box_x, walk_box_y, distance);
    if (distance <= min_distance) {
      min_distance = distance;
      corr_pos_x = walk_box_x;
      corr_pos_y = walk_box_y;
      dest_walk_box = box_idx;
    }
    ++walk_box;
  }

  debug_out("Final %d, %d wb %d", corr_pos_x, corr_pos_y, dest_walk_box);

  *x = corr_pos_x;
  *y = corr_pos_y;

  map_set_ds(save_ds);

  return dest_walk_box;
}

uint16_t walkbox_get_corrected_box_position(struct walk_box *box, uint8_t *x, uint8_t *y)
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
    uint16_t save_cs = map_cs_main_priv();
    x_left = binary_search_xy(box->topleft_x, box->bottomleft_x, box->top_y, box->bottom_y, yc);
    map_set_cs(save_cs);
    x_right = x_left;
  }
  else if (xc > box->topright_x || xc > box->bottomright_x) {
    //debug_out("  right of box");
    uint16_t save_cs = map_cs_main_priv();
    x_left = binary_search_xy(box->topright_x, box->bottomright_x, box->top_y, box->bottom_y, yc);
    map_set_cs(save_cs);
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
void walkbox_find_closest_box_point(uint8_t box_id, uint8_t *px, uint8_t *py)
{
  uint16_t save_cs = map_cs_main_priv();


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

  map_set_cs(save_cs);
}

/** @} */ // walkbox_public

//----------------------------------------------------------------------------------------------

/**
 * @defgroup walkbox_private Walk Box Private Functions
 * @{
 */
#pragma clang section text="code_main_private"

static uint8_t binary_search_xy(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t yc)
{
  uint8_t yn = y1;
  uint8_t xn = x1;
  while (yn != yc) {
    //debug_out("yn %d yc %d y1 %d y2 %d x1 %d x2 %d", yn, yc, y1, y2, x1, x2);
    xn = (x1 + x2) >> 1;
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

/** @} */ // walkbox_private
