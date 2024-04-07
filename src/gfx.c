#include "gfx.h"
#include "actor.h"
#include "costume.h"
#include "dma.h"
#include "error.h"
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

#define GFX_HEIGHT 128
#define CHRCOUNT 80
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

#pragma clang section data="data_gfx" rodata="cdata_gfx" bss="bss_gfx"
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
static uint8_t color_strip[GFX_HEIGHT];
static uint8_t __huge *next_char_data;
static uint16_t obj_first_char[MAX_OBJECTS];
static uint8_t next_obj_slot = 0;
static uint8_t num_chars_at_row[16];
static uint8_t screen_update_request = 0;
static uint8_t masking_cache_iterations[119];
static uint16_t masking_cache_data_offset[119];
static uint16_t screen_pixel_offset_x;

static dmalist_single_option_t dmalist_rle_strip_copy = {
  .opt_token      = 0x85,                   // destination skip rate
  .opt_arg        = 0x08,                   // = 8 bytes
  .end_of_options = 0x00,
  .command        = 0,                      // DMA copy command
  .count          = 0,
  .src_addr       = 0,
  .src_bank       = 0,
  .dst_addr       = 0,
  .dst_bank       = 0x07
};

static const uint16_t times_chrcount[18] = {
  CHRCOUNT * 0,
  CHRCOUNT * 1,
  CHRCOUNT * 2,
  CHRCOUNT * 3,
  CHRCOUNT * 4,
  CHRCOUNT * 5,
  CHRCOUNT * 6,
  CHRCOUNT * 7,
  CHRCOUNT * 8,
  CHRCOUNT * 9,
  CHRCOUNT * 10,
  CHRCOUNT * 11,
  CHRCOUNT * 12,
  CHRCOUNT * 13,
  CHRCOUNT * 14,
  CHRCOUNT * 15,
  CHRCOUNT * 16,
  CHRCOUNT * 17
};

//-----------------------------------------------------------------------------------------------

// Private init functions
static void setup_irq(void);
// Private interrupt function
static void raster_irq(void);
// Private gfx functions
static void decode_rle_bitmap(uint8_t *src, uint16_t width, uint8_t height);
static void decode_masking_buffer(uint8_t *src, uint16_t width, uint16_t height);
static void update_cursor(uint8_t snail_override);
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

  dmalist_rle_strip_copy.src_addr = LSB16(UNBANKED_PTR(color_strip));
  dmalist_rle_strip_copy.src_bank = BANK(UNBANKED_PTR(color_strip));

  __auto_type screen_ptr    = FAR_U16_PTR(SCREEN_RAM);
  __auto_type colram_ptr    = FAR_U16_PTR(COLRAM);
  __auto_type screen_bb_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN);
  __auto_type colram_bb_ptr = NEAR_U16_PTR(BACKBUFFER_COLRAM);

  for (uint16_t i = 0; i < CHRCOUNT * 25; ++i) {
    *screen_ptr++    = 0x0020;
    *screen_bb_ptr++ = 0x0020;
    *colram_ptr++    = 0x0f00;
    *colram_bb_ptr++ = 0x0f00;
  }

  for (uint8_t i = 0; i < 16; ++i) {
    num_chars_at_row[i] = 40;
  }

  VICIV.scrnptr   = (uint32_t)SCREEN_RAM; // implicitly sets CHRCOUNT(9..8) to 0
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.colptr    = 0x800;
  VICIV.chrcount  = CHRCOUNT;     // 80 chars per row
  VICIV.linestep  = CHRCOUNT * 2; // 160 bytes per row (2 bytes per char)

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

  VICIV.rasterline = 252;
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
  static dmalist_single_option_t dmalist_copy_gfx[2] = {
    {
      .opt_token  = 0x81,
      .opt_arg    = 0x00,
      .command    = 4, // copy + chain
      .count      = CHRCOUNT * 2 * 16,
      .src_addr   = BACKBUFFER_SCREEN + CHRCOUNT * 2 * 2,
      .src_bank   = 0x00,
      .dst_addr   = LSB16(SCREEN_RAM + CHRCOUNT * 2 * 2),
      .dst_bank   = BANK(SCREEN_RAM)
    },
    {
      .opt_token  = 0x81,
      .opt_arg    = 0xff,
      .command    = 0,
      .count      = CHRCOUNT * 2 * 16,
      .src_addr   = BACKBUFFER_COLRAM + CHRCOUNT * 2 * 2,
      .src_bank   = 0x00,
      .dst_addr   = LSB16(COLRAM + CHRCOUNT * 2 * 2),
      .dst_bank   = BANK(COLRAM)
    }
  };

  uint32_t map_save = map_get();
  unmap_all();
  if (!(VICIV.irr & 0x01)) {
    map_set(map_save);
    return;
  }

  ++raster_irq_counter;

  input_update();

  map_cs_gfx();
  if (script_watchdog < 30) {
    ++script_watchdog;
  }
  update_cursor(script_watchdog == 30);

  if (screen_update_request) {
    unmap_ds();
    dma_trigger(&dmalist_copy_gfx);
    screen_update_request = 0;
  }

  VICIV.irr = VICIV.irr; // ack interrupt
  map_set(map_save);     // restore MAP  
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
  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM) + CHRCOUNT * 2;

  uint16_t num_chars = 16 * CHRCOUNT;
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
  gfx_update_screen();
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

