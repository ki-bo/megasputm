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
 */
void heap_init(void)
{
  map_ds_heap();
  __heap_initialize(&__default_heap, (void *)RES_MAPPED, HEAP_SIZE);
  unmap_ds();
}

/** @} */ // heap_init
