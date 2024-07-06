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
  diskio_init();

  // load and init gfx module (CS_DISKIO is still mapped from diskio_init)
  diskio_load_file("M10", (uint8_t __far *)(0x11800)); // load gfx2 code
  diskio_load_file("M12", (uint8_t __far *)(0x14000)); // load gfx code
  gfx_init();

  // init input module
  input_init();

  // init main engine code
  res_init();    // resource module
  heap_init();   // heap
  inv_init();    // inventory
  script_init(); // script parser
  actor_init();  // actor module
  vm_init();     // virtual machine and main game logic
}
