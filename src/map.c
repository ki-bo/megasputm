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

#include "map.h"

#pragma clang section bss="zzpage"

union map_t __attribute__((zpage)) map_regs;

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
  * @brief Initializes the memory mapping system
  * 
  * This function must be called before any other memory mapping functions are
  * used. It sets up the memory mapping register memory.
  *
  * Code section: code_init
  */
void map_init(void)
{
  //map_regs.quad = 0;
}
