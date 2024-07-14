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

#include "costume.h"
#include <stdint.h>

enum text_style {
  TEXT_STYLE_NORMAL,
  TEXT_STYLE_HIGHLIGHTED,
  TEXT_STYLE_SENTENCE,
  TEXT_STYLE_INVENTORY,
  TEXT_STYLE_INVENTORY_ARROW
};

// code_init functions
void gfx_init(void);

// code_gfx2 functions
void gfx_update_flashlight(void);

// code_gfx functions
void gfx_start(void);
void gfx_fade_out(void);
uint8_t gfx_wait_for_jiffy_timer(void);
void gfx_wait_vsync(void);
void gfx_reset_palettes(void);
void gfx_get_palette(uint8_t palette, uint8_t col_idx, uint8_t *r, uint8_t *g, uint8_t *b);
void gfx_set_palette(uint8_t palette, uint8_t col_idx, uint8_t r, uint8_t g, uint8_t b);
void gfx_clear_bg_image(void);
void gfx_decode_bg_image(uint8_t __huge *src, uint16_t width);
void gfx_decode_masking_buffer(uint16_t bg_masking_offset, uint16_t width);
void gfx_set_object_image(uint8_t __huge *src, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void gfx_clear_dialog(void);
void gfx_print_dialog(uint8_t color, const char *text, uint8_t num_chars);
void gfx_draw_bg(uint8_t lights);
void gfx_draw_object(uint8_t local_id, int8_t x, int8_t y);
void gfx_enable_flashlight(void);
void gfx_disable_flashlight(void);
void gfx_flashlight_irq_update(uint8_t enable);
uint8_t gfx_prepare_actor_drawing(int16_t screen_pos_x, int8_t screen_pos_y, uint8_t width, uint8_t height, uint8_t palette);
void gfx_draw_actor_cel(uint8_t xpos, uint8_t ypos, struct costume_cel *cel_data, uint8_t mirror);
void gfx_apply_actor_masking(int16_t xpos, int8_t ypos, uint8_t masking);
void gfx_finalize_actor_drawing(void);
void gfx_reset_actor_drawing(void);
void gfx_update_main_screen(void);
void gfx_print_interface_text(uint8_t x, uint8_t y, const char *name, enum text_style style);
void gfx_change_interface_text_style(uint8_t x, uint8_t y, uint8_t size, enum text_style style);
void gfx_clear_sentence(void);
void gfx_clear_verbs(void);
void gfx_clear_inventory(void);
