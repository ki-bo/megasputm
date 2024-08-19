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

#include "dma.h"
#include "util.h"

#pragma clang section bss="zzpage"
global_dma_t __attribute__((zpage)) global_dma;

/**
  * @defgroup dma_init DMA Init Functions
  * @{
  */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
  * @brief Initialize the DMA controller
  * 
  * Code section: code_init
  */
void dma_init(void)
{
  DMA.en018b   &= 0xfe; // disable DMA F018B mode
  DMA.addrmb    = 0;    // dma list MB = 0
  DMA.addrbank  = 0;    // dma list bank is always 0
}

/** @} */ // dma_init


/**
  * @defgroup dma_public DMA Public Functions
  * @{
  */
#pragma clang section text="code" rodata="cdata" data="data" bss="zdata"
void dma_trigger(const void *dma_list)
{

  DMA.addrmsb      = MSB(dma_list);
  DMA.etrig_mapped = LSB(dma_list);
}

void dma_trigger_global(void)
{
  DMA.addrmsb      = MSB(&global_dma);
  DMA.etrig_mapped = LSB(&global_dma);
}

/** @} */ // dma_public
