#include "gfx.h"
#include "dma.h"
#include "input.h"
#include "io.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <mega65.h>
#include <stdint.h>

//-----------------------------------------------------------------------------------------------

#define UNBANKED_PTR(ptr) ((void __far *)((uint32_t)(ptr) + 0x12000UL))
#define UNBANKED_SPR_PTR(ptr) ((void *)(((uint32_t)(ptr) + 0x12000UL) / 64))

//-----------------------------------------------------------------------------------------------

#pragma clang section rodata="cdata_init"
const char palette_red[16] = {
  0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0x5, 0x5, 0x5, 0x5,  0xf, 0xf, 0xf, 0xf
};
const char palette_green[16] = {
  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x5, 0xa,  0x5, 0x5, 0xf, 0xf,  0x5, 0x5, 0xf, 0xf
};
const char palette_blue[16] = {
  0x0, 0xa, 0x0, 0xa,  0x0, 0xa, 0x0, 0xa,  0x5, 0xf, 0x5, 0xf,  0x5, 0xf, 0x5, 0xf
};

//-----------------------------------------------------------------------------------------------

#pragma clang section rodata="cdata_gfx" bss="bss_gfx"
__attribute__((aligned(64))) static const uint8_t cursor_snail[] = {
  0x11,0x11,0x11,0x10,0x00,0x00,0x01,0x11,
  0x11,0x11,0x11,0x06,0x66,0x66,0x60,0x11,
  0x11,0x11,0x10,0x66,0x06,0x00,0x66,0x01,
  0x11,0xf1,0x06,0x60,0x66,0x66,0x06,0x60,
  0xf1,0x1f,0x06,0x06,0x60,0x06,0x60,0x60,
  0x1f,0x1f,0x06,0x06,0x66,0x60,0x60,0x60,
  0x1f,0x1f,0x06,0x06,0x06,0x60,0x60,0x60,
  0x1f,0x1f,0x06,0x66,0x06,0x06,0x66,0x60,
  0x1f,0xff,0x06,0x06,0x66,0x66,0x06,0x60,
  0xf6,0xff,0x06,0x60,0x60,0x00,0x66,0x01,
  0xff,0xff,0x06,0x60,0x66,0x66,0x60,0x11,
  0x11,0xff,0xf0,0x66,0x00,0x60,0x66,0x01,
  0x11,0xff,0xff,0x06,0x66,0x66,0x60,0x01,
  0x11,0x1f,0xff,0xf0,0x00,0x00,0x0f,0x11,
  0x11,0x11,0xff,0xff,0xff,0xff,0xff,0xf1,
  0x11,0x11,0x1f,0xff,0xff,0xff,0xff,0xff,
};

__attribute__((aligned(64))) static const uint8_t cursor_cross[] = {
  0b00000001,0b00000000,0b00000000,
  0b00000001,0b00000000,0b00000000,
  0b00000101,0b01000000,0b00000000,
  0b00000011,0b10000000,0b00000000,
  0b00000001,0b00000000,0b00000000,
  0b00100000,0b00001000,0b00000000,
  0b00010000,0b00010000,0b00000000,
  0b11111000,0b00111110,0b00000000,
  0b00010000,0b00010000,0b00000000,
  0b00100000,0b00001000,0b00000000,
  0b00000001,0b00000000,0b00000000,
  0b00000011,0b10000000,0b00000000,
  0b00000101,0b01000000,0b00000000,
  0b00000001,0b00000000,0b00000000,
  0b00000001,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,
}; 

//-----------------------------------------------------------------------------------------------

__attribute__((aligned(16))) static uint8_t *sprite_pointers[8];

//-----------------------------------------------------------------------------------------------

