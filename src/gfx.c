#include "gfx.h"
#include "io.h"
#include "util.h"
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#define SCREEN_RAM 0x2a000
#define COLRAM 0xff80800UL
#define BG_BITMAP 0x40000ul
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

  memset_far(FAR_U8_PTR(BG_BITMAP), 0, 0xffff);
  memset_far(FAR_U8_PTR(COLRAM), 0, 2000);

  __auto_type screen = FAR_U16_PTR(SCREEN_RAM);
  uint16_t char_data = BG_BITMAP / 64;
  for (uint8_t x = 0; x < 40; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      *screen = char_data++;
      screen += 40;
    }
    screen -= 639;
  }

  VICIV.scrnptr = (uint32_t)SCREEN_RAM;
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.colptr = 0x800;

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

  ETHERNET.ctrl1 &= ~(ETH_RXQEN_MASK | ETH_TXQEN_MASK); // dsiable ethernet interrupts

  __enable_interrupts();
}

//*****************************************************************************
// Function definitions, code_gfx
//*****************************************************************************
#pragma clang section text="code_gfx" rodata="cdata_gfx" data="data_gfx" bss="bss_gfx"

void gfx_test()
{
  VICIV.bordercol = COLOR_WHITE;
}

#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

uint8_t raster_irq_counter = 0;

__attribute__((interrupt()))
static void raster_irq ()
{
  ++raster_irq_counter;
  VICIV.irr = VICIV.irr; // ack interrupt
}

uint8_t wait_for_raster_irq(void)
{
  uint8_t counter = 0;
  while (counter == 0) {
    __disable_interrupts();
    counter = raster_irq_counter;
    raster_irq_counter = 0;
    __enable_interrupts();
  }
  return counter;
}
