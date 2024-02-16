#include <string.h>
#include "diskio.h"
#include "init.h"
#include "map.h"
#include "util.h"

#pragma clang section text="code"

__task void main(void) 
{
  global_init();
  map_diskio();
  //diskio_load_room(45, (uint8_t __far *)(0x40000));
  diskio_load_file("45.LFL", (uint8_t __far *)(0x2000));
  unmap_all();
  //dma_init();
  //diskio_init();
  //diskio_load_room(45, FAR_U8_PTR(0x12000));
  //diskio_load();
  //gfx_init();
  while(1) POKE(0xd020, 5);
}

#pragma clang section text="code_main"

__task void main_entry(void)
{

}
