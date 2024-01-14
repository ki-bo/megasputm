#ifndef __DMA_H
#define __DMA_H

#include <stdint.h>

struct __dma {
  volatile uint8_t addrlsbtrig;
  volatile uint8_t addrmsb;
  volatile uint8_t addrbank;
  uint8_t en018b;
  uint8_t addrmb;
  volatile uint8_t etrig;
};

#define DMA (*(struct __dma *)0xd700)

typedef struct 
{
  // F018A format DMA request
  uint8_t command;        //!< Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
  uint16_t count;         //!< Number of bytes to copy
  union {
    uint16_t src_addr; //!< Source address
    uint8_t fill_byte;    //!< Fill byte
  };
  uint8_t src_bank;    //!< Source bank and flags
  uint16_t dst_addr;     //!< Destination address
  uint8_t dst_bank;      //!< Destination bank and flags
} dmalist_t;

typedef struct 
{
    // F018A format DMA request with 32-bit addresses
    uint8_t opt_token;      //!< Option token
    uint8_t opt_arg;        //!< Option argument (byte)
    uint8_t end_of_options; //!< End of options token (0x00)
    uint8_t command;        //!< Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
    uint16_t count;         //!< Number of bytes to copy
    union {
      uint16_t src_addr; //!< Source address
      uint8_t fill_byte;    //!< Fill byte
    };
    uint8_t src_bank;    //!< Source bank and flags
    uint16_t dst_addr;     //!< Destination address
    uint8_t dst_bank;      //!< Destination bank and flags
} dmalist_single_option_t;

void dma_init(void);

inline void dma_trigger(void *dma_list)
{
  DMA.addrbank    = 0;
  DMA.addrmsb     = (uint8_t)((uint16_t)&dma_list >> 8);
  DMA.addrlsbtrig = (uint8_t)((uint16_t)&dma_list & 0xff);  
}


#endif // __DMA_H