void gfx_clear_bg_image(void)
{
  // clear one screen worth of char data
  memset_bank(FAR_U8_PTR(BG_BITMAP), 0, 16U * 40U * 64U);

  // reset next_char_data pointer
  next_char_data = HUGE_U8_PTR(BG_BITMAP) + 16 * 40 * 2;

  // reset object pointers
  next_obj_slot = 0;
}

/**
 * @brief Decodes a room background image and stores it in the background bitmap.
 * 
 * The background bitmap is located at BG_BITMAP. The static pointer next_char_data
 * will point to the next byte following the last byte written to the background bitmap.
 * Object images will be stored as char data following the room background image
 * (starting at next_char_data).
 * 
 * @param src The encoded bitmap data in the room resource.
 * @param width The width of the bitmap in characters.
 *
 * Code section: code_gfx
 */
void gfx_decode_bg_image(uint8_t *src, uint16_t width)
{
  debug_out("Decoding bg image, width: %d\n", width)
  // when decoding a room background image, we always start over at the
  // beginning of the char data memory
  next_char_data = HUGE_U8_PTR(BG_BITMAP);

  decode_rle_bitmap(src, width, GFX_HEIGHT);
  next_obj_slot = 0;
}

void gfx_decode_masking_buffer(uint8_t *src, uint16_t width)
{
  // debug_out("Decode masking buffer, width: %d\n", width);
  uint8_t mask_col = 0;
  uint16_t num_bytes = width * (GFX_HEIGHT / 8);
  uint8_t remaining_pixels = GFX_HEIGHT;
  uint16_t mask_data_offset = 0;
  uint8_t iterations;
  uint8_t count_byte;

  while (num_bytes) {
    uint8_t count_byte = *src++;
    ++mask_data_offset;
    iterations = count_byte & 0x7f;
    //count_byte &= 0x80;
    //uint8_t data_byte = *src;
    // debug_out("  remaining pixels[%d]: %d\n", mask_col - 1, remaining_pixels);
    // debug_out("bytes left: %d fill: %d, iterations: %d, data_byte: %d @offset %d", num_bytes, count_byte!=0, iterations, data_byte, mask_data_offset);

    if (iterations > remaining_pixels) {
      num_bytes -= iterations;
      iterations -= remaining_pixels;
      if (!(count_byte & 0x80)) {
        masking_cache_iterations[mask_col] = iterations | 0x80;
        masking_cache_data_offset[mask_col] = mask_data_offset + remaining_pixels;
      }
      else {
        masking_cache_iterations[mask_col] = iterations;
        masking_cache_data_offset[mask_col] = mask_data_offset;      
      }
      // debug_out("  cache_iterations[%d] = %d (%d)\n", mask_col, masking_cache_iterations[mask_col] & 0x7f, masking_cache_iterations[mask_col]);
      // debug_out("  cache_offset[%d]     = %d\n", mask_col, masking_cache_data_offset[mask_col]);
      ++mask_col;
      remaining_pixels = GFX_HEIGHT - iterations;
    }
    else {
      num_bytes -= iterations;
      remaining_pixels -= iterations;
    }

    if (!(count_byte & 0x80)) {
      mask_data_offset += iterations;
      src += iterations;
    }
    else {
      ++mask_data_offset;
      ++src;
    }
  }
}



/**
 * @brief Decodes an object image and stores it in the char data memory.
 *
 * The static pointer next_char_data will point to the next byte following the last byte
 * written to the char data memory. That way, object char data will be stored sequentially
 * in memory. We will keep track of object IDs and their corresponding char numbers in
 * 
 * 
 * @param src Pointer to the encoded object image data.
 * @param width Width of the object image in characters.
 * @param height Height of the object image in characters.
 */