// Private init functions
static void setup_irq(void);
// Private interrupt function
static void raster_irq(void);
static void update_cursor(void);
static void set_dialog_color(uint8_t color);

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_init GFX Init Functions
 * @{
 */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Initialises the gfx module.
 *
 * This function must be called before any other gfx function.
 *
 * Code section: code_init
 */
void gfx_init()
{
  VICIV.sdbdrwd_msb &= ~VIC4_HOTREG_MASK;
  VICII.ctrl1       &= ~0x10; // disable video output
  VICIV.palsel       = 0x00; // select and map palette 0
  VICIV.ctrla       |= 0x04; // enable RAM palette
  VICIV.ctrlb        = VIC4_VFAST_MASK;
  VICIV.ctrlc       &= ~VIC4_FCLRLO_MASK;
  VICIV.ctrlc       |= VIC4_FCLRHI_MASK | VIC4_CHR16_MASK;

  memset_bank(FAR_U8_PTR(BG_BITMAP), 0, 0 /* 0 means 64kb */);
  memset_bank(FAR_U8_PTR(COLRAM), 0, 2000);

  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM);
  __auto_type colram_ptr = FAR_U16_PTR(COLRAM);

  for (uint16_t i = 0; i < 1000; ++i) {
    *screen_ptr++ = 0x0020;
    *colram_ptr++ = 0x0f00;
  }

  gfx_fade_in();

  VICIV.scrnptr   = (uint32_t)SCREEN_RAM; // implicitly sets CHRCOUNT(9..8) to 0
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.colptr    = 0x800;
  VICIV.chrcount  = 40; // 40 chars per row
  VICIV.linestep  = 80; // 80 bytes per row (2 bytes per char)

  // setup EGA palette
  for (uint8_t i = 0; i < 16; ++i) {
    PALETTE.red[i]   = palette_red[i];
    PALETTE.green[i] = palette_green[i];
    PALETTE.blue[i]  = palette_blue[i];
  }

  // setup cursors / sprites
  VICII.spr_bg_prio     = 0;                                       // sprites have priority over background
  VICII.spr_exp_x       = 0;                                       // no sprite expansion X
  VICII.spr_exp_y       = 0;                                       // no sprite expansion Y
  VICIV.ctrlc          &= ~VIC4_SPR_H640_MASK;                     // no sprite H640 mode
  VICIV.spr_hgten       = 0x03;                                    // enable variable height for sprites 0 and 1
  VICIV.spr_hght        = 16;                                      // 16 pixels high
  VICIV.spr_x64en       = 0x01;                                    // enable 8 bytes per row for sprite 0

  // charset & spr16 enable
  VICIV.charptr         = 0x1000UL;                                // will overwrite SPREN16, so set this first
  VICIV.spr_16en        = 0x01;                                    // enable 16 colors for sprite 0
  // 16 bit sprite pointers
  uint8_t rasline0_save = VICIV.rasline0;
  VICIV.spr_ptradr      = (uint32_t)UNBANKED_PTR(sprite_pointers);
  VICIV.spr_ptradr_bnk |= 0x80;                                    // enable 16 bit sprite pointers (SPRPTR16)
  VICIV.rasline0        = rasline0_save;
  
  VICIV.spr_yadj        = 0x00;                                    // no vertical adjustment
  VICIV.spr_enalpha     = 0x00;                                    // no alpha blending
  VICIV.spr_env400      = 0x00;                                    // no V400 mode
  VICIV.spr0_color      = 0x01;                                    // set transparent color for sprite 0

  sprite_pointers[0] = UNBANKED_SPR_PTR(cursor_snail);
  sprite_pointers[1] = UNBANKED_SPR_PTR(cursor_cross);

  setup_irq();
}

//-----------------------------------------------------------------------------------------------

/**
 * @brief Setup interrupt routine for the raster interrupt.
 * 
 * Code section: code_init
 */
void setup_irq(void)
{
  CPU_VECTORS.irq = &raster_irq;

  VICIV.rasterline = 250;
  VICIV.ctrl1 &= 0x7f;
  VICIV.imr = 0x01;  // enable raster interrupt
  VICIV.irr = VICIV.irr; // clear pending interrupts

  CIA1.icr = 0x7f; // disable CIA1 interrupts
  CIA2.icr = 0x7f; // disable CIA2 interrupts
  CIA1.icr; // volatile reads will ack pending irqs
  CIA2.icr;
}

