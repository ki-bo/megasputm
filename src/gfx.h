#ifndef __GFX_H
#define __GFX_H

#include <stdint.h>

void gfx_init(void);
uint8_t wait_for_raster_irq(void);

#endif // __GFX_H
