#include "diskio.h"
#include <mega65/memory.h>

void waitForBusyClear(void)
{
  while (PEEK(0xd082) & 0x80);
}

void prepare_drive(void)
{
  POKE(0xd080, 0x60); // enable LED and motor
  POKE(0xd081, 0x20); // spinup command
  waitForBusyClear();
}

void diskio_init()
{
  POKE(0xd680, 0x81); // map sector data to $de00
  clear_bits(0xd689, 0x80);
}
