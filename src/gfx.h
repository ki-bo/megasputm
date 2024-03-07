#ifndef __GFX_H
#define __GFX_H

#include <stdint.h>

void gfx_init(void);
uint8_t wait_for_jiffy_timer(void);
uint8_t wait_for_next_frame(void);

#endif // __GFX_H