/** @} */ // gfx_init

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_runtime GFX Functions in code segment
 * @{
 */
#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"

/// Counter increased every frame by a raster interrupt
volatile uint8_t raster_irq_counter = 0;

/**
 * @brief Raster interrupt routine.
 * 
 * This function is called every frame.
 *
 * Code section: code
 */
__attribute__((interrupt()))
static void raster_irq ()
{
  uint32_t map_save = map_get();

  ++raster_irq_counter;

  input_update();

  map_cs_gfx();
  update_cursor();

  map_set(map_save);     // restore MAP  
  VICIV.irr = VICIV.irr; // ack interrupt
}

/** @} */ // gfx_runtime

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_public GFX Public Functions 
 */
#pragma clang section text="code_gfx" rodata="cdata_gfx" data="data_gfx" bss="bss_gfx"

/**
 * @brief Starts the gfx module and enables video output.
 * 
 * This function must be called after gfx_init. It enabled the raster interrupt and video output.
 *
 * Code section: code_gfx
 */
void gfx_start(void)
{
  // Be careful with this register, as using rmw instructions just enabling the BLANK
  // bit will also read and write the other bits. The upper bit will read the current
  // bit8 of the current raster line, and writing it again will set the raster line
  // for the raster interrupt. This can lead to strange errors where the wrong raster line
  // might get set depending on when the write happens. To avoid all this, we just set
  // the whole byte dirctly, making sure we keep the MSB clear.
  VICII.ctrl1 = 0x1b; // enable video output
  __enable_interrupts();
}

/**
 * @brief Fades out the graphics area of the screen.
 *
 * Code section: code_gfx
 */
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

/**
 * @brief Fades in the graphics area of the screen.
 *
 * Code section: code_gfx
 */
void gfx_fade_in(void)
{
  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM) + 80;
  uint16_t char_data = BG_BITMAP / 64;

  for (uint8_t x = 0; x < 40; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      *screen_ptr = char_data++;
      screen_ptr += 40;
    }
    screen_ptr -= 639;
  }
}

/**
 * @brief Waits for the next jiffy timer interrupt.
 * 
 * @return The number of jiffies that have passed since the last call to this function.
 *
 * Code section: code_gfx
 */
uint8_t gfx_wait_for_jiffy_timer(void)
{
  static uint8_t last_raster_irq_counter = 0;
  while (last_raster_irq_counter == raster_irq_counter);
  uint8_t elapsed_jiffies = raster_irq_counter - last_raster_irq_counter;
  last_raster_irq_counter = raster_irq_counter;
  return elapsed_jiffies;
}

/**
 * @brief Waits for the next frame.
 *
 * Code section: code_gfx
 */
void gfx_wait_for_next_frame(void)
{
  uint8_t counter = raster_irq_counter;
  while (counter == raster_irq_counter);
}

/**
 * @brief Decodes a room background image and stores it in the background bitmap.
 * 
 * The background bitmap is located at BG_BITMAP.
 * 
 * @param src The encoded bitmap data in the room resource.
 * @param width The width of the bitmap in characters.
 *
 * Code section: code_gfx
 */
void gfx_decode_bg_image(uint8_t *src, uint16_t width)
{
#define GFX_STRIP_HEIGHT 128

  static dmalist_single_option_t dmalist_rle_strip_copy = {
    .opt_token      = 0x85,                   // destination skip rate
    .opt_arg        = 0x08,                   // = 8 bytes
    .end_of_options = 0x00,
    .command        = 0,                      // DMA copy command
    .count          = GFX_STRIP_HEIGHT,
    .src_addr       = 0,
    .src_bank       = 0,
    .dst_addr       = 0,
    .dst_bank       = 0x07
  };

  static uint8_t color_strip[128];

  uint8_t __huge *dst = HUGE_U8_PTR(BG_BITMAP);

  uint8_t rle_counter = 1;
  uint8_t keep_color = 0;
  uint8_t col_byte;
  uint8_t y = 0;
  uint16_t x = 0;

  dmalist_rle_strip_copy.src_addr = LSB16(UNBANKED_PTR(color_strip));
  dmalist_rle_strip_copy.src_bank = BANK(UNBANKED_PTR(color_strip));

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
    if (y == GFX_STRIP_HEIGHT) {
      dmalist_rle_strip_copy.dst_addr = LSB16(dst);
      dmalist_rle_strip_copy.dst_bank = BANK(dst);
      dma_trigger(&dmalist_rle_strip_copy);
      y = 0;
      ++x;
      ++dst;
      if (!(LSB(x) & 0x07)) {
        dst += (GFX_STRIP_HEIGHT-1) * 8;
      }
    }
  } 
  while (x != width);
}

