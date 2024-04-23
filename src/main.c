#include "diskio.h"
#include "init.h"
#include "map.h"
#include "vm.h"
#include "util.h"

#pragma clang section text="code"

/**
 * @brief Entry function called by startup code
 *
 * This is the first C code getting executed after the startup code
 * has finished. It initializes the system and then calls the main_entry
 * function in the code_main section.
 *
 * It is part of the runtime code loaded during startup to 0x0200.
 *
 * Code section: code
 */
__task void main(void) 
{
  global_init();

  // Use diskio module to load the main code to section code_main at 0x4000
  // Be aware that this is overwriting both init code and init bss in memory.
  // So init needs to be complete at this point and can't be called anymore.
  map_cs_diskio();
  diskio_load_file("M01", (uint8_t __far *)(0x2000));  // load script parser code
  diskio_load_file("M02", (uint8_t __far *)(0x4000));  // load main code
  unmap_cs();

  // switch to real drive
  //*(uint8_t *)(0xd68b) &= ~1;
  //*(uint8_t *)(0xd6a1) |= 1;
  
  // Jump into loaded main code
  vm_mainloop();
}
