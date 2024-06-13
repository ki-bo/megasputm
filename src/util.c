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
    diskio_check_motor_off(1);
  }
}

void debug_msg(char* msg)
{
#ifdef DEBUG
  while (*msg) {
    __asm (" sta 0xd643\n"
           " clv"
           :           /* no output operands */
           : "Ka"(*msg) /* input operands */
           : "a" /* clobber list */);
    msg++;
  }
  __asm (" lda #0x0d\n"
         " sta 0xd643\n"
         " clv\n"
         " lda #0x0a\n"
         " sta 0xd643\n"
         " clv"
         : /* no output operands */
         : /* no input operands*/
         : "a" /* clobber list */);
#endif
}

void debug_msg2(char* msg)
{
#ifdef DEBUG
  while (*msg) {
    __asm (" sta 0xd643\n"
           " clv"
           :           /* no output operands */
           : "Ka"(*msg) /* input operands */
           : "a" /* clobber list */);
    msg++;
  }
#endif
}

void __far *memcpy_to_bank(void __far *dest, const void *src, size_t n)
{
  global_dma.no_opt.end_of_options = 0;
  global_dma.no_opt.command        = 0;      // DMA copy command
  global_dma.no_opt.count          = n;
  global_dma.no_opt.src_addr       = (uint16_t)src;
  global_dma.no_opt.src_bank       = 0;
  global_dma.no_opt.dst_addr       = (uint16_t)dest;
  global_dma.no_opt.dst_bank       = BANK(dest);
  dma_trigger_global();
  return dest; 
}

void __far *memcpy_bank(void __far *dest, const void __far *src, size_t n)
{
  global_dma.no_opt.end_of_options = 0;
  global_dma.no_opt.command        = 0;      // DMA copy command
  global_dma.no_opt.count          = n;
  global_dma.no_opt.src_addr       = LSB16(src);
  global_dma.no_opt.src_bank       = BANK(src);
  global_dma.no_opt.dst_addr       = LSB16(dest);
  global_dma.no_opt.dst_bank       = BANK(dest);
  dma_trigger_global();
  return dest;
}

void __far *memcpy_far(void __far *dest, const void __far *src, size_t n)
{
  __asm(" lda #0\n"
        " sta zp:global_dma+4\n" // end_of_options
        " sta zp:global_dma+5\n" // command
        " sta 0xd701\n"
        " lda #0x80\n"
        " sta zp:global_dma\n"
        " inc a\n"
        " sta zp:global_dma+2\n"
        " ldq zp:_Zp+4\n"       // src address
        " stq zp:global_dma+8\n"
        " aslq q\n"
        " aslq q\n"
        " aslq q\n"
        " aslq q\n"
        " stz zp:global_dma+1\n"
        " ldq zp:_Zp\n"         // dest address
        " stq zp:global_dma+11\n"
        " aslq q\n"
        " aslq q\n"
        " aslq q\n"
        " aslq q\n"
        " stz zp:global_dma+3\n"
        " lda #0xf0\n"
        " trb zp:global_dma+10\n"
        " trb zp:global_dma+13\n"
        " ldz #0\n"
        " ldq (zp:_Vsp),z\n"
        " sta zp:global_dma+6\n"   // count
        " stx zp:global_dma+7\n"
        " lda #.byte0 global_dma\n"
        " sta 0xd705\n"
        :
        :
        : "a", "x", "y", "z"
  );
  return 0;
}

void __far *memset20(void __far *s, int c, size_t n)
{
  global_dma.no_opt.end_of_options = 0;
  global_dma.no_opt.command        = 0x03;      // DMA fill command
  global_dma.no_opt.count          = n;
  global_dma.no_opt.fill_byte      = LSB(c);
  global_dma.no_opt.dst_addr       = LSB16(s);
  global_dma.no_opt.dst_bank       = BANK(s);
  dma_trigger_global();
  return s;
}

void __far *memset32(void __far *s, uint32_t c, size_t n)
{
  global_dma.single_opt.end_of_options = 0;
  global_dma.single_opt.opt_token      = 0x81;
  global_dma.single_opt.opt_arg        = 0xff;
  global_dma.single_opt.command        = 0x03;      // DMA fill command
  global_dma.single_opt.count          = n;
  global_dma.single_opt.fill_byte      = LSB(c);
  global_dma.single_opt.dst_addr       = LSB16(s);
  global_dma.single_opt.dst_bank       = BANK(s);
  dma_trigger_global();
  return s;
}
