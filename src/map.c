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
