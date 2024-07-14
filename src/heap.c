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

#include "heap.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include <stddef.h>
#include <stdlib.h>


// defined in compiler's runtime lib
extern struct __heap_s __default_heap;
void __heap_initialize(struct __heap_s *heap, void *heapstart, size_t heapsize);

/**
  * @defgroup heap_init Heap Init Functions
  * @{
  */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
  * @brief Initialize the heap.
  * 
  * You need to ensure the resource module is initialized before calling this function, as it
  * will reserve the memory for the heap during its init routine.
  *
  * The heap is always assumed to be in the first pages of the resource memory.
  *
  * Code section: code_init
  */
void heap_init(void)
{
  map_ds_heap();
  __heap_initialize(&__default_heap, (void *)RES_MAPPED, HEAP_SIZE);
  UNMAP_DS
}

/** @} */ // heap_init
