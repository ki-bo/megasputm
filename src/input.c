/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "input.h"
#include "io.h"
#include "util.h"
#include "vm.h"

//-----------------------------------------------------------------------------------------------

#pragma clang section bss="zdata"

//-----------------------------------------------------------------------------------------------

enum {
    INPUT_ASCII_RUNSTOP = 0x03,
    INPUT_ASCII_ESCAPE  = 0x1b
};


uint16_t input_cursor_x;
uint8_t  input_cursor_y;
uint8_t  input_button_pressed;
uint8_t  input_key_pressed;

static int16_t new_x;
static int16_t new_y;

static void handle_joystick(void);
static void handle_mouse(void);
static int8_t check_mouse_movement(uint8_t pot, uint8_t old_pot);
static int8_t apply_acceleration(int8_t value);
static void handle_keyboard(void);

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
  CIA1.ddra   = 0xff; // set CIA1 port A as output
  CIA1.ddrb   = 0xff; // set CIA1 port B as output
  CIA1.pra    = 0xff; // connect mouse port 1 to SID1
  CIA1.prb    = 0xff; // pull all pins of port B high
  UART_E_DDR |= 0x02; // set UART_E pin as output
  UART_E_PRA |= 0x02; // set UART_E pin to high (controlling keyboard column C8 on the C65/MEGA65)
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
  // manually map main_priv, as we are called in irq context
  __asm(" lda #0xa0\n"
        " ldx #0x20\n"
        " ldy #0\n"
        " ldz #0\n"
        " map\n"
        " eom\n"
        :
        :
        : "a", "x", "y", "z");

  new_x = input_cursor_x;
  new_y = input_cursor_y;

  handle_mouse();
  handle_keyboard();
  handle_joystick();
  CIA1.pra = 0x40; // prepare CIA1 alredy for sampling mouse, as this takes some time

  if (new_x < 0) {
    new_x = 0;
  }
  else if (new_x > 319) {
    new_x = 319;
  }
  if (new_y < 0) {
    new_y = 0;
  }
  else if (new_y > 199) {
    new_y = 199;
  }

  input_cursor_x = new_x;
  input_cursor_y = new_y;
}

static void handle_joystick(void)
{
  static uint8_t old_joy1;
  uint8_t joy2 = CIA1.pra;
  uint8_t joy1 = CIA1.prb;
  if (!(joy2 & 0x01)) {
    new_y -= 2;
  } 
  else if (!(joy2 & 0x02)) {
    new_y += 2;
  }
  if (!(joy2 & 0x04)) {
    new_x -= 2;
  }
  else if (!(joy2 & 0x08)) {
    new_x += 2;
  }
  if (ui_state & UI_FLAGS_ENABLE_CURSOR) {
    input_button_pressed = (!(joy2 & 0x10) || !(joy1 & 0x10)) ? INPUT_BUTTON_LEFT : 0;
  }
  
  if ((old_joy1 & 1) && !(joy1 & 1)) {
    // edge triggered right mouse button is handled as override key
    input_key_pressed = vm_read_var8(VAR_OVERRIDE_KEY);
  }
  old_joy1 = joy1;
}

static void handle_mouse(void)
{
  static uint8_t old_potx = 0;
  static uint8_t old_poty = 0;
  uint8_t potx = POT.x;
  uint8_t poty = POT.y;
  // prepare CIA1 already now for joystick handling, as this takes some time
  CIA1.pra  = 0xff;

  int8_t diff = check_mouse_movement(potx, old_potx);
  if (diff) {
    new_x += apply_acceleration(diff);
    old_potx = potx;
  }
  diff = check_mouse_movement(poty, old_poty);
  if (diff) {
    new_y -= apply_acceleration(diff);
    old_poty = poty;
  }
}

#pragma clang section text="code_main_private" rodata="cdata_main_private" data="data_main_private"

static int8_t check_mouse_movement(uint8_t pot, uint8_t old_pot) 
{
  uint8_t diff = (pot - old_pot) & 0x7f;
  if (diff < 64) {
    return (int8_t)diff >> 1; // divide by 2 but keep sign/msb
  }
  // handle negative value, add two msb sign bits and mask out noise bit
  diff |= 0xc1;
  if (diff != 0xff) {
    ++diff;
    return (int8_t)diff >> 1;
  }
  return 0;
}

static int8_t apply_acceleration(int8_t value)
{
  //uint8_t abs_diff = abs8(value);
  uint8_t abs_diff;
  __asm(" tax\n"
        " bpl done\n"
        " neg a\n"
        "done:"
        : "=Ka"(abs_diff)
        : "Ka"(value)
        : "a", "x");

  if (abs_diff > 15) {
    return value << 2;
  }
  if (abs_diff > 10) {
    return value << 1;
  }
  return value;
}

static void handle_keyboard(void)
{
  if (input_key_pressed == 0) { // = 0 means previous key was processed
    uint8_t key_pressed_ascii = ASCIIKEY;
    if (key_pressed_ascii != 0) {
      if (key_pressed_ascii == INPUT_ASCII_ESCAPE || key_pressed_ascii == INPUT_ASCII_RUNSTOP || key_pressed_ascii == 0xf4) {
        input_key_pressed = vm_read_var8(VAR_OVERRIDE_KEY);
      }
      else if (key_pressed_ascii >= 0xf1 && key_pressed_ascii <= 0xfe) {
        // handling function keys
        switch (key_pressed_ascii)
        {
          case 0xf1:
            input_key_pressed = 1;
            break;
          case 0xf3:
            input_key_pressed = 2;
            break;
          case 0xf5:
            input_key_pressed = 3;
            break;
          case 0xf8:
            input_key_pressed = 8;
            break;
          case 0xf9:
            input_key_pressed = 5;
            break;
        }
      }
      else if (key_pressed_ascii >= 0x61 && key_pressed_ascii <= 0x7a) {
        // handling A-Z keys
        input_key_pressed = key_pressed_ascii;
      }
      else if (key_pressed_ascii == 0x1f) {
        // handling HELP key
        input_key_pressed = 0x1f;
      }
      else if (key_pressed_ascii == 0x20) {
        // handling space key
        input_key_pressed = 0x20;
      }
      else if (key_pressed_ascii == 0x0d) {
        // handling return key
        input_key_pressed = 0x0d;
      }
      else if (key_pressed_ascii == 0x3c || key_pressed_ascii == 0x3e) {
        // handling < and > keys
        input_key_pressed = key_pressed_ascii;
      }
      //debug_out("key pressed %d = %d", key_pressed_ascii, input_key_pressed);
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
