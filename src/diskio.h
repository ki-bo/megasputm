#pragma once

#include <stdint.h>

enum {
    FILE_TYPE_SEQ = 0x81,
    FILE_TYPE_PRG = 0x82
};

// code_init functions
void diskio_init(void);

// code_diskio functions
void diskio_switch_to_real_drive(void);
void diskio_check_motor_off(uint8_t elapsed_jiffies);
uint8_t diskio_file_exists(const char *filename);
void diskio_load_file(const char *filename, uint8_t __far *address);
void diskio_load_game_objects(void);
uint16_t diskio_start_resource_loading(uint8_t type, uint8_t id);
void diskio_continue_resource_loading(uint8_t __huge *target_ptr);
void diskio_open_for_reading(const char *filename, uint8_t file_type);
void diskio_read(uint8_t *target_ptr, uint16_t size);
void diskio_close_for_reading(void);
void diskio_open_for_writing(void);
void diskio_write(const uint8_t __huge *data, uint16_t size);
void diskio_close_for_writing(const char *filename, uint8_t file_type);
