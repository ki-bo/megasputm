/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    0x00,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
  },
  {
    0x40,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
  },
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
    0x7c,
    0b01100110,
    0b00011000,
    0b00111100,
    0b01100110,
    0b01111110,
    0b01100110,
    0b01100110,
    0b00000000
  },
  {
    0x7d,
    0b01000010,
    0b00111100,
    0b01100110,
    0b01100110,
    0b01100110,
    0b01100110,
    0b00111100,
    0b00000000
  },
  {
    0x7e,
    0b00011100,
    0b00110110,
    0b00110110,
    0b01111100,
    0b01100110,
    0b01100110,
    0b01111100,
    0b01000000
  },
  {
    0xfc,
    0b00000001,
    0b00000011,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00111110,
    0b00000011,
    0b00000000
  },
  {
    0xfd,
    0b10000000,
    0b11000000,
    0b01100000,
    0b00110000,
    0b00011000,
    0b01111100,
    0b11000000,
    0b00000000
  },
  {
    0xfe,
    0b00000000,
    0b00000011,
    0b00111110,
    0b00011000,
    0b00001100,
    0b00000110,
    0b00000011,
    0b00000001
  },
  {
    0xff,
    0b00000000,
    0b11000000,
    0b01111100,
    0b00011000,
    0b00110000,
    0b01100000,
    0b11000000,
    0b10000000
  }
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

  // copy symbols for drawing borders
  copy_chars(0x70, 0x0d, 0x01); // symbol upper left corner
  copy_chars(0x6e, 0x0e, 0x01); // symbol upper right corner
  copy_chars(0x7d, 0x0f, 0x01); // symbol lower right corner
  copy_chars(0x6d, 0x10, 0x01); // symbol lower left corner
  copy_chars(0x5d, 0x1a, 0x01); // symbol vertical line
  copy_chars(0x40, 0x1b, 0x01); // symbol horizontal line

  // copy special characters
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
