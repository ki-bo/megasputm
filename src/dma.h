#ifndef __DMA_H
#define __DMA_H

#include "util.h"
#include <stdint.h>

struct __dma {
  volatile uint8_t addrlsbtrig;
  volatile uint8_t addrmsb;
  volatile uint8_t addrbank;
  uint8_t en018b;
  uint8_t addrmb;
  volatile uint8_t etrig;
  uint8_t unused;
  volatile uint8_t trig_inline;
};

#define DMA (*(struct __dma *)0xd700)

typedef struct 
{
  // F018A format DMA request
  uint8_t command;        // Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
  uint16_t count;         // Number of bytes to copy
  union {
    uint16_t src_addr;    // Source address
    uint8_t fill_byte;    // Fill byte
  };
  uint8_t src_bank;       // Source bank and flags
  uint16_t dst_addr;      // Destination address
  uint8_t dst_bank;       // Destination bank and flags
} dmalist_t;

typedef struct 
{
    // F018A format DMA request with 1 option
    uint8_t opt_token;      // Option token
    uint8_t opt_arg;        // Option argument (byte)
    uint8_t end_of_options; // End of options token (0x00)
    uint8_t command;        // Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
    uint16_t count;         // Number of bytes to copy
    union {
      uint16_t src_addr;    // Source address
      uint8_t fill_byte;    // Fill byte
    };
    uint8_t src_bank;       // Source bank and flags
    uint16_t dst_addr;      // Destination address
    uint8_t dst_bank;       // Destination bank and flags
} dmalist_single_option_t;

typedef struct 
{
    // F018A format DMA request with two options
    uint8_t opt_token1;      // Option token
    uint8_t opt_arg1;        // Option argument (byte)
    uint8_t opt_token2;      // Option token
    uint8_t opt_arg2;        // Option argument (byte)
    uint8_t end_of_options;  // End of options token (0x00)
    uint8_t command;         // Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
    uint16_t count;          // Number of bytes to copy
    union {
      uint16_t src_addr;     // Source address
      uint8_t fill_byte;     // Fill byte
    };
    uint8_t src_bank;        // Source bank and flags
    uint16_t dst_addr;       // Destination address
    uint8_t dst_bank;        // Destination bank and flags
} dmalist_two_options_t;

extern dmalist_t               global_dma_list;
extern dmalist_single_option_t global_dma_list_opt1;
extern dmalist_two_options_t   global_dma_list_opt2;

void dma_init(void);

inline void dma_trigger(void *dma_list)
{
  DMA.addrbank    = 0;
  DMA.addrmsb     = MSB(dma_list);
  DMA.addrlsbtrig = LSB(dma_list);
}

inline void dma_trigger_ext(void *dma_list)
{
  DMA.addrbank = 0;
  DMA.addrmsb  = MSB(dma_list);
  DMA.etrig    = LSB(dma_list);
}

inline void dma_trigger_far(void __far *dma_list)
{
  DMA.addrbank    = BANK(dma_list);
  DMA.addrmsb     = MSB(dma_list);
  DMA.addrlsbtrig = LSB(dma_list);
}

inline void dma_trigger_far_ext(void __far *dma_list)
{
  DMA.addrbank = BANK(dma_list);
  DMA.addrmsb  = MSB(dma_list);
  DMA.etrig    = LSB(dma_list);
}


#endif // __DMA_H
