#ifndef __DISKIO_H
#define __DISKIO_H

#include <stdint.h>

// code_init functions
void diskio_init(void);

// code_diskio functions
void diskio_switch_to_real_drive(void);
void diskio_check_motor_off(uint8_t elapsed_jiffies);
void diskio_load_file(const char *filename, uint8_t __far *address);
void diskio_load_room(uint8_t room, __far uint8_t *address);
uint16_t diskio_start_resource_loading(uint8_t type, uint8_t id);
void diskio_continue_resource_loading(uint8_t __huge *target_ptr);

#endif // __DISKIO_H
