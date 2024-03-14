#include "init.h"
#include "util.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "map.h"
#include "resource.h"
#include "script.h"
#include "vm.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

void global_init(void)
{
  // configure dma
  dma_init();

  // init diskio module
  map_cs_diskio();
  diskio_init();

  // init gfx module  
  diskio_load_file("M12", (uint8_t __far *)(0x14000)); // load gfx code
  map_cs_gfx();
  gfx_init();
  unmap_cs();


  // init main engine code
  res_init();
  script_init();
  vm_init();
}
