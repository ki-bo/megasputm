#include "dma.h"
#include "util.h"

//-----------------------------------------------------------------------------------------------

dmalist_t               global_dma_list;
dmalist_single_option_t global_dma_list_opt1;
dmalist_two_options_t   global_dma_list_opt2;

//-----------------------------------------------------------------------------------------------

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

  global_dma_list.end_of_options      = 0x00;
  global_dma_list_opt1.end_of_options = 0x00;
  global_dma_list_opt2.end_of_options = 0x00;
}

/** @} */ // dma_init

//-----------------------------------------------------------------------------------------------
