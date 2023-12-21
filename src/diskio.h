#include <stdint.h>

void diskio_init(void);
void diskio_read_sector(uint8_t *buf, uint32_t sector);
