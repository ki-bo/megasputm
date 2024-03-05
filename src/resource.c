#include "resource.h"
#include "diskio.h"
#include "map.h"
#include "util.h"

static uint16_t start_resource_loading(uint8_t type, uint8_t id);
static void continue_resource_loading(void);

#pragma clang section bss="zdata"

uint8_t page_res_type[256];
uint8_t page_res_index[256];

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

void res_init(void)
{

}

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

uint8_t res_provide(uint8_t type, uint8_t id)
{
  uint8_t i = 0;
  do {
    if(page_res_type[i] == type && page_res_index[i] == id) {
      return i;
    }
    i++;
  }
  while(i != 0);

  uint16_t chunk_size = start_resource_loading(type, id);
  uint8_t page = 0;
  map_resource(page);
  continue_resource_loading();
  return 0;
}

#pragma clang section text="code"
static uint16_t start_resource_loading(uint8_t type, uint8_t id)
{
  map_diskio();
  uint16_t chunk_size = diskio_start_resource_loading(type, id);
  unmap_cs();
  return chunk_size;
}

static void continue_resource_loading(void)
{
  map_diskio();
  diskio_continue_resource_loading();
  unmap_cs();
}

