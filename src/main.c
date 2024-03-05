#include "diskio.h"
#include "init.h"
#include "io.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include <string.h>

// Private wrapper functions
static void check_motor_off(void);

#pragma clang section text="code_main"

__task void main_entry(void)
{
  // We will never return, so reset the stack pointer to the top of the stack
  __asm(" ldx #0xff\n"
        " txs"
        : /* no output operands */
        : /* no input operands */
        : "x" /* clobber list */);

  uint8_t start_script = 1;
  uint8_t hint = 0;
  res_provide(RES_TYPE_SCRIPT | RES_LOCKED_MASK, start_script, hint);
  while(1) {
    check_motor_off();
    POKE(0xd020, 2);
  }
}

#pragma clang section text="code"

/**
 * @brief Function to check if the disk drive motor needs to be turned off
 *
 * This function checks if the disk drive motor needs to be turned off. It
 * is called in the main loop of the main code several times per second.
 *
 * Code section: code 
 */
static void check_motor_off(void)
{
  map_cs_diskio();
  diskio_check_motor_off();
  unmap_cs();
}

/**
 * @brief Entry function called by startup code
 *
 * This is the first C code getting executed after the startup code
 * has finished. It initializes the system and then calls the main_entry
 * function in the code_main section.
 *
 * It is part of the runtime code loaded during startup to 0x0200.
 *
 * Code section: code
 */
__task void main(void) 
{
  global_init();

  // Use diskio module to load the main code to section code_main at 0x2000
  map_cs_diskio();
  diskio_load_file("M01", (uint8_t __far *)(0x2000));
  unmap_all();
  
  // switch to real drive
  //*(uint8_t *)(0xd68b) &= ~1;
  //*(uint8_t *)(0xd6a1) |= 1;
  
  // Jump into loaded main code
  main_entry();
}
