#pragma once

#include <stdint.h>

uint8_t kernal_get_last_error(void);
void kernal_close_all(uint8_t device);
uint8_t kernal_set_banks(uint8_t memory, uint8_t fileName);
uint8_t kernal_set_banks_long(uint32_t memory, uint32_t fileName);
void kernal_set_logical_file(uint8_t logical, uint8_t device, uint8_t secondary);
void kernal_set_name(char *fileName, uint8_t len);
uint8_t kernal_open(void);
uint8_t kernal_set_logical_output(uint8_t logical);
uint8_t kernal_write_byte(uint8_t data);
uint8_t kernal_close_logical_file(uint8_t logical);
void kernal_reset_channels(void);

