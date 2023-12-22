#include "util.h"
#include <mega65/debug.h>
#include <mega65/memory.h>

void fatal_error(const char *message)
{
  debug_msg(message);
  while (1) POKE(0xd020, 2);
}
