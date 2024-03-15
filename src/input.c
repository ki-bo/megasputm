#include "input.h"
#include <mega65.h>

#pragma clang section bss="zdata"

//-----------------------------------------------------------------------------------------------

uint8_t input_cursor_x;
uint8_t input_cursor_y;
uint8_t input_button_pressed;

//-----------------------------------------------------------------------------------------------

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

//-----------------------------------------------------------------------------------------------

void input_init(void)
{
  input_cursor_x = 0;
  input_cursor_y = 0;
  CIA1.ddra = 0xff; // set CIA1 port A as input
}

//-----------------------------------------------------------------------------------------------

#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

//-----------------------------------------------------------------------------------------------

void input_update(void)
{
  uint8_t joy = CIA1.pra;
  if (!(joy & 0x01)) {
    input_cursor_y -= 2;
  } else if (!(joy & 0x02)) {
    input_cursor_y += 2;
  }
  if (!(joy & 0x04)) {
    input_cursor_x -= 1;
  } else if (!(joy & 0x08)) {
    input_cursor_x += 1;
  }
  input_button_pressed = !(joy & 0x10) ? 1 : 0;

  if (input_cursor_x < 12) {
    input_cursor_x = 12;
  } else if (input_cursor_x > 171) {
    input_cursor_x = 171;
  }

  if (input_cursor_y < 50) {
    input_cursor_y = 50;
  } else if (input_cursor_y > 249) {
    input_cursor_y = 249;
  }
}

//-----------------------------------------------------------------------------------------------
