#include "init.h"
#include "util.h"
#include "dma.h"
#include "gfx.h"
#include "map.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

void global_init(void)
{
  POKE16(0xd020, 0);

  dma_init();
  map_and_init_diskio();

  //gfx_init();
}
