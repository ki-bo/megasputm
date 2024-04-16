#ifndef __GFX_H
#define __GFX_H

#include "costume.h"
#include <stdint.h>

// code_init functions
void gfx_init(void);

// code_gfx functions
void gfx_start(void);
void gfx_fade_out(void);
uint8_t gfx_wait_for_jiffy_timer(void);
void gfx_wait_vsync(void);
void gfx_clear_bg_image(void);
void gfx_decode_bg_image(uint8_t *src, uint16_t width);
void gfx_decode_masking_buffer(uint16_t bg_masking_offset, uint16_t width);
void gfx_decode_object_image(uint8_t *src, uint8_t width, uint8_t height);
void gfx_clear_dialog(void);
void gfx_print_dialog(uint8_t color, const char *text, uint8_t num_chars);
void gfx_draw_bg(void);
void gfx_draw_object(uint8_t local_id, int8_t x, int8_t y, uint8_t width, uint8_t height);
void gfx_draw_cel(int16_t xpos, int16_t ypos, struct costume_cel *cel_data, uint8_t mirror);
void gfx_finalize_cel_drawing(void);
void gfx_reset_cel_drawing(void);
void gfx_update_main_screen(void);

#endif // __GFX_H
