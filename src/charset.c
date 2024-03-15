#include "charset.h"
#include "util.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

void charset_init(void)
{
  // make default Commodore lower case charset available
  __auto_type dest = FAR_U8_PTR(CHARSET);
  __auto_type src = FAR_U8_PTR(0x2d800UL);
  memcpy_bank(dest, src, 2048);

  // move lower case letters to the ascii range
  memcpy_bank(dest + 0x60 * 8, src, 0x20 * 8);
}