void gfx_decode_object_image(uint8_t *src, uint8_t width, uint8_t height)
{
  obj_first_char[next_obj_slot] = (uint32_t)next_char_data / 64;
  decode_rle_bitmap(src, width, height);
  ++next_obj_slot;
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
    .count    = CHRCOUNT * 4 - 2,
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
      screen_ptr = FAR_U16_PTR(SCREEN_RAM) + CHRCOUNT;
      continue;
    }
    *screen_ptr = (uint16_t)c;
    ++num_chars;
    ++screen_ptr;
  }

  return num_chars;
}

/**
 * @brief Draws the current room's background image
 *
 * The background image is drawn to the backbuffer screen memory. The horizontal
 * camera position is taken into account.
 * 
 * Code section: code_gfx
 */
void gfx_draw_bg(void)
{
  uint16_t ds_save = map_get_ds();
  unmap_ds();

  uint16_t left_char_offset = camera_x - 20;
  screen_pixel_offset_x = left_char_offset * 8;

  __auto_type screen_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2;
  uint16_t char_data = BG_BITMAP / 64 + left_char_offset * 16;

  for (uint8_t x = 0; x < 40; ++x) {
    for (uint8_t y = 0; y < 16; ++y) {
      *screen_ptr = char_data++;
      screen_ptr += CHRCOUNT;
    }
    screen_ptr -= CHRCOUNT * 16 - 1;
  }

  memset(num_chars_at_row, 40, 16);

  map_set_ds(ds_save);
}

/**
 * @brief Draws an object to the backbuffer.
 *
 * The object is drawn to the backbuffer screen memory. The coordinates x and y are
 * the position relative to the visible screen area (top/left being 0,0). The object
 * is drawn with the given width and height in characters.
 * 
 * @param local_id The local object ID (0-based position in the room's objects list)
 * @param x The x position of the object in characters.
 * @param y The y position of the object in characters.
 * @param width The width of the object in characters.
 * @param height The height of the object in characters.
 *
 * Code section: code_gfx
 */
void gfx_draw_object(uint8_t local_id, int8_t x, int8_t y, uint8_t width, uint8_t height)
{
  uint16_t *screen_ptr;
  uint16_t char_num_row = obj_first_char[local_id];
  uint16_t char_num_col;
  uint8_t width_save = width;
  int8_t col;
  int8_t row = y;
  uint8_t initial_height = height;
  uint8_t first;

  do {
    if (row >= 0 && row < 16) {
      screen_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2 + times_chrcount[row];
      width = width_save;
      col = x;
      char_num_col = char_num_row;
      first = 1;
      do {
        if (col >= 0 && col < 40) {
          if (first) {
            first = 0;
            screen_ptr += col;
          }
          *screen_ptr = char_num_col;
          ++screen_ptr;
        }
        ++col;
        char_num_col += initial_height;
      }
      while (--width);
    }
    ++row;
    ++char_num_row;
  }
  while (--height);
}

