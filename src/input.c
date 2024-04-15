#include "input.h"
#include "io.h"
#include "util.h"
#include <mega65.h>

//-----------------------------------------------------------------------------------------------

#pragma clang section bss="zdata"

//-----------------------------------------------------------------------------------------------

uint8_t input_cursor_x;
uint8_t input_cursor_y;
uint8_t input_button_pressed;
uint8_t input_key_pressed;

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup input_init Input Init Functions
 * @{
 */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Initialize the input module
 *
 * Code section: code_init
 */
void input_init(void)
{
  input_cursor_x = 0;
  input_cursor_y = 0;
  CIA1.ddra = 0xff; // set CIA1 port A as input
}

/** @} */ // input_init

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup input_public Input Public Functions
 * @{
 */
#pragma clang section text="code_main" rodata="cdata_main" data="data_main" bss="zdata"

/**
 * @brief Update the input module
 *
 * This function updates the cursor position. Should be called once per frame in the
 * interrupt routine.
 *
 * Code section: code_main
 */
void input_update(void)
{
  uint8_t joy = CIA1.pra;
  if (!(joy & 0x01)) {
    if (input_cursor_y < 2) {
      input_cursor_y = 0;
    } 
    else {
      input_cursor_y -= 2;
    }
  } 
  else if (!(joy & 0x02)) {
    if (input_cursor_y > 197) {
      input_cursor_y = 199;
    } 
    else {
      input_cursor_y += 2;
    }
  }
  if (!(joy & 0x04) && input_cursor_x != 0) {
    input_cursor_x -= 1;
  } 
  else if (!(joy & 0x08) && input_cursor_x != 159) {
    input_cursor_x += 1;
  }
  input_button_pressed = !(joy & 0x10) ? INPUT_BUTTON_LEFT : 0;

  // keyboard handling
  if (input_key_pressed == 0) { // = 0 means previous key was processed
    uint8_t key_pressed_ascii = ASCIIKEY;
    if (key_pressed_ascii != 0) {
      debug_out("key pressed %d", key_pressed_ascii);
      input_key_pressed = key_pressed_ascii;
    }
    else {
      input_key_pressed = 0;
    }
    // writing to register dequeues the last read key
    ASCIIKEY = 0;
  }
}

/** @} */ // input_public

//-----------------------------------------------------------------------------------------------
