#include "diskio.h"
#include <mega65/memory.h>

int main() {
  mega65_io_enable();
  diskio_init();
  return 0;
}
