#include "map.h"
#include "diskio.h"
#include "resource.h"
#include "util.h"

static inline void apply_map(void);

#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

union {
  struct {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t z;
  };
  uint32_t quad;
} map_regs = {1, 1, 1, 1}; // need to init with non-zero to force it into data section

//-----------------------------------------------------------------------------------------------

void map_init(void)
{
  map_regs.quad = 0;
}

//-----------------------------------------------------------------------------------------------

void unmap_all(void)
{
  map_regs.quad = 0;
  apply_map();
}

//-----------------------------------------------------------------------------------------------

void unmap_cs(void)
{
  map_regs.a = 0;
  map_regs.x = 0;
  apply_map();
}

//-----------------------------------------------------------------------------------------------

void unmap_ds(void)
{
  map_regs.y = 0;
  map_regs.z = 0;
  apply_map();
}

//-----------------------------------------------------------------------------------------------

void map_cs_diskio(void)
{
  map_regs.a = 0x00;
  map_regs.x = 0x21;
  apply_map();
}

//-----------------------------------------------------------------------------------------------

void map_cs_gfx(void)
{
  map_regs.a = 0x20;
  map_regs.x = 0x21;
  apply_map();}

//-----------------------------------------------------------------------------------------------

void map_ds_resource(uint8_t res_page)
{
  // map offset: 0x50000 + page*256 - 0x8000
  uint16_t offset = 0x3000 + (RESOURCE_MEMORY / 256) + res_page - 0x80;
  map_regs.y = LSB(offset);
  map_regs.z = MSB(offset);
  apply_map();
}

//-----------------------------------------------------------------------------------------------

static inline void apply_map(void)
{
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------
