#if defined(__CALYPSI_TARGET_65816__) || (defined(__CALYPSI_TARGET_SYSTEM_MEGA65__) && defined(__CALYPSI_CORE_45GS02__))
#pragma rtattr initialize="large"

#include <stddef.h>
#include <stdint.h>

#if defined(__CALYPSI_TARGET_65816__)
#define _InitTableAttr __far
#else
#define _InitTableAttr
#endif

#ifdef __CALYPSI_HUGE_ATTRIBUTE_ENABLED__

struct __init_data {
  __huge uint8_t       *dest;
  __huge const uint8_t *src;
  size_t                size;
};

void __initialize_sections(_InitTableAttr struct __init_data *p,
                           _InitTableAttr struct __init_data *end) {
  while (p < end) {
    __attribute__((huge)) uint8_t *dest = p->dest;
    __attribute__((huge)) const uint8_t *src = p->src;
    if (src) {
      for (size_t i = p->size; i != 0; i--) {
        *dest++ = *src++;
      }
    } else {
      for (size_t i = p->size; i != 0; i--) {
        *dest++ = 0;
      }
    }
    p++;
  }
}

#else

struct __init_data {
  __far uint8_t       *dest;
  __far const uint8_t *src;
  size_t              size;
};

void __initialize_sections(_InitTableAttr struct __init_data *p,
                           _InitTableAttr struct __init_data *end) {
  while (p < end) {
    __attribute__((far)) uint8_t *dest = p->dest;
    __attribute__((far)) const uint8_t *src = p->src;
    if (src) {
      for (size_t i = p->size; i != 0; i--) {
        *dest = *src;
	      dest = (__far uint8_t*)(((uint32_t)dest) + 1);
	      src = (__far const uint8_t*)(((uint32_t)src) + 1);
      }
    } else {
      
      for (size_t i = p->size; i != 0; i--) {
        *dest = 0;
	      dest = (__far uint8_t*)(((uint32_t)dest) + 1);
      }
    }
    p++;
  }
}

#endif // __CALYPSI_HUGE_ATTRIBUTE_ENABLED__
#endif // __CALYPSI_TARGET_65816__ || (__CALYPSI_TARGET_SYSTEM_MEGA65__ && __CALYPSI_CORE_45GS02__)
