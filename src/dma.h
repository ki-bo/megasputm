#ifndef __DMA_H
#define __DMA_H

#include "util.h"
#include "io.h"
#include <stdint.h>

typedef struct 
{
  // F018A format DMA request
  uint8_t end_of_options; // End of options token (0x00)
  uint8_t command;        // Command (LSB), e.g. DMA_COPY_CMD, DMA_FILL_CMD, etc.
  union {
    uint16_t count;
    struct {
      uint8_t count_lsb;
      uint8_t count_msb;
    };
  };
  union {
    struct {
      union {
        uint16_t src_addr;    // Source address
        uint8_t fill_byte;    // Fill byte
      };
      uint8_t src_bank;       // Source bank and flags
    };
    struct {
      uint8_t src_addr_lsb;
      uint16_t src_addr_page;
    };
  };
  union {
    struct {
      uint16_t dst_addr;      // Destination address
      uint8_t dst_bank;       // Destination bank and flags
    };
    struct {
      uint8_t dst_addr_lsb;
      uint16_t dst_addr_page;
    };
  };
  uint16_t modulo;
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
    uint16_t modulo;
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
    uint16_t modulo;
} dmalist_two_options_t;

extern dmalist_t               global_dma_list;
extern dmalist_single_option_t global_dma_list_opt1;
extern dmalist_two_options_t   global_dma_list_opt2;

enum {
  DMA_CMD_COPY  = 0x00,
  DMA_CMD_FILL  = 0x03,
  DMA_CMD_CHAIN = 0x04
};

enum {
  DMA_F018A_FLAGS_DST_MODULO = 0x20
};

// code_init functions
void dma_init(void);

static inline void dma_trigger(const void *dma_list)
{

  DMA.addrmsb      = MSB(dma_list);
  DMA.etrig_mapped = LSB(dma_list);
}

#endif // __DMA_H
