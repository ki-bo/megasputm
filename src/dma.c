#include "dma.h"
#include "util.h"

#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"
void dma_init(void)
{
  DMA.en018b &= 0xfe; // disable DMA F018B mode
  DMA.addrmb = 0; // dma list MB = 0
}
