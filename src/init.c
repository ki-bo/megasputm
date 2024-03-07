#include "init.h"
#include "util.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "map.h"
#include "resource.h"
#include "vm.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

void global_init(void)
{
  dma_init();
  map_cs_diskio();
  diskio_init();
  map_cs_gfx();
  gfx_init();
  unmap_cs();
  res_init();
  vm_init();
}
