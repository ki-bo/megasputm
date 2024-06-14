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

//-----------------------------------------------------------------------------------------------
