#include "dma.h"
#include "util.h"

#pragma clang section text="initcode" rodata="initcdata"
void dma_init(void)
{
  DMA.en018b &= 0xfe; // disable DMA F018B mode
  DMA.addrmb = 0; // dma list MB = 0
}
