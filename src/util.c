#include "util.h"
#include "dma.h"
#include <stdint.h>
#include <stdio.h>

//#include <mega65/debug.h>
//#include <mega65/memory.h>

char msg[80];

void fatal_error(const char *message)
{
  debug_msg((char *)message);
  while (1) POKE(0xd020, 2);
}

void debug_msg(char* msg)
{
  while (*msg) {
    __asm volatile(" sta 0xd643\n"
                   " nop"
                   :           /* no output operands */
                   : "Ka"(*msg) /* input operands */
                   : "a" /* clobber list */);
    msg++;
  }
  __asm volatile(" lda #0x0d\n"
                  " sta 0xd643\n"
                  " nop\n"
                  " lda #0x0a\n"
                  " sta 0xd643\n"
                  " nop"
                  : /* no output operands */
                  : /* no input operands*/
                  : "a" /* clobber list */);
}


void *memcpy(void *dest, const void *src, size_t n)
{
  static dmalist_t dmalist_copy = {
    .command = 0x00,      //!< DMA copy command
    .count = 0x0000,
    .src_addr = 0x0000,
    .src_bank = 0x00,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  dmalist_copy.count    = n;
  dmalist_copy.src_addr = (uint16_t)src;
  dmalist_copy.dst_addr = (uint16_t)dest;

  DMA.addrbank    = 0;
  DMA.addrmsb     = MSB(&dmalist_copy);
  DMA.addrlsbtrig = LSB(&dmalist_copy);

  return dest;
}

void __far *memcpy_to_bank(void __far *dest, const void *src, size_t n)
{
  static dmalist_t dmalist_copy = {
    .command = 0x00,      //!< DMA copy command
    .count = 0x0000,
    .src_addr = 0x0000,
    .src_bank = 0x00,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  dmalist_copy.count    = n;
  dmalist_copy.src_addr = (uint16_t)src;
  dmalist_copy.dst_bank = BANK(dest);
  dmalist_copy.dst_addr = (uint16_t)dest;

  DMA.addrbank = 0;
  DMA.addrmsb  = MSB(&dmalist_copy);
  DMA.etrig    = LSB(&dmalist_copy); 

  return dest; 
}

void __far *memcpy_far(void __far *dest, const void __far *src, size_t n)
{
  static dmalist_t dmalist_copy = {
    .command = 0x00,      //!< DMA copy command
    .count = 0x0000,
    .src_addr = 0x0000,
    .src_bank = 0x00,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  dmalist_copy.count    = n;
  dmalist_copy.src_bank = BANK(src);
  dmalist_copy.src_addr = (uint16_t)src;
  dmalist_copy.dst_bank = BANK(dest);
  dmalist_copy.dst_addr = (uint16_t)dest;

  DMA.addrbank = 0;
  DMA.addrmsb  = MSB(&dmalist_copy);
  DMA.etrig    = LSB(&dmalist_copy); 

  return dest; 
}

void memcpy_to_io(__far void *dest, const void *src, size_t n)
{
  static dmalist_single_option_t dmalist_copy = {
    .opt_token = 0x81,
    .opt_arg = 0xff,
    .end_of_options = 0x00,
    .command = 0x00,      //!< DMA copy command
    .count = 0x0000,
    .src_addr = 0x0000,
    .src_bank = 0x00,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  dmalist_copy.count    = n;
  dmalist_copy.src_addr = (uint16_t)src;
  dmalist_copy.dst_bank = BANK(dest);
  dmalist_copy.dst_addr = (uint16_t)dest;

  DMA.addrbank = 0;
  DMA.addrmsb  = MSB(&dmalist_copy);
  DMA.etrig    = LSB(&dmalist_copy);
}

void *memset(void *s, int c, size_t n)
{
  static dmalist_t dmalist_fill = {
    .command = 0x03,      //!< DMA fill command
    .src_bank = 0x00,
    .dst_bank = 0x00
  };

  dmalist_fill.count     = n;
  dmalist_fill.fill_byte = c & 0xff;
  dmalist_fill.dst_addr  = (uint16_t)s;

  DMA.addrbank    = 0;
  DMA.addrmsb     = MSB(&dmalist_fill);
  DMA.addrlsbtrig = LSB(&dmalist_fill);

  return s;
}
