#include "dma.h"
#include "util.h"

dmalist_t               global_dma_list;
dmalist_single_option_t global_dma_list_opt1;
dmalist_two_options_t   global_dma_list_opt2;

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"
void dma_init(void)
{
  DMA.en018b &= 0xfe; // disable DMA F018B mode
  DMA.addrmb = 0; // dma list MB = 0

  global_dma_list_opt1.end_of_options = 0x00;
  global_dma_list_opt2.end_of_options = 0x00;
}
