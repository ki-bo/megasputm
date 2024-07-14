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

#include "diskio.h"
#include "init.h"
#include "map.h"
#include "vm.h"
#include "util.h"

#pragma clang section text="code"

/**
  * @brief Entry function called by startup code
  *
  * This is the first C code getting executed after the startup code
  * has finished. It initializes the system and then calls the vm_mainloop
  * function in the code_main section.
  *
  * This function is part of the runtime code loaded during startup to 0x0200.
  *
  * Code section: code
  */
__task void main(void) 
{
  // initialize all modules
  global_init();

  // Use diskio module to load the main code to the respective secions. Code resides
  // in 0x2000-0x7fff and 0xd000-0xdfff.
  // Be aware that this is overwriting both init code and init bss in memory.
  // So all init functions need to be complete at this point and can't be called anymore.
  MAP_CS_DISKIO
  diskio_load_file("M01", (uint8_t __far *)(0x2000));  // load script parser code
  diskio_load_file("M02", (uint8_t __far *)(0x4000));  // load main code
  diskio_load_file("M03", (uint8_t __far *)(0xd000));  // load main private code
  // switch back to real drive
  //diskio_switch_to_real_drive();
  UNMAP_CS

  // Jump into loaded main code
  vm_mainloop();
}
