#include "init.h"
#include "util.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "map.h"
#include "resource.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

void global_init(void)
{
  POKE16(0xd020, 0);

  dma_init();
  map_cs_diskio();
  diskio_init_entry();
  unmap_all();
  res_init();

  //gfx_init();
}
