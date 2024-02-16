#include "gfx.h"
#include "util.h"
#include <mega65.h>

#define SCREEN_RAM 0xd000
#define BG_BITMAP 0x40000ul

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"
const char palette_red[16] = {
  0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0x5, 0x5, 0x5, 0x5,  0xf, 0xf, 0xf, 0xf
};
const char palette_green[16] = {
  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x5, 0xa,  0x5, 0x5, 0xf, 0xf,  0x5, 0x5, 0xf, 0xf
};
const char palette_blue[16] = {
  0x0, 0xa, 0x0, 0xa,  0x0, 0xa, 0x0, 0xa,  0x5, 0xf, 0x5, 0xf,  0x5, 0xf, 0x5, 0xf
};

void gfx_init()
{
  VICIV.scrnptr = (uint32_t)SCREEN_RAM;
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.colptr = 0x800;

  VICIV.palsel = 0xff; // select and map palette 3
  uint8_t i = 15;
  memcpy((void *)PALETTE.red, palette_red, sizeof(palette_red));
  memcpy((void *)PALETTE.green, palette_green, sizeof(palette_green));
  memcpy((void *)PALETTE.blue, palette_blue, sizeof(palette_blue));

  uint16_t *screen = (uint16_t *)SCREEN_RAM;
  uint16_t char_data = BG_BITMAP / 64;
  for (uint8_t x = 0; x < 40; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      *screen = char_data++;
      screen += 40;
    }
    screen -= 639;
  }
}

#pragma clang section text="code_gfx" rodata="cdata_gfx" data="data_gfx" bss="bss_gfx"
void gfx_test()
{
  VICIV.bordercol = COLOR_WHITE;
}

static const char dummy_data[0x1000] = {1};