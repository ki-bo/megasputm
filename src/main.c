#include "diskio.h"
#include "gfx.h"
#include "util.h"
#include <calypsi/intrinsics6502.h>

__task void main(void) 
{
  dma_init();
  diskio_init();
  diskio_load_room(45, FAR_U8_PTR(0x12000));
  //gfx_init();
  while(1) POKE(0xd020, 5);
}
