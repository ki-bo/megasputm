#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

enum {
    INPUT_BUTTON_LEFT  = 1,
    INPUT_BUTTON_RIGHT = 2
};

enum {
    INPUT_KEY_RUNSTOP = 0x03,
    INPUT_KEY_ESCAPE  = 0x1b
};

extern uint8_t input_cursor_x;
extern uint8_t input_cursor_y;
extern uint8_t input_button_pressed;
extern uint8_t input_key_pressed;

#define HOTSPOT_OFFSET_X 7
#define HOTSPOT_OFFSET_Y 7

// code_init functions
void input_init(void);

// code_main functions
void input_update(void);

#endif // __INPUT_H
