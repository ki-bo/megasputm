/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * MEGASPUTM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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

#include "init.h"
#include "actor.h"
#include "charset.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "input.h"
#include "inventory.h"
#include "map.h"
#include "sound_mod.h"
#include "sound_sid.h"
#include "util.h"
#include "resource.h"
#include "script.h"
#include "vm.h"

#pragma clang section rodata="autoboot_load_address"
const uint16_t mega65_load_address = 0x2001;

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Callback from startup code to do low-level initialisation
 *
 * This function is called from the startup code before main() is called. The startup will setup
 * the soft stack and then call this function. No data initialisation has been done at this point.
 * That means that global bss data has not been zeroed and no heap has been setup, yet.
 */
void __low_level_init(void)
{
  // load diskio module to 0x12000
  static const char diskio_filename[] = "M11";
  __asm(" lda #3\n" // filename size
        " ldx #.byte0 diskio_filename\n"
        " ldy #.byte1 diskio_filename\n"
        " jsr 0xffbd\n" // SETNAM
        " lda #32\n"
		    " ldx #8\n"
		    " ldy #0\n"
		    " jsr 0xffba\n" // SETLFS
		    " lda #1\n"
        " ldx #0\n"
		    " jsr 0xff6b\n" // SETBNK
		    " ldy #0x20\n"
		    " ldx #0x00\n"
		    " lda #0b01000000\n" // bit6 = raw read (don't skip first 2 bytes)
		    " jsr 0xffd5\n" // LOAD
        :
        :
        : "a", "x", "y", "z");

  __disable_interrupts();

  __asm(" map\n"
        " eom\n"
        :
        : "Ka"(U8(0)), "Kx"(U8(0)), "Ky"(U8(0)), "Kz"(U8(0))
        :);

  CPU_PORTDDR = 65;   // force 40 MHz mode
  CPU_PORT    = 0x35; // all RAM + I/O (C64 style banking)
  
  // enable MEGA65 I/O personality
  VICIV.key = 0x47;
  VICIV.key = 0x53;

  VICIV.ctrla  = 0;  // disable all $D030 style banking
  DMA.en018b  &= ~1; // disable F018B mode

  // disable write protection for banks 2 and 3
  __asm(" lda #2\n"
        " sta 0xd641\n"
        " clv\n");

  // black blank screen
  VICIV.bordercol = 0;
  VICIV.screencol = 0;
  VICIV.ctrl1     = 0; // disable screen

  // This puts all code of the "code" section into the correct location
  // It is safe to call "code" section functions from now on
  relocate_runtime();
}


/**
  * @brief Initialises all submodules
  *
  * This function does all initialisation of all sub-modules. It needs to be called
  * at the beginning of the autoboot prg.
  *
  * Code section: code_init
  */
void global_init(void)
{
  map_init();
  
  // configure dma
  dma_init();

  // prepare charset
  charset_init();

  // init diskio module
  diskio_init();

  // load and init gfx module (CS_DISKIO is still mapped from diskio_init)
  diskio_load_file(0, "M10", (uint8_t __far *)(0x11800)); // load gfx2 code
  diskio_load_file(0, "M12", (uint8_t __far *)(0x14000)); // load gfx code
  gfx_init();
 
  // init input module
  input_init();

  // init main engine code
  res_init();    // resource module
  inv_init();    // inventory
  script_init(); // script parser
  actor_init();  // actor module
  vm_init();     // virtual machine and main game logic

  // load and init sound module (depends on use_sid_sounds, which gets set during vm_init() when loading the index)
  MAP_CS_DISKIO
  if (!use_sid_sounds) {
    diskio_load_file(0, "M13", (uint8_t __far *)(0x16000)); // load mod sound code
    sound_init();
  }
  else {
    diskio_load_file(0, "M14", (uint8_t __far *)(0x16000)); // load sid sound code
    c64_sound_init();
  }
}
