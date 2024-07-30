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

#include "init.h"
#include "actor.h"
#include "charset.h"
#include "dma.h"
#include "diskio.h"
#include "gfx.h"
#include "input.h"
#include "inventory.h"
#include "map.h"
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
  diskio_load_file("M10", (uint8_t __far *)(0x11800)); // load gfx2 code
  diskio_load_file("M12", (uint8_t __far *)(0x14000)); // load gfx code
  gfx_init();

  // init input module
  input_init();

  // init main engine code
  res_init();    // resource module
  inv_init();    // inventory
  script_init(); // script parser
  actor_init();  // actor module
  vm_init();     // virtual machine and main game logic
}
