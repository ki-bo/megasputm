#pragma rtattr initialize="normal"

#include <stddef.h>
#include <string.h>

#define ATTR_TABLE

#if defined(__CALYPSI_TARGET_65816__) && defined(__CALYPSI_DATA_MODEL_SMALL__)
#define ATTR __far
#undef ATTR_TABLE
#define ATTR_TABLE __far
#define memory_copy __memcpy_far
#define memory_set  __memset_far
#elif defined(__CALYPSI_TARGET_6502__) && (defined(__CALYPSI_TARGET_SYSTEM_FOENIX__) || defined(__CALYPSI_TARGET_SYSTEM_MEGA65__))
#if defined(__CALYPSI_TARGET_SYSTEM_MEGA65__)
#define ATTR __far
#else
#define ATTR __vram
#endif
#define memory_copy __memcpy_far
#define memory_set  __memset_far
#else
#define ATTR
#define memory_copy memcpy
#define memory_set  memset
#endif

struct __init_data {
  ATTR void       *dest;
  ATTR const void *src;
  size_t           size;
};

void __initialize_sections(ATTR_TABLE struct __init_data *p, ATTR_TABLE struct __init_data *end) {
  while (p < end) {
    if (p->src) {
      memory_copy(p->dest, p->src, p->size);
    } else {
      memory_set(p->dest, 0, p->size);
    }
    p++;
  }
}