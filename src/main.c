#include <string.h>
#include "diskio.h"
#include "init.h"
#include "map.h"
#include "util.h"

#pragma clang section text="code_main"

__task void main_entry(void)
{
  while(1) POKE(0xd020, 5);
}

#pragma clang section text="code"

__task void main(void) 
{
  global_init();
  map_diskio();
  diskio_load_file("M01", (uint8_t __far *)(0x2000));
  unmap_all();
  main_entry();
}
