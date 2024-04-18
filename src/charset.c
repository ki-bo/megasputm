#include "charset.h"
#include "dma.h"
#include "util.h"

//-----------------------------------------------------------------------------------------------

// private charset functions
static void copy_chars(uint8_t char_idx_src, uint8_t char_idx_dst, uint16_t num_chars);

//-----------------------------------------------------------------------------------------------

#pragma clang section rodata="cdata_init"

static const uint8_t char_definitions[][9] = {
  {
    0x5b,
    0b01100110,
    0b00000000,
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b00111110,
    0b00000000
  },
  {
    0x5c,
    0b01100110,
    0b00000000,
    0b00111100,
    0b00000110,
    0b00111110,
    0b01100110,
    0b00111110,
    0b00000000
  },
  {
    0x5e,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11011011,
    0b11011011,
    0b00000000
  },
  {
    0x7b,
    0b01100110,
    0b00000000,
    0b00111100,
    0b01100110,
    0b01100110,
    0b01100110,
    0b00111100,
    0b00000000
  },
  {
    0x7e,
    0b00000000,
    0b00111000,
    0b01101100,
    0b01101100,
    0b11111000,
    0b11001100,
    0b11111000,
    0b10000000
  },
};

/**
 * @defgroup charset_init_public Charset Init Public Functions
 * @{
 */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Init charset
 *
 * Takes the default MEGA65 charset from ROM (0x2d800) and copies it to the charrom area
 * (0xff7e000). The characters are re-arranged to match the ASCII codes the game uses. 
 */
void charset_init(void)
{
  // make default Commodore lower case charset available
  copy_chars(0, 0, 256);

  // move lower case letters (index 0x01) to the ascii range (0x61)
  copy_chars(0x01, 0x61, 0x1a);

  // copy quotation mark
  copy_chars(0x22, 0x60, 0x01);

  uint8_t num_char_definitions = sizeof(char_definitions) / sizeof(char_definitions[0]);
  for (uint8_t c = 0; c < num_char_definitions; ++c) {
    uint8_t char_idx = char_definitions[c][0];
    for (uint8_t i = 0; i < 8; ++i) {
      FAR_U8_PTR(0xff7e000 + char_idx * 8)[i] = char_definitions[c][i + 1];
    }
  }
}

/** @} */ // charset_init_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup charset_init_private Charset Init Private Functions
 * @{
 */

/**
 * @brief Copies a number of characters from ROM to the charrom area
 *
 * The chars are copied from the lower case default charset in ROM (0x2d000) to the
 * charrom area (0xff7e000).
 * 
 * @param char_idx_src Index of the first character to copy from (0-255)
 * @param char_idx_dst Index of the first character to copy to (0-255)
 * @param num_chars Number of characters to copy
 */
void copy_chars(uint8_t char_idx_src, uint8_t char_idx_dst, uint16_t num_chars)
{
  static dmalist_single_option_t dmalist_charset_copy = {
    .opt_token      = 0x81,
    .opt_arg        = 0xff,
    .end_of_options = 0x00,
    .command        = 0,
    .src_addr       = 0xd800,
    .src_bank       = 0x02,
    .dst_addr       = 0xe000,
    .dst_bank       = 0x07
  };

  dmalist_charset_copy.count    = num_chars * 8;
  dmalist_charset_copy.src_addr = 0xd800 + char_idx_src * 8;
  dmalist_charset_copy.dst_addr = 0xe000 + char_idx_dst * 8;
  dma_trigger(&dmalist_charset_copy);
}

/** @} */ // charset_init_private

//-----------------------------------------------------------------------------------------------
