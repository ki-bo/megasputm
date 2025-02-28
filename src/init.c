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

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="zdata_init"

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
    sound_resource_type = RES_TYPE_SOUND_MOD;
    diskio_load_file(0, "M13", (uint8_t __far *)(0x16000)); // load mod sound code
    sound_init();
  }
  else {
    sound_resource_type = RES_TYPE_SOUND_SID;
    diskio_load_file(0, "M14", (uint8_t __far *)(0x16000)); // load sid sound code
    c64_sound_init();
  }
}
