#include "util.h"
#include "dma.h"
#include <stdint.h>
#include <stdio.h>

//#include <mega65/debug.h>
//#include <mega65/memory.h>

char msg[80];

void fatal_error(error_code_t error)
{
  debug_out("Fatal error: %d", error);
  while (1) POKE(0xd020, 2);
}

void fatal_error_str(const char *message)
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
  global_dma_list.command  = 0;      // DMA copy command
  global_dma_list.count    = n;
  global_dma_list.src_addr = (uint16_t)src;
  global_dma_list.src_bank = 0;
  global_dma_list.dst_addr = (uint16_t)dest;
  global_dma_list.dst_bank = 0;
  dma_trigger(&global_dma_list);
  return dest;
}

void __far *memcpy_to_bank(void __far *dest, const void *src, size_t n)
{
  global_dma_list.command  = 0;      // DMA copy command
  global_dma_list.count    = n;
  global_dma_list.src_addr = (uint16_t)src;
  global_dma_list.src_bank = 0;
  global_dma_list.dst_addr = (uint16_t)dest;
  global_dma_list.dst_bank = BANK(dest);
  dma_trigger(&global_dma_list);
  return dest; 
}

void __far *memcpy_far(void __far *dest, const void __far *src, size_t n)
{
  global_dma_list_opt2.opt_token1 = 0x80;
  global_dma_list_opt2.opt_arg1   = MB(src);
  global_dma_list_opt2.opt_token2 = 0x81;
  global_dma_list_opt2.opt_arg2   = MB(dest);
  global_dma_list_opt2.command    = 0;      // DMA copy command
  global_dma_list_opt2.count      = n;
  global_dma_list_opt2.src_addr   = LSB16(src);
  global_dma_list_opt2.src_bank   = BANK(src);
  global_dma_list_opt2.dst_addr   = LSB16(dest);
  global_dma_list_opt2.dst_bank   = BANK(dest);
  dma_trigger_ext(&global_dma_list_opt2);
  return dest; 
}

void *memset(void *s, int c, size_t n)
{
  global_dma_list.command   = 0x03;      // DMA fill command
  global_dma_list.count     = n;
  global_dma_list.fill_byte = LSB(c);
  global_dma_list.dst_addr  = LSB16(s);
  global_dma_list.dst_bank  = 0;
  dma_trigger(&global_dma_list);
  return s;
}

void __far *memset_far(void __far *s, int c, size_t n)
{
  global_dma_list_opt1.opt_token = 0x81;
  global_dma_list_opt1.opt_arg   = MB(s);
  global_dma_list_opt1.command   = 0x03;      // DMA fill command
  global_dma_list_opt1.count     = n;
  global_dma_list_opt1.fill_byte = LSB(c);
  global_dma_list_opt1.dst_addr  = LSB16(s);
  global_dma_list_opt1.dst_bank  = BANK(s);
  dma_trigger_ext(&global_dma_list_opt1);
  return s;
}
