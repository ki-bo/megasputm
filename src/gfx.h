#ifndef __GFX_H
#define __GFX_H

#include <stdint.h>

// code_init functions
void gfx_init(void);

// code functions
uint8_t gfx_wait_for_jiffy_timer(void);
void gfx_wait_for_next_frame(void);

// code_gfx functions
void gfx_start(void);
void gfx_fade_out(void);
void gfx_fade_in(void);
void gfx_decode_bg_image(uint8_t *src, uint16_t width);

#endif // __GFX_H