/**
 * @brief Clears the dialog area of the screen.
 *
 * Code section: code_gfx
 */
void gfx_clear_dialog(void)
{
  static const dmalist_t dmalist_clear_dialog_screen = {
    .command  = 0,
    .count    = 80 * 2 - 2,
    .src_addr = LSB16(SCREEN_RAM),
    .src_bank = BANK(SCREEN_RAM),
    .dst_addr = LSB16(SCREEN_RAM + 2),
    .dst_bank = BANK(SCREEN_RAM + 2),
  };

  *FAR_U16_PTR(SCREEN_RAM) = 0x0020;
  dma_trigger(&dmalist_clear_dialog_screen);
}

/**
 * @brief Prints a dialog to the screen.
 * 
 * @param color The color palette index of the dialog text.
 * @param text The dialog text as null-terminated ASCII string.
 * @return The number of characters printed.
 *
 * Code section: code_gfx
 */
uint8_t gfx_print_dialog(uint8_t color, const char *text)
{
  gfx_clear_dialog();
  set_dialog_color(color);

  uint8_t num_chars = 0;
  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM);
  for (uint8_t i = 0; i < 80; ++i) {
    char c = text[i];
    if (c == '\0') {
      break;
    }
    else if (c == 1) {
      screen_ptr = FAR_U16_PTR(SCREEN_RAM) + 40;
      continue;
    }
    *screen_ptr = (uint16_t)c;
    ++num_chars;
    ++screen_ptr;
  }

  return num_chars;
}

/** @} */ // gfx_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_private GFX Private Functions
 * @{
 */

/**
 * @brief Updates the cursor.
 *
 * This function is called every frame by the raster interrupt. It updates the cursor position and
 * appearance. It will hide and show the cursor based on VAR_CURSOR_STATE.
 *
 * Code section: code_gfx
 */
void update_cursor()
{
  uint8_t cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  if (!(cursor_state & 0x01)) {
    VICII.spr_ena = 0x00;
    return;
  }

  uint16_t spr_pos_x = input_cursor_x * 2 - HOTSPOT_OFFSET_X;
  uint8_t  spr_pos_y = input_cursor_y - HOTSPOT_OFFSET_Y;
  if (cursor_state & 0x02) {
    VICII.spr_ena  = 0x02;
    VICII.spr1_x   = LSB(spr_pos_x);
    VICII.spr_hi_x = MSB(spr_pos_x) == 0 ? 0x00 : 0x02;
    VICII.spr1_y   = spr_pos_y;
  }
  else {
    VICII.spr_ena  = 0x01;
    VICII.spr0_x   = LSB(spr_pos_x);
    VICII.spr_hi_x = MSB(spr_pos_x) == 0 ? 0x00 : 0x01;
    VICII.spr0_y   = spr_pos_y;
  }


  static const uint8_t cursor_color_rotate[] = { 8, 7, 15, 7 };
  static uint8_t cursor_color_index = 0;
  static uint8_t wait_frames = 8;

  VICII.spr1_color = cursor_color_rotate[cursor_color_index];
  if (--wait_frames == 0) {
    wait_frames = 8;
    ++cursor_color_index;
    if (cursor_color_index == sizeof(cursor_color_rotate)) {
      cursor_color_index = 0;
    }
  }
}

/**
 * @brief Sets the color of the dialog text.
 * 
 * Fills the colram of the dialog area with the given color.
 *
 * @param color The color palette index.
 *
 * Code section: code_gfx
 */
void set_dialog_color(uint8_t color)
{
  static const dmalist_two_options_t dmalist_clear_dialog_colram = {
    .opt_token1 = 0x80,
    .opt_arg1   = 0xff,
    .opt_token2 = 0x81,
    .opt_arg2   = 0xff,
    .command    = 0,
    .count      = 80 * 2 - 2,
    .src_addr   = LSB16(COLRAM),
    .src_bank   = BANK(COLRAM),
    .dst_addr   = LSB16(COLRAM + 2),
    .dst_bank   = BANK(COLRAM + 2)
  };

  __auto_type ptr = FAR_U8_PTR(COLRAM);
  *ptr = 0;
  *(ptr+1) = color;
  dma_trigger(&dmalist_clear_dialog_colram);
}

/** @} */ // gfx_private

//-----------------------------------------------------------------------------------------------
