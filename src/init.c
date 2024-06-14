#include "init.h"
#include "actor.h"
#include "charset.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "heap.h"
#include "input.h"
#include "inventory.h"
#include "map.h"
#include "util.h"
#include "resource.h"
#include "script.h"
#include "vm.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

/**
  * @brief Initialises all submodules
  *
  * This function does all initialisation of all sub-modules. It needs to be called
  * at the beginning of the autoboot prg.
  *
  * Code section: code_init
  */
void global_init(void)
{
  map_init();
  
  // configure dma
  dma_init();

  // prepare charset
  charset_init();

  // init diskio module
  MAP_CS_DISKIO
  diskio_init();

  // init gfx module  
  diskio_load_file("M12", (uint8_t __far *)(0x14000)); // load gfx code
  MAP_CS_GFX
  gfx_init();
  UNMAP_CS

  // init input module
  input_init();

  // init main engine code
  res_init();
  heap_init();
  inv_init();
  script_init();
  actor_init();
  vm_init();
}
