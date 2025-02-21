#pragma once

#include <stdint.h>

typedef uint8_t err_t;

void hdos_init(uint16_t dataBuf, uint16_t xferBuf);

err_t hdos_set_filename(const char *fileName);
err_t hdos_open_file(void);
void hdos_close_file(void);
err_t hdos_read_byte(uint8_t *data);
void hdos_close_dir(uint8_t desc);
err_t hdos_open_dir(uint8_t *desc);
err_t hdos_read_dir(uint8_t desc);
err_t hdos_change_dir(void);
err_t hdos_load_file_attic(uint32_t offset);