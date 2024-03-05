#include "map.h"
#include "diskio.h"
#include "util.h"

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
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------

void unmap_cs(void)
{
  map_regs.a = 0;
  map_regs.x = 0;
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------

void unmap_ds(void)
{
  map_regs.y = 0;
  map_regs.z = 0;
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------

void map_diskio(void)
{
  map_regs.a = 0x00;
  map_regs.x = 0x21;

  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------

void map_gfx(void)
{
  map_regs.a = 0x20;
  map_regs.x = 0x21;
  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* no input operands */
         :                     /* clobber list */);
}

//-----------------------------------------------------------------------------------------------

void map_resource(uint8_t res_page)
{
  // map offset: 0x50000 + page*256 - 0x8000
  map_regs.y = res_page + 0x80;
  if ((int8_t)res_page < 0) {
    map_regs.z = 0x35;
  }
  else {
    map_regs.z = 0x34;
  }

  __asm (" map\n"
         " eom"
         :                     /* no output operands */
         : "Kq"(map_regs.quad) /* input operands */
         :                     /* clobber list */);
}