void gfx_draw_cel(int16_t xpos, int16_t ypos, uint8_t *cel_data)
{
  __auto_type hdr = (struct costume_image *)cel_data;
  cel_data += sizeof(struct costume_image);

  uint8_t width = hdr->width;
  int16_t screen_pos_x = xpos - screen_pixel_offset_x;
  if (screen_pos_x + width <= 0) {
    debug_out("out of screen %d", screen_pos_x + width);
    return;
  }
  uint8_t height = hdr->height;
  uint8_t num_char_cols = (hdr->width + 7) / 8;
  uint8_t num_char_rows = (hdr->height + 7) / 8;
  uint8_t num_pixel_lines_of_chars = num_char_rows * 8;
  uint16_t num_chars = num_char_cols * num_char_rows;
  uint16_t num_bytes = num_chars * 64;
  uint16_t char_num = (uint32_t)next_char_data / 64;
  uint8_t run_length_counter = 1;
  uint8_t current_color;
  uint16_t col_addr_inc = (num_pixel_lines_of_chars - 1) * 8;
  int16_t x = 0;
  int16_t y = 0;

  memset_bank((uint8_t __far *)next_char_data, num_bytes, 0);

  dmalist_rle_strip_copy.count = num_pixel_lines_of_chars;

  do {
    if (--run_length_counter == 0)
    {
      uint8_t data_byte = *cel_data++;
      run_length_counter = data_byte & 0x0f;
      current_color = data_byte >> 4;
      if (run_length_counter == 0)
      {
        run_length_counter = *cel_data++;
      }
    }
    color_strip[y] = current_color;
    ++y;
    if (y == height) {
      dmalist_rle_strip_copy.dst_addr = LSB16(next_char_data);
      dmalist_rle_strip_copy.dst_bank = BANK(next_char_data);
      dma_trigger(&dmalist_rle_strip_copy);
      y = 0;
      ++x;
      ++next_char_data;
      if (!(x &0x07)) {
        next_char_data += col_addr_inc;
      }
    }
  }
  while (x != width);

  unmap_ds();

  next_char_data += num_bytes;

  // place cel using rrb features
  screen_pos_x &= 0x3ff;
  uint8_t screen_pos_y = y / 8;
  __auto_type screen_left_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2;
  __auto_type colram_left_ptr = NEAR_U16_PTR(BACKBUFFER_COLRAM) + CHRCOUNT * 2;

  for (uint8_t y = 0; y < num_char_rows; ++y) {
    debug_out("screen_pos_y %d screen_left_ptr %04x num_chars_at_row %d", screen_pos_y, (uint16_t)screen_left_ptr, num_chars_at_row[screen_pos_y]);
    __auto_type screen_ptr = screen_left_ptr + num_chars_at_row[screen_pos_y];
    __auto_type colram_ptr = colram_left_ptr + num_chars_at_row[screen_pos_y];
    debug_out("gotox screen_ptr %04x colram_ptr %08lx", (uint16_t)screen_ptr, (uint32_t)colram_ptr);
    *screen_ptr++ = screen_pos_x;
    *colram_ptr++ = 0x0090;
    uint16_t cur_char = char_num;
    for (uint8_t x = 0; x < num_char_cols; ++x) {
      debug_out(" char x %d y %d screen_ptr %04x colram_ptr %08lx cur_char %04x", x, y, (uint16_t)screen_ptr, (uint32_t)colram_ptr, cur_char);
      *screen_ptr++ = cur_char;
      *colram_ptr++ = 0x0f00;
      cur_char += num_char_rows;
    }
    ++char_num;
    ++screen_pos_y;
    screen_left_ptr += CHRCOUNT;
    colram_left_ptr += CHRCOUNT;
  }

  screen_left_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2 + 40;
  colram_left_ptr = NEAR_U16_PTR(BACKBUFFER_COLRAM) + CHRCOUNT * 2 + 40;
  for (uint8_t i = 0; i < 6; ++i) {
    debug_out2("%04x.%04x ", *screen_left_ptr, *colram_left_ptr);
    ++screen_left_ptr;
    ++colram_left_ptr;
  }
  screen_update_request = 1;
  fatal_error(1);

}

/**
 * @brief Copies the backbuffer to the screen.
 *
 * The screen update is done in the interrupt routine to avoid tearing. This function will
 * request it and block until the copying is done.
 * 
 * Code section: code_gfx
 */
void gfx_update_screen(void)
{
  screen_update_request = 1;
  while (screen_update_request);
}

/** @} */ // gfx_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_private GFX Private Functions
 * @{
 */

static void decode_rle_bitmap(uint8_t *src, uint16_t width, uint8_t height)
{
  uint8_t rle_counter = 1;
  uint8_t keep_color = 0;
  uint8_t col_byte;
  uint8_t y = 0;
  uint16_t x = 0;
  uint16_t col_addr_inc = (height - 1) * 8;

  dmalist_rle_strip_copy.count = height;

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
      dmalist_rle_strip_copy.dst_addr = LSB16(next_char_data);
      dmalist_rle_strip_copy.dst_bank = BANK(next_char_data);
      dma_trigger(&dmalist_rle_strip_copy);
      y = 0;
      ++x;
      ++next_char_data;
      if (!(LSB(x) & 0x07)) {
        next_char_data += col_addr_inc;
      }
    }
  } 
  while (x != width);
}

/**
 * @brief Updates the cursor.
 *
 * This function is called every frame by the raster interrupt. It updates the cursor position and
 * appearance. It will hide and show the cursor based on VAR_CURSOR_STATE.
 *
 * Code section: code_gfx
 */
void update_cursor(uint8_t snail_override)
{
  uint8_t cursor_state = vm_read_var8(VAR_CURSOR_STATE);
  if (!(cursor_state & 0x01)) {
    VICII.spr_ena = 0x00;
    return;
  }

  uint16_t spr_pos_x = (input_cursor_x + 12) * 2 - HOTSPOT_OFFSET_X;
  uint8_t  spr_pos_y = (input_cursor_y + 50)     - HOTSPOT_OFFSET_Y;
  if (cursor_state & 0x02 && !snail_override) {
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
    .count      = CHRCOUNT * 2 * 2 - 2,
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
