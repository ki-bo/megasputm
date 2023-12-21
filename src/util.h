#include <stdint.h>

inline void set_bits(uint16_t address, uint8_t mask)
{
    asm volatile("tsb %0\n"
                :                         /* no output operands */
                : "i"(address), "a"(mask) /* input operands */
                :                         /* clobber list */);
}

inline void clear_bits(uint16_t address, uint8_t mask)
{
  asm volatile("trb %0\n"
               :                         /* no output operands */
               : "i"(address), "a"(mask) /* input operands */
               :                         /* clobber list */);
}
