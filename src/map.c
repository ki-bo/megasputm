#include "map.h"
#include "diskio.h"
#include "util.h"

#pragma clang section text="code"
void unmap_all(void)
{
  __asm (" lda #0x00\n"
         " tax\n"
         " tay\n"
         " taz\n"
         " map\n"
         " eom"
         :                 /* no output operands */
         :                 /* no input operands */
         : "a","x","y","z" /* clobber list */);
}

void map_diskio(void)
{
  __asm (" lda #0x00\n"
         " ldx #0x21\n"
         " ldy #0x00\n"
         " ldz #0x00\n"
         " map\n"
         " eom"
         :                 /* no output operands */
         :                 /* no input operands */
         : "a","x","y","z" /* clobber list */);
}
