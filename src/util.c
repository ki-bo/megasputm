#include "util.h"
#include "dma.h"
#include "diskio.h"
#include "map.h"
#include <stdint.h>
#include <stdio.h>

char msg[80];

void fatal_error(error_code_t error)
{
  debug_out("Fatal error: %d", error);
  //POKE(0xd020, 5);
  map_cs_diskio();
  while (1) {
    diskio_check_motor_off();
  }
}

void fatal_error_str(const char *message)
{
  debug_msg((char *)message);
  POKE(0xd020, 5);
  map_cs_diskio();
  while (1) {
    diskio_check_motor_off();
  }
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

void __far *memcpy_bank(void __far *dest, const void __far *src, size_t n)
{
  global_dma_list.command    = 0;      // DMA copy command
  global_dma_list.count      = n;
  global_dma_list.src_addr   = LSB16(src);
  global_dma_list.src_bank   = BANK(src);
  global_dma_list.dst_addr   = LSB16(dest);
  global_dma_list.dst_bank   = BANK(dest);
  dma_trigger(&global_dma_list);
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

void __far *memset_bank(void __far *s, int c, size_t n)
{
  global_dma_list.command   = 0x03;      // DMA fill command
  global_dma_list.count     = n;
  global_dma_list.fill_byte = LSB(c);
  global_dma_list.dst_addr  = LSB16(s);
  global_dma_list.dst_bank  = BANK(s);
  dma_trigger(&global_dma_list);
  return s;
}
