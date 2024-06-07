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
#include "vm.h"
#include <mega65.h>
#include <string.h>
#include <stdint.h>

//-----------------------------------------------------------------------------------------------

#define GFX_HEIGHT 128
#define CHRCOUNT 120
#define SCREEN_RAM_SENTENCE (SCREEN_RAM + CHRCOUNT * 2 * 18)
#define SCREEN_RAM_VERBS (SCREEN_RAM + CHRCOUNT * 2 * 19)
#define SCREEN_RAM_INVENTORY (SCREEN_RAM + CHRCOUNT * 2 * 22)
#define UNBANKED_PTR(ptr) ((void __far *)((uint32_t)(ptr) + 0x12000UL))
#define UNBANKED_SPR_PTR(ptr) ((void *)(((uint32_t)(ptr) + 0x12000UL) / 64))

//-----------------------------------------------------------------------------------------------

#pragma clang section rodata="cdata_init"
const char palette_red[32] = {
  0x0, 0x0, 0x0, 0x0,  0xb, 0xb, 0xb, 0xb,  0x7, 0x7, 0x0, 0x0,  0xf, 0xf, 0xf, 0xf,
  0x0, 0x0, 0x0, 0x0,  0xb, 0xb, 0xb, 0xb,  0x7, 0x7, 0x0, 0x0,  0xf, 0xf, 0xf, 0xf
};
const char palette_green[32] = {
  0x0, 0x0, 0xb, 0xb,  0x0, 0x0, 0x7, 0xb,  0x7, 0x7, 0xf, 0xf,  0x8, 0x0, 0xf, 0xf,
  0x0, 0x0, 0xb, 0x0,  0x0, 0x0, 0x7, 0xb,  0x7, 0x7, 0xf, 0xf,  0x8, 0x0, 0xf, 0xf
};
const char palette_blue[32] = {
  0x0, 0xb, 0x0, 0xb,  0x0, 0xb, 0x0, 0xb,  0x7, 0xf, 0x0, 0xf,  0x8, 0xf, 0x0, 0xf,
  0x0, 0x0, 0xb, 0xb,  0x0, 0xb, 0x0, 0xb,  0x7, 0xf, 0x0, 0xf,  0x8, 0xf, 0x0, 0xf
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
static uint8_t __huge *char_data_start_actors;
static uint16_t obj_first_char[MAX_OBJECTS];
static uint8_t obj_x[MAX_OBJECTS];
static uint8_t obj_y[MAX_OBJECTS];
static uint8_t obj_width[MAX_OBJECTS];
static uint8_t obj_height[MAX_OBJECTS];
static uint8_t __huge *obj_mask_data[MAX_OBJECTS];
static uint8_t obj_draw_list[MAX_OBJECTS];
static uint8_t num_objects_drawn;
static uint8_t next_obj_slot = 0;
static uint8_t num_chars_at_row[16];
static uint8_t masking_cache_iterations[119];
static uint16_t masking_cache_data_offset[119];
static uint8_t num_masking_cache_cols = 0;
static uint8_t masking_column[GFX_HEIGHT];
static uint16_t masking_data_room_offset;
static uint32_t masking_char_data;
static uint16_t screen_pixel_offset_x;
static int16_t actor_x;
static int8_t actor_y;
static uint8_t actor_width;
static uint8_t actor_height;
static uint32_t actor_char_data;

static dmalist_three_options_no_3rd_arg_t dmalist_rle_strip_copy = {
  .opt_token1     = 0x85,                   // destination skip rate
  .opt_arg1       = 0x08,                   // = 8 bytes
  .opt_token2     = 0x86,                   // transparent color handling
  .opt_arg2       = 0x00,                   // transparent color
  .opt_token3     = 0x06,                   // enable (7) or disable (6) transparent color handling
  .end_of_options = 0x00,
  .command        = 0,                      // DMA copy command
  .count          = 0,
  .src_addr       = 0,
  .src_bank       = 0,
  .dst_addr       = 0,
  .dst_bank       = 0x07
};

static const uint16_t times_chrcount[] = {
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
  CHRCOUNT * 17,
  CHRCOUNT * 18,
  CHRCOUNT * 19,
  CHRCOUNT * 20,
  CHRCOUNT * 21,
  CHRCOUNT * 22,
  CHRCOUNT * 23,
  CHRCOUNT * 24
};

//-----------------------------------------------------------------------------------------------

// Private init functions
static void setup_irq(void);
// Private interrupt function
static void raster_irq(void);
// Private gfx functions
static uint8_t *decode_rle_bitmap(uint8_t *src, uint16_t width, uint8_t height);
static void reset_objects(void);
static void update_cursor(uint8_t snail_override);
static void set_dialog_color(uint8_t color);
static uint16_t check_next_char_data_wrap_around(uint8_t width, uint8_t height);
static void place_rrb_object(uint16_t char_num, int16_t screen_pos_x, int8_t screen_pos_y, uint8_t width_chars, uint8_t height_chars);
static void apply_actor_masking(void);
static void decode_single_mask_column(int16_t col, int8_t y_start, uint8_t num_lines);
static void decode_object_mask_column(uint8_t local_id, int16_t col, uint8_t y_start, uint8_t num_lines, uint8_t idx_dst);
static uint16_t text_style_to_color(enum text_style style);

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
  VICIV.sdbdrwd_msb  &= ~VIC4_HOTREG_MASK;
  VICII.ctrl1        &= ~0x10; // disable video output
  VICIV.palsel        = 0x00; // select and map palette 0
  VICIV.ctrla        |= 0x04; // enable RAM palette
  VICIV.ctrlb         = VIC4_VFAST_MASK;
  VICIV.ctrlc        &= ~VIC4_FCLRLO_MASK;
  VICIV.ctrlc        |= VIC4_FCLRHI_MASK | VIC4_CHR16_MASK;
  VICIV.xpos_msb     &= ~(VIC4_NORRDEL_MASK | VIC4_DBLRR_MASK);
  VICIV.tbdrpos       = 0x68;
  VICIV.textypos_lsb  = 0x67; 

  memset20(FAR_U8_PTR(BG_BITMAP), 0, 0 /* 0 means 64kb */);
  memset20(FAR_U8_PTR(COLRAM), 0, 2000);

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
  for (uint8_t i = 0; i < sizeof(palette_red); ++i) {
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
 * @defgroup gfx_runtime GFX Functions in code_main segment
 * @{
 */
#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

/// Counter increased every frame by a raster interrupt
volatile uint8_t raster_irq_counter = 0;

/**
 * @brief Raster interrupt routine.
 * 
 * This function is called every frame. It updates the cursor position and
 * appearance. A raster_irq counter is updated which will drive the timing 
 * of the non-interrupt main loop and scripts. It is also used as vsync
 * trigger to control the timing of the screen updates.
 *
 * This irq function is placed in the code section to make sure it is always
 * visible and never overlayed by other banked code or data.
 *
 * Code section: code
 */
__attribute__((interrupt()))
static void raster_irq ()
{
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
 * @brief Waits for the next jiffy timer increase.
 *
 * The jiffy counter is updated every raster irq. This function will block until the jiffy
 * counter is updated the next time.
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
void gfx_wait_vsync(void)
{
  uint8_t counter = raster_irq_counter;
  while (counter == raster_irq_counter);
}

/**
 * @brief Clears the background image.
 *
 * The pixel data memory is cleared so the background image is blank. This routine
 * is used when selecting room 0 (also called "the-void" in SCUMM).
 *
 * Code section: code_gfx
 */
void gfx_clear_bg_image(void)
{
  uint16_t num_bytes = 16U * 40U * 64U;

  // clear one screen worth of char data
  memset20(FAR_U8_PTR(BG_BITMAP), 0, num_bytes);

  // reset next_char_data pointer
  next_char_data = HUGE_U8_PTR(BG_BITMAP) + num_bytes;

  // reset object pointers
  reset_objects();
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
  //debug_out("Decoding bg image, width: %d\n", width)
  // when decoding a room background image, we always start over at the
  // beginning of the char data memory
  next_char_data = HUGE_U8_PTR(BG_BITMAP);

  decode_rle_bitmap(src, width, GFX_HEIGHT);
  
  char_data_start_actors = next_char_data;
  
  reset_objects();
}

/**
 * @brief Decodes the binary masking data after the background image.
 *
 * The masking data is stored in the room resource following the background image.
 * It is used to mask out actor pixels that should not be drawn over the background.
 * That way, actors can be placed behind background image objects by not drawing
 * their pixels where the masking buffer is set.
 *
 * The result is not cached in memory, but the masking_cache_iterations and
 * masking_cache_data_offset arrays are filled with the data for each column.
 * Those arrays allow starting the actual decoding of the masking data at
 * arbitrary columns.
 * 
 * @param bg_masking_offset Room offset to background masking data.
 * @param width Width of the masking data in characters.
 *
 * Code section: code_gfx
 */
void gfx_decode_masking_buffer(uint16_t bg_masking_offset, uint16_t width)
{
  uint16_t save_ds = map_get_ds();

  masking_data_room_offset = bg_masking_offset;
  uint8_t *src = map_ds_room_offset(bg_masking_offset);
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

    // count_byte &= 0x80;
    // uint8_t data_byte = *src;
    // debug_out("  remaining pixels[%d]: %d\n", mask_col - 1, remaining_pixels);
    // debug_out("bytes left: %d fill: %d, iterations: %d, data_byte: %d @offset %d", num_bytes, count_byte!=0, iterations, data_byte, mask_data_offset);

    if (iterations > remaining_pixels) {
      num_bytes -= iterations;
      iterations -= remaining_pixels;
      uint8_t cache_iterations = iterations;
      uint16_t cache_data_offset = mask_data_offset;
      if (count_byte & 0x80) {
        cache_iterations |= 0x80;
      }
      else {
        cache_data_offset += remaining_pixels;
      }
      masking_cache_iterations[mask_col] = cache_iterations;
      masking_cache_data_offset[mask_col] = cache_data_offset;
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
  num_masking_cache_cols = mask_col;

  map_set_ds(save_ds);
}

/**
 * @brief Decodes an object image and stores it in the char data memory.
 *
 * The static pointer next_char_data will point to the next byte following the last byte
 * written to the char data memory. That way, object char data will be stored sequentially
 * in memory. We will keep track of object IDs and their corresponding char numbers in
 * the obj_first_char array.
 *
 * This function will map DS but won't restore it. It is assumed that the caller will
 * restore the DS register after calling this function.
 * 
 * @param src Pointer to the encoded object image data.
 * @param x X scene position of the object image in pixels.
 * @param y Y scene position of the object image in characters.
 * @param width Width of the object image in characters.
 * @param height Height of the object image in characters.
 *
 * Code section: code_gfx
 */
void gfx_set_object_image(uint8_t __huge *src, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
  obj_first_char[next_obj_slot] = (uint32_t)next_char_data / 64;
  obj_x[next_obj_slot]          = x;
  obj_y[next_obj_slot]          = y;
  obj_width[next_obj_slot]      = width;
  obj_height[next_obj_slot]     = height;

  uint8_t *mapped_src = map_ds_ptr(src);
  uint8_t *mapped_mask_data = decode_rle_bitmap(mapped_src, width * 8, height * 8);
  obj_mask_data[next_obj_slot] = src + (uint16_t)(mapped_mask_data - mapped_src);

  ++next_obj_slot;
  char_data_start_actors = next_char_data;
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
 * The text is printed to the dialog area of the screen. The dialog area is located
 * at the top of the screen and is 2 lines high. The text is printed in the given color.
 *
 * If character code 0x01 is encountered in the text, the text will continue on the next
 * line.
 * 
 * @param color The color palette index of the dialog text.
 * @param text The dialog text as ASCII string.
 * @param num_chars The number of characters to print.
 *
 * Code section: code_gfx
 */
void gfx_print_dialog(uint8_t color, const char *text, uint8_t num_chars)
{
  gfx_clear_dialog();
  set_dialog_color(color);

  __auto_type screen_ptr = FAR_U16_PTR(SCREEN_RAM);
  for (uint8_t i = 0; i < num_chars; ++i) {
    char c = text[i];
    if (c == 1) {
      screen_ptr = FAR_U16_PTR(SCREEN_RAM) + CHRCOUNT;
      continue;
    }
    *screen_ptr++ = (uint16_t)c;
  }
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
  reset_objects();

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
void gfx_draw_object(uint8_t local_id, int8_t x, int8_t y)
{
  //debug_out("Obj %d at %d, %d", local_id, x, y);
  uint16_t *screen_ptr;
  uint16_t char_num_col;
  uint16_t char_num_row   = obj_first_char[local_id];
  uint8_t  width          = obj_width[local_id];
  uint8_t  height         = obj_height[local_id];
  uint8_t  initial_width  = width;
  uint8_t  initial_height = height;
  int8_t   row            = y;
  int8_t   col;
  uint8_t  first;

  obj_draw_list[num_objects_drawn++] = local_id;

  do {
    if (row >= 0 && row < 16) {
      screen_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2 + times_chrcount[row];
      width = initial_width;
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

uint8_t gfx_prepare_actor_drawing(int16_t pos_x, int8_t pos_y, uint8_t width, uint8_t height)
{
  static dmalist_t dmalist_clear_actor_chars = {
    .command  = DMA_CMD_FILL,
    .count    = 0,
    .src_addr = 0,
    .src_bank = 0,
    .dst_addr = 0,
    .dst_bank = 0
  };

  int16_t screen_pos_x = pos_x - screen_pixel_offset_x;
  if (screen_pos_x >= 320 || screen_pos_x + width < 0 || pos_y + height < 0) {
    return 0;
  }

  uint8_t actor_width_chars  = (width  + 7) >> 3;
  uint8_t actor_height_chars = (height + 7) >> 3;

  actor_x      = pos_x;
  actor_y      = pos_y;
  actor_width  = actor_width_chars  * 8;
  actor_height = actor_height_chars * 8;

  uint16_t num_bytes = check_next_char_data_wrap_around(actor_width, actor_height);
  actor_char_data = (uint32_t)next_char_data;
  next_char_data += num_bytes;

  dmalist_clear_actor_chars.count = num_bytes;
  dmalist_clear_actor_chars.dst_addr = LSB16(actor_char_data);
  dmalist_clear_actor_chars.dst_bank = BANK(actor_char_data);
  dma_trigger(&dmalist_clear_actor_chars);

  place_rrb_object(actor_char_data / 64, screen_pos_x, pos_y, actor_width_chars, actor_height_chars);
  
  return 1;
}

void gfx_draw_actor_cel(uint8_t xpos, uint8_t ypos, struct costume_cel *cel_data, uint8_t mirror)
{
  //debug_out(" cel x %d y %d width %d height %d", xpos, ypos, cel_data->width, cel_data->height);

  uint8_t width = cel_data->width;
  uint8_t height = cel_data->height;

  if (mirror) {
    xpos += width - 1;
  }

  uint32_t char_data = actor_char_data;
  if (xpos / 8) {
    char_data += xpos / 8 * actor_height * 8;
  }
  char_data += ypos * 8 + (xpos & 0x07);

  uint16_t char_data_incr = (actor_height - 1) * 8 + 1;

  uint8_t run_length_counter = 1;
  uint8_t current_color;
  int16_t x = 0;
  int16_t y = 0;
  dmalist_rle_strip_copy.count = height;
  dmalist_rle_strip_copy.opt_token3 = 0x07; // enable transparent color handling
  dmalist_rle_strip_copy.opt_arg2   = 0x00; // transparent color

  uint8_t *rle_data = ((uint8_t *)cel_data) + sizeof(struct costume_cel);
  do {
    if (--run_length_counter == 0)
    {
      uint8_t data_byte = *rle_data++;
      run_length_counter = data_byte & 0x0f;
      current_color = data_byte >> 4;
      if (current_color) {
        current_color |= 0x10;
      }
      if (run_length_counter == 0)
      {
        run_length_counter = *rle_data++;
      }
    }
    color_strip[y] = current_color;
    ++y;
    if (y == height) {
      dmalist_rle_strip_copy.dst_addr = LSB16(char_data);
      dmalist_rle_strip_copy.dst_bank = BANK(char_data);
      dma_trigger(&dmalist_rle_strip_copy);
      y = 0;
      ++x;
      if (mirror) {
        char_data -= ((uint8_t)char_data & 7) == 0 ? char_data_incr : 1;
      }
      else {
        char_data += ((uint8_t)char_data & 7) == 7 ? char_data_incr : 1;
      }
    }
  }
  while (x != width);
}

void gfx_apply_actor_masking(int16_t xpos, int8_t ypos, uint8_t masking)
{
  __auto_type next_char_data_save = next_char_data;
  uint16_t num_bytes = check_next_char_data_wrap_around(actor_width, actor_height);
  masking_char_data = (uint32_t)next_char_data;

  uint8_t  cur_x    = 0;
  uint8_t  cur_y    = 0;
  uint8_t  mask     = 0x80;
  uint16_t col_incr = (actor_height - 1) * 8;

  decode_single_mask_column(xpos / 8, ypos, actor_height);
  mask >>= xpos & 7;
  dmalist_rle_strip_copy.count = actor_height;
  dmalist_rle_strip_copy.opt_token3 = 0x06; // disable transparent color handling

  do {
    color_strip[cur_y] = (masking_column[cur_y] & mask) ? 0x00 : 0x01;
    ++cur_y;
    if (cur_y == actor_height) {
      dmalist_rle_strip_copy.dst_addr = LSB16(next_char_data);
      dmalist_rle_strip_copy.dst_bank = BANK(next_char_data);
      dma_trigger(&dmalist_rle_strip_copy);
      ++next_char_data;
      if ((((uint8_t)next_char_data) & 0x07) == 0) {
        next_char_data += col_incr;
      }
      ++cur_x;
      cur_y = 0;
      mask >>= 1;
      if (!mask) {
        decode_single_mask_column((xpos + cur_x) / 8, ypos, actor_height);
        mask = 0x80;
      }
    }
  }
  while (cur_x != actor_width);

  // copy the masking data over the actor char data
  static dmalist_two_options_no_2nd_arg_t dmalist_apply_masking = {
    .opt_token1     = 0x86,                   // transparent color handling
    .opt_arg1       = 0x01,                   // transparent color
    .opt_token2     = 0x07,                   // enable (7) or disable (6) transparent color handling
    .end_of_options = 0x00,
    .command        = 0,                      // DMA copy command
    .count          = 0,
    .src_addr       = 0,
    .src_bank       = 0,
    .dst_addr       = 0,
    .dst_bank       = 0
  };

  dmalist_apply_masking.count    = num_bytes;
  dmalist_apply_masking.src_addr = LSB16(masking_char_data);
  dmalist_apply_masking.src_bank = BANK(masking_char_data);
  dmalist_apply_masking.dst_addr = LSB16(actor_char_data);
  dmalist_apply_masking.dst_bank = BANK(actor_char_data);
  dma_trigger(&dmalist_apply_masking);

  next_char_data = next_char_data_save;
}

/**
 * @brief Finalizes drawing of all actors of the current screen.
 *
 * Needs to be called after all actors were drawn to the backbuffer. It will reposition
 * the RRB position (GOTOX) to the right edge of the screen and place an end-of-line
 * character code to indicate the RRB rendering should stop for this character row.
 *
 * Code section: code_gfx
 */
void gfx_finalize_actor_drawing(void)
{
  uint16_t save_ds = map_get_ds();
  unmap_ds();
  static uint8_t max_end_of_row = 0;

  __auto_type screen_start_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN) + CHRCOUNT * 2;
  __auto_type colram_start_ptr = NEAR_U16_PTR(BACKBUFFER_COLRAM) + CHRCOUNT * 2;
  for (uint8_t y = 0; y < 16; ++y) {
    uint8_t end_of_row = num_chars_at_row[y];
    max_end_of_row = max(max_end_of_row, end_of_row);
    if (end_of_row >= CHRCOUNT - 2) {
      fatal_error(ERR_CHRCOUNT_EXCEEDED);
    }
    __auto_type screen_ptr = screen_start_ptr + end_of_row;
    __auto_type colram_ptr = colram_start_ptr + end_of_row;
    *screen_ptr++ = 0x0140; // gotox to right screen edge (x=320)
    *colram_ptr++ = 0x0010;
    // set next row pointers
    screen_start_ptr += CHRCOUNT;
    colram_start_ptr += CHRCOUNT;
  }

  //debug_out("max_end_of_row: %d", max_end_of_row);
  map_set_ds(save_ds);
}

/**
 * @brief Resets RRB write pointers to the end of the background image.
 *
 * This function needs to be called before the actor cels are drawn to the backbuffer.
 * It will reset the position of the RRB write positions for each character row to the
 * end of the background image.
 * Any new cels drawn will therefore overwrite any previously placed RRB objects.
 * We also zeroize the colram bytes beyond the 40 background picture chars each row.
 * This is to prevent accidental gotox back into the visual area.
 *
 * Code section: code_gfx
 */
void gfx_reset_actor_drawing(void)
{
  memset20(UNBANKED_PTR(num_chars_at_row), 40, 16);

  // Next, zeroise all colram bytes beyond the 40 background picture chars each row.
  // This is to prevent the RRB to accidently do any gotox back into the visual area.
  // As we will be beyond the right edge of the screen, anyway, we don't need to 
  // worry about the screen ram values, as those chars won't be visible, anyway.
  // The actor drawing will then use these zeroed area to place their actor
  // cels into, making them visible on the screen with prepended gotox.
  static dmalist_t dmalist_reset_rrb = {
    .command        = DMA_CMD_FILL,
    .count          = (CHRCOUNT - 40) * 2,
    .src_addr       = 0,
    .src_bank       = 0,
    .dst_addr       = 0,
    .dst_bank       = 0
  };

  // start at the end of the first row of the background image
  uint16_t colram_addr = BACKBUFFER_COLRAM + CHRCOUNT * 4 + 40 * 2;
  for (uint8_t y = 0; y < 16; ++y) {
    dmalist_reset_rrb.dst_addr = colram_addr;
    DMA.addrmsb = MSB(&dmalist_reset_rrb);
    DMA.etrig_mapped = LSB(&dmalist_reset_rrb);
    colram_addr += CHRCOUNT * 2;
  }
}

/**
 * @brief Copies the backbuffer to the screen.
 *
 * Copies the backbuffer screen and color RAM to the actual screen and color RAM.
 * Should be called after gfx_wait_vsync to avoid tearing.
 * 
 * Code section: code_gfx
 */
void gfx_update_main_screen(void)
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

  dma_trigger(&dmalist_copy_gfx);
}

void gfx_print_interface_text(uint8_t x, uint8_t y, const char *name, enum text_style style)
{
  uint16_t col = text_style_to_color(style);
  uint16_t __far *screen_ptr = FAR_U16_PTR(SCREEN_RAM) + times_chrcount[y] + x;
  uint16_t __far *colram_ptr = FAR_U16_PTR(COLRAM) + times_chrcount[y] + x;
  while (*name) {
    *screen_ptr++ = *name++;
    *colram_ptr++ = col;
  }
}

void gfx_change_interface_text_style(uint8_t x, uint8_t y, uint8_t size, enum text_style style)
{
  uint16_t col = text_style_to_color(style);
  uint16_t __far *colram_ptr = FAR_U16_PTR(COLRAM) + times_chrcount[y] + x;
  while (size--) {
    *colram_ptr++ = col;
  }
}

void gfx_clear_sentence(void)
{
  static const dmalist_t dmalist_clear_sentence = {
    .command  = 0,
    .count    = 40 - 2,
    .src_addr = LSB16(SCREEN_RAM_SENTENCE),
    .src_bank = BANK(SCREEN_RAM_SENTENCE),
    .dst_addr = LSB16(SCREEN_RAM_SENTENCE + 2),
    .dst_bank = BANK(SCREEN_RAM_SENTENCE + 2),
  };

  *FAR_U16_PTR(SCREEN_RAM_SENTENCE) = 0x0020;
  dma_trigger(&dmalist_clear_sentence);
}

void gfx_clear_verbs(void)
{
  static const dmalist_t dmalist_clear_verbs = {
    .command  = 0,
    .count    = CHRCOUNT * 2 * 3 - 2,
    .src_addr = LSB16(SCREEN_RAM_VERBS),
    .src_bank = BANK(SCREEN_RAM_VERBS),
    .dst_addr = LSB16(SCREEN_RAM_VERBS + 2),
    .dst_bank = BANK(SCREEN_RAM_VERBS + 2),
  };

  *FAR_U16_PTR(SCREEN_RAM_VERBS) = 0x0020;
  dma_trigger(&dmalist_clear_verbs);
}

void gfx_clear_inventory(void)
{
  static const dmalist_t dmalist_clear_inventory = {
    .command  = 0,
    .count    = CHRCOUNT * 2 * 2 - 2,
    .src_addr = LSB16(SCREEN_RAM_INVENTORY),
    .src_bank = BANK(SCREEN_RAM_INVENTORY),
    .dst_addr = LSB16(SCREEN_RAM_INVENTORY + 2),
    .dst_bank = BANK(SCREEN_RAM_INVENTORY + 2),
  };

  *FAR_U16_PTR(SCREEN_RAM_INVENTORY) = 0x0020;
  dma_trigger(&dmalist_clear_inventory);
}

/** @} */ // gfx_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup gfx_private GFX Private Functions
 * @{
 */

static uint8_t *decode_rle_bitmap(uint8_t *src, uint16_t width, uint8_t height)
{
  uint8_t rle_counter = 1;
  uint8_t keep_color = 0;
  uint8_t col_byte;
  uint8_t y = 0;
  uint16_t x = 0;
  uint16_t col_addr_inc = (height - 1) * 8;

  dmalist_rle_strip_copy.count = height;
  dmalist_rle_strip_copy.opt_token3 = 0x06; // disable transparent color handling

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

  return src;
}

void reset_objects(void)
{
  next_obj_slot = 0;
  num_objects_drawn = 0;
}

/**
 * @brief Updates the cursor.
 *
 * This function is called every frame by the raster interrupt. It updates the cursor position and
 * appearance. It will hide and show the cursor based on ui_state flag UI_FLAGS_ENABLE_CURSOR.
 *
 * Code section: code_gfx
 */
void update_cursor(uint8_t snail_override)
{
  if (!(ui_state & UI_FLAGS_ENABLE_CURSOR)) {
    VICII.spr_ena = 0x00;
    return;
  }

  uint8_t cursor_image = snail_override ? 2 : vm_read_var8(VAR_CURSOR_STATE);
  uint16_t spr_pos_x = (input_cursor_x + 12) * 2 - HOTSPOT_OFFSET_X;
  uint8_t  spr_pos_y = (input_cursor_y + 50)     - HOTSPOT_OFFSET_Y;
  // cursor_image bit 1 = snail/regular cursor
  if (!snail_override) {
    // enable sprite 2 only = regular cursor
    VICII.spr_ena  = 0x02;
    VICII.spr1_x   = LSB(spr_pos_x);
    VICII.spr_hi_x = MSB(spr_pos_x) == 0 ? 0x00 : 0x02;
    VICII.spr1_y   = spr_pos_y;
  }
  else {
    // enable sprite 1 only = snail cursor
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

static uint16_t check_next_char_data_wrap_around(uint8_t width, uint8_t height)
{
  uint16_t num_bytes = width * height;
  if ((uint32_t)next_char_data + num_bytes > SOUND_DATA) {
    next_char_data = char_data_start_actors;
  }
  return num_bytes;
}

static void place_rrb_object(uint16_t char_num, int16_t screen_pos_x, int8_t screen_pos_y, uint8_t width_chars, uint8_t height_chars)
{
  static uint8_t row_masks[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};

  //debug_out("place rrb object: char_num %d, x %d, y %d, width %d, height %d", char_num, screen_pos_x, screen_pos_y, width_chars, height_chars);

  // place cel using rrb features
  int8_t char_row = screen_pos_y / 8;
  screen_pos_x &= 0x3ff;
  int8_t last_but_one_row = height_chars - 2;
  uint8_t shift_y = (uint8_t)screen_pos_y & 0x07;
  if (shift_y) {
    shift_y = 8 - shift_y;
  }
  else {
    --char_row;
  }
  if (char_row > 15) {
    return;
  }

  uint8_t rowmask = ~row_masks[shift_y];
  uint16_t gotox_col = (uint8_t)0x98 | (rowmask << 8);
  uint16_t gotox_scr = shift_y << 13;
  --char_num;

  uint16_t save_ds = map_get_ds();
  unmap_ds();
  
  __auto_type screen_start_ptr = NEAR_U16_PTR(BACKBUFFER_SCREEN);
  
  if (char_row >= 0) {
    uint16_t offset = times_chrcount[char_row + 2];
    screen_start_ptr += times_chrcount[char_row + 2];
  }
  __auto_type colram_start_ptr = screen_start_ptr + (BACKBUFFER_COLRAM - BACKBUFFER_SCREEN) / 2;
  for (int8_t y = -1; y < height_chars; ++y) {
    if (char_row >= 0 && char_row < 16) {
      uint8_t num_chars_cur_row = num_chars_at_row[char_row];
      __auto_type screen_ptr = screen_start_ptr + num_chars_cur_row;
      __auto_type colram_ptr = colram_start_ptr + num_chars_cur_row;
      *screen_ptr = screen_pos_x | gotox_scr;
      *colram_ptr = gotox_col;
      ++screen_ptr;
      ++colram_ptr;
      uint16_t cur_char = char_num;
      for (uint8_t x = 0; x < width_chars; ++x) {
        *screen_ptr++ = cur_char;
        *colram_ptr++ = 0x0f00;
        cur_char += height_chars;
      }
      num_chars_at_row[char_row] += width_chars + 1;
    }
   if (y == last_but_one_row) {
      rowmask = row_masks[shift_y];
      gotox_col = (uint8_t)0x98 | (rowmask << 8);
    }
    else if (y == -1) {
      gotox_col = 0x0090;
    }
    ++char_num;
    ++char_row;
    screen_start_ptr += CHRCOUNT;
    colram_start_ptr += CHRCOUNT;
  }

  map_set_ds(save_ds);
}

static void decode_single_mask_column(int16_t col, int8_t y_start, uint8_t num_lines)
{
  if (col < 0 || col > num_masking_cache_cols || y_start <= -num_lines) {
    memset20(UNBANKED_PTR(masking_column), 0, num_lines);
    return;
  }

  uint16_t save_ds = map_get_ds();
  uint8_t *data = map_ds_room_offset(masking_data_room_offset);
  uint8_t cur_mask;
  uint8_t iterations;
  uint8_t fill;
  uint8_t idx_src = 0;
  uint8_t idx_dst_start = 0;
  int8_t  y_start_save = y_start;
  uint8_t idx_dst;

  if (y_start < 0) {
    memset20(UNBANKED_PTR(masking_column), 0, -y_start);
    idx_dst_start = -y_start;
    y_start = 0;
  }
  idx_dst = idx_dst_start;

  if (col == 0) {
    iterations = data[idx_src++];
  }
  else {
    iterations = masking_cache_iterations[(uint8_t)col - 1];
    data += masking_cache_data_offset[(uint8_t)col - 1];
  }
  fill = iterations & 0x80;
  iterations &= 0x7f;

  while (y_start) {
    if (iterations <= y_start) {
      y_start -= iterations;
      if (!(fill & 0x80)) {
        idx_src += iterations;
      }
      else {
        ++idx_src;
      }
      iterations = data[idx_src++];
      fill = iterations & 0x80;
      iterations &= 0x7f;
    }
    else {
      iterations -= y_start;
      if (!(fill & 0x80)) {
        idx_src += y_start;
      }
      y_start = 0;
    }
  }
  cur_mask = data[idx_src++];

  while (idx_dst != num_lines) {
    masking_column[idx_dst++] = cur_mask;
    --iterations;
    if (!iterations) {
      iterations = data[idx_src++];
      cur_mask = data[idx_src++];
      fill = iterations & 0x80;
      iterations &= 0x7f;
    }
    else if (!fill) {
      cur_mask = data[idx_src++];
    }
  }

  for (uint8_t local_obj_id = 0; local_obj_id < num_objects_drawn; ++local_obj_id) {
    decode_object_mask_column(obj_draw_list[local_obj_id], col, y_start_save, num_lines, idx_dst_start);
  }

  map_set_ds(save_ds);
}

static void decode_object_mask_column(uint8_t local_id, int16_t col, uint8_t y_start, uint8_t num_lines, uint8_t idx_dst)
{
  uint16_t obj_x1 = obj_x[local_id];
  uint8_t  obj_x2 = obj_x1 + obj_width[local_id];

  if (obj_x1 > col || obj_x2 <= col) {
    return;
  }

  uint8_t obj_y1     = obj_y[local_id] * 8;
  uint8_t obj_y2     = obj_y1 + obj_height[local_id] * 8;
  uint8_t iterations = 1;
  uint8_t cur_mask;
  uint8_t fill;

  if (obj_y1 > y_start) {
    uint8_t diff = obj_y1 - y_start;
    if (diff >= num_lines) {
      return;
    }
    num_lines -= diff;
    idx_dst   += diff;
    y_start    = obj_y1;
  }
  
  __auto_type src = map_ds_ptr(obj_mask_data[local_id]);

  while (obj_x1 <= col) {
    for (uint8_t y = obj_y1; y < obj_y2; ++y) {
      --iterations;
      if (!iterations) {
        iterations  = *src++;
        cur_mask    = *src++;
        fill        = iterations & 0x80;
        iterations &= 0x7f;
      }
      else if (!fill) {
        cur_mask = *src++;
      }
      if (obj_x1 == col && y >= y_start) {
        masking_column[idx_dst++] = cur_mask;
        --num_lines;
        if (!num_lines) {
          return;
        }
      }
    }
    ++obj_x1;
  }
}

static uint16_t text_style_to_color(enum text_style style)
{
  switch (style) {
    case TEXT_STYLE_NORMAL:
      return 0x0200;
    case TEXT_STYLE_HIGHLIGHTED:
      return 0x0e00;
    case TEXT_STYLE_SENTENCE:
    case TEXT_STYLE_INVENTORY:
      return 0x0500;
    case TEXT_STYLE_INVENTORY_ARROW:
      return 0x0100;
    default:
      return 0x0200;
  }
}

/** @} */ // gfx_private

//-----------------------------------------------------------------------------------------------
