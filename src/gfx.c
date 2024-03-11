#include "gfx.h"
#include "io.h"
#include "resource.h"
#include "util.h"
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <stdint.h>

#define SCREEN_RAM 0x10000
#define COLRAM 0xff80800UL
#define BG_BITMAP 0x28000
#define FRAMECOUNT (*(volatile uint8_t *)0xd7fa)

/**
 * @defgroup gfx_init GFX Init Functions
 * @{
 */
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

// Private init functions
static void setup_irq(void);
// Private interrupt function
static void raster_irq(void);

/**
 * @brief Initialises the gfx module.
 *
 * This function must be called before any other gfx function.
 *
 * Code section: code_init
 */
void gfx_init()
{
  VICIV.palsel = 0x00; // select and map palette 0
  VICIV.ctrla |= 0x04; // enable RAM palette
  VICIV.ctrlb = VIC4_VFAST_MASK;
  VICIV.ctrlc &= ~VIC4_FCLRLO_MASK;
  VICIV.ctrlc |= VIC4_FCLRHI_MASK | VIC4_CHR16_MASK;

  memset_bank(FAR_U8_PTR(BG_BITMAP), 0, 0 /* 0 means 64kb */);
  memset_bank(FAR_U8_PTR(COLRAM), 0, 2000);

  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM);
  __auto_type colram_ptr = FAR_U16_PTR(COLRAM);
  uint16_t char_data = BG_BITMAP / 64;

  for (uint16_t i = 0; i < 1000; ++i) {
    *screen_ptr++ = 0x0020;
    *colram_ptr++ = 0x0f00;
  }

  // Fill chars with initial fcm pattern starting at 3rd row (offset 80 chars/words)
  screen_ptr = FAR_U16_PTR(SCREEN_RAM) + 80;
  colram_ptr = FAR_U16_PTR(COLRAM) + 80;

  for (uint8_t x = 0; x < 40; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      *screen_ptr = char_data++;
      screen_ptr += 40;
    }
    screen_ptr -= 639;
  }

  VICIV.scrnptr = (uint32_t)SCREEN_RAM; // implicitly sets CHRCOUNT(9..8) to 0
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.colptr = 0x800;
  VICIV.chrcount = 40; // 40 chars per row
  VICIV.linestep = 80; // 80 bytes per row (2 bytes per char)


  int8_t i = 15;
  do {
    PALETTE.red[i] = palette_red[i];
    PALETTE.green[i] = palette_green[i];
    PALETTE.blue[i] = palette_blue[i];
  }
  while (--i >= 0);

  setup_irq();
}

void setup_irq(void)
{
  *NEAR_U16_PTR(0xfffe) = (uint16_t)&raster_irq;

  VICIV.rasterline = 250;
  VICIV.ctrl1 &= 0x7f;
  VICIV.imr = 0x01;  // enable raster interrupt
  VICIV.irr = VICIV.irr; // clear pending interrupts

  CIA1.icr = 0x7f; // disable CIA1 interrupts
  CIA2.icr = 0x7f; // disable CIA2 interrupts
  CIA1.icr; // volatile reads will ack pending irqs
  CIA2.icr;

  //ETHERNET.ctrl1 &= ~(ETH_RXQEN_MASK | ETH_TXQEN_MASK); // dsiable ethernet interrupts

  __enable_interrupts();
}

#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

volatile uint8_t raster_irq_counter = 0;

__attribute__((interrupt()))
static void raster_irq ()
{
  ++raster_irq_counter;
  VICIV.irr = VICIV.irr; // ack interrupt
}

//*****************************************************************************
// Function definitions, code_gfx
//*****************************************************************************
#pragma clang section text="code_gfx" rodata="cdata_gfx" data="data_gfx" bss="bss_gfx"

void gfx_fade_out(void)
{
  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM) + 80;

  uint16_t num_chars = 16*40;
  do {
    *screen_ptr = 0x0020;
    ++screen_ptr;
  }
  while (--num_chars != 0);
}

uint8_t gfx_wait_for_jiffy_timer(void)
{
  static uint8_t last_raster_irq_counter = 0;
  while (last_raster_irq_counter == raster_irq_counter);
  uint8_t elapsed_jiffies = raster_irq_counter - last_raster_irq_counter;
  last_raster_irq_counter = raster_irq_counter;
  return elapsed_jiffies;
}

void gfx_wait_for_next_frame(void)
{
  uint8_t counter = raster_irq_counter;
  while (counter == raster_irq_counter);
}

void gfx_decode_bg_image(uint8_t *src, uint16_t width)
{
  static uint8_t color_strip[128];

  uint8_t __huge *dst = HUGE_U8_PTR(BG_BITMAP);
  const uint8_t height = 128;

  uint8_t rle_counter = 1;
  uint8_t keep_color = 0;
  uint8_t col_byte;
  uint8_t y = 0;
  uint16_t x = 0;

  do {
    --rle_counter;
    if (rle_counter == 0) {
      col_byte = *src++;
      keep_color = col_byte & 0x80;
      if (keep_color) {
        rle_counter = col_byte & 0x7f;
      }
      else {
        rle_counter = col_byte >> 4;
        col_byte &= 0x0f;
      }
      if (rle_counter == 0) {
        rle_counter = *src++;
      }
    }
    if (!keep_color) {
      color_strip[y] = col_byte;
    }

    ++y;
    if (y == height) {
      uint8_t __huge *strip_ptr = dst;
      for (y = 0; y < height; ++y) {
        *strip_ptr = color_strip[y];
        strip_ptr = strip_ptr + 8;
      }
      y = 0;
      ++x;
      dst = dst + 1;
      if (!(x & 0x07)) {
        dst = dst + (8 * (uint16_t)(height - 1));
      }
    }
  } 
  while (x != width);
}
