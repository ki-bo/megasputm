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

#pragma once

#include <mega65.h>
#include <stdint.h>

struct __cpu_vectors {
  void *nmi;
  void *reset;
  void *irq;
};

struct __f011 {
  uint8_t fdc_control;
  uint8_t command;
  uint16_t status;
  uint8_t track;
  uint8_t sector;
  uint8_t side;
  uint8_t data;
  uint8_t clock;
  uint8_t step;
  uint8_t pcode;
};

enum fdc_status_mask_t
{
  /** Drive select (0 to 7). Internal drive is 0. Second floppy drive on internal cable 
      is 1. Other values reserved for C1565 external drive interface. */
  FDC_DS_MASK = 0b00000111,
  /** Directly controls the SIDE signal to the floppy drive, i.e., selecting which side of the media is active. */
  FDC_SIDE_MASK = 0b00001000,
  /** Swap upper and lower halves of data buffer (i.e. invert bit 8 of the sector buffer) */
  FDC_SWAP_MASK = 0b00010000,
  /** Activates drive motor and LED (unless LED signal is also set, causing the drive LED to blink) */
  FDC_MOTOR_MASK = 0b00100000,
  /** Drive LED blinks when set */
  FDC_LED_MASK = 0b01000000,
  /** The floppy controller has generated an interrupt (read only). Note that in- terrupts are not currently implemented on the 45GS27. */
  FDC_IRQ_MASK = 0b10000000,
  /** F011 Head is over track 0 flag (read only) */
  FDC_TK0_MASK = 0x0001,
  /** F011 FDC CRC check failure flag (read only) */
  FDC_CRC_MASK = 0x0008,
  /** F011 FDC Request Not Found (RNF), i.e., a sector read or write operation did not find the requested sector (read only) */
  FDC_RNF_MASK = 0x0010,
  /** F011 FDC CPU and disk pointers to sector buffer are equal, indicating that the sector buffer is either full or empty. (read only) */
  FDC_EQ_MASK = 0x0020,
  /** F011 FDC DRQ flag (one or more bytes of data are ready) (read only) */
  FDC_DRQ_MASK = 0x0040,
  /** F011 FDC busy flag (command is being executed) (read only) */
  FDC_BUSY_MASK = 0x0080,
  /** F011 Write Request flag, i.e., the requested sector was found during a write operation (read only) */
  FDC_WTREQ_MASK = 0x4000,
  /** F011 Read Request flag, i.e., the requested sector was found during a read operation (read only) */
  FDC_RDREQ_MASK = 0x8000
};

enum fdc_command_t
{
  FDC_CMD_CLR_BUFFER_PTRS = 0x01,
  FDC_CMD_STEP_OUT = 0x10,
  FDC_CMD_STEP_IN = 0x18,
  FDC_CMD_SPINUP = 0x20,
  FDC_CMD_READ_SECTOR = 0x40,
  FDC_CMD_WRITE_SECTOR = 0x84
};

typedef union adma24_value_t
{
  struct {
    uint16_t addr16;
    uint8_t  bank;
  };
  struct {
    uint16_t lsb16;
    uint8_t  msb;
  };
  struct
  {
    uint8_t l; // lsb
    uint8_t c; // middle byte
    uint8_t m; // msb
  };
} adma24_value_t;

struct __audio_channel {
  uint8_t ctrl;
  adma24_value_t base_addr;
  adma24_value_t freq;
  uint16_t top_addr;
  uint8_t volume;
  adma24_value_t current_addr;
  adma24_value_t timing_ctr;
};

struct __dma {
  uint8_t addrlsbtrig;
  uint8_t addrmsb;
  uint8_t addrbank;
  uint8_t en018b;
  uint8_t addrmb;
  uint8_t etrig;
  uint8_t etrig_mapped;
  uint8_t trig_inline;
  uint8_t unused1[9];
  uint8_t aud_ctrl;
  uint8_t unused2[10];
  uint8_t aud_ch_pan_vol[4];
  struct __audio_channel aud_ch[4];
};

enum dma_audio_ctrl_mask_t
{
  /** Enable Audio DMA */
  ADMA_EN_MASK = 0b10000000,
  /** Audio DMA blocked (read only) */
  AMDA_BLKD_MASK = 0b01000000,
  /** Audio DMA block writes (samples still get read) */
  ADMA_WRBLK_MASK = 0b00100000,
  /** Audio DMA bypasses audio mixer */
  ADMA_NOMIX_MASK = 0b00010000,
  /** Audio DMA block timeout (read only) DEBUG */
  ADMA_BLKTO_MASK = 0b00000111
};

enum dma_channel_ctrl_mask_t
{
  /** Enable Audio DMA channel */
  ADMA_CHEN_MASK = 0b10000000,
  /** Enable Audio DMA channel looping */
  ADMA_CHLOOP_MASK = 0b01000000,
  /** Enable Audio DMA channel unsigned samples */
  ADMA_CHUSGN_MASK = 0b00100000,
  /** Audio DMA channel play 32-sample sine wave instead of DMA data */
  ADMA_CHSINE_MASK = 0b00010000,
  /** Audio DMA channel stop flag */
  ADMA_CHSTP_MASK = 0b00001000,
  /** Audio DMA channel sample bits (11=16, 10=8, 01=upper nybl, 00=lower nybl) */
  ADMA_CHSBITS_MASK = 0b00000011
};

enum {
  ADMA_SBITS_16 = 0b11,
  ADMA_SBITS_8  = 0b10,
  ADMA_SBITS_4U = 0b01,
  ADMA_SBITS_4L = 0b00
};

struct __pot {
  uint8_t x;
  uint8_t y;
};

#define CPU_VECTORS (*(volatile struct __cpu_vectors *) 0xfffa)
#define FDC         (*(volatile struct __f011 *)        0xd080)
#define POT         (*(volatile struct __pot *)         0xd419)
#define ASCIIKEY    (*(volatile uint8_t *)              0xd610)
#define DMA         (*(volatile struct __dma *)         0xd700)
#define RNDGEN      (*(volatile uint8_t *)              0xd7ef)
#define RNDRDY      (*(volatile uint8_t *)              0xd7fe)
