#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

extern uint8_t input_cursor_x;
extern uint8_t input_cursor_y;
extern uint8_t input_button_pressed;

#define HOTSPOT_OFFSET_X 7
#define HOTSPOT_OFFSET_Y 7

// code_init functions
void input_init(void);

// code_main functions
void input_update(void);

#endif // __INPUT_H