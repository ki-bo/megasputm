#include <stdint.h>

void init_diskio(void);
void diskio_read_sector(uint8_t *buf, uint32_t sector);
