#include <mega65/memory.h>
#include <stdio.h>

int main() {
  printf("HELLO WORLD\n");
  POKE(0xD020, 5);
  lpoke(0x40000, 0);
  unsigned char col = lpeek(0x40000);
  POKE(0xD021, col);
  return 0;
}
