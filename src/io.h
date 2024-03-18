#ifndef __IO_H
#define __IO_H

#include <stdint.h>

struct __cpu_vectors {
  void *nmi;
  void *reset;
  void *irq;
};

struct __f011 {
  uint8_t fdc_control;
  volatile uint8_t command;
  uint16_t status;
  volatile uint8_t track;
  volatile uint8_t sector;
  volatile uint8_t side;
  volatile uint8_t data;
  uint8_t clock;
  uint8_t step;
  uint8_t pcode;
};

enum fdc_status_mask_t
{
  /** Drive select (0 to 7). Internal drive is 0. Second floppy drive on internal cable 
      is 1. Other values reserved for C1565 external drive interface. */
  FDC_DS_MASK = 0b00000111,
  /** Directly controls the SIDE signal to the floppy drive, i.e., selecting which side of the media is active. */
  FDC_SIDE_MASK = 0b00001000,
  /** Swap upper and lower halves of data buffer (i.e. invert bit 8 of the sector buffer) */
  FDC_SWAP_MASK = 0b00010000,
  /** Activates drive motor and LED (unless LED signal is also set, causing the drive LED to blink) */
  FDC_MOTOR_MASK = 0b00100000,
  /** Drive LED blinks when set */
  FDC_LED_MASK = 0b01000000,
  /** The floppy controller has generated an interrupt (read only). Note that in- terrupts are not currently implemented on the 45GS27. */
  FDC_IRQ_MASK = 0b10000000,
  /** F011 Head is over track 0 flag (read only) */
  FDC_TK0_MASK = 0x0001,
  /** F011 FDC CRC check failure flag (read only) */
  FDC_CRC_MASK = 0x0008,
  /** F011 FDC Request Not Found (RNF), i.e., a sector read or write operation did not find the requested sector (read only) */
  FDC_RNF_MASK = 0x0010,
  /** F011 FDC CPU and disk pointers to sector buffer are equal, indicating that the sector buffer is either full or empty. (read only) */
  FDC_EQ_MASK = 0x0020,
  /** F011 FDC DRQ flag (one or more bytes of data are ready) (read only) */
  FDC_DRQ_MASK = 0x0040,
  /** F011 FDC busy flag (command is being executed) (read only) */
  FDC_BUSY_MASK = 0x0080,
  /** F011 Read Request flag, i.e., the requested sector was found during a read operation (read only) */
  FDC_RDREQ_MASK = 0x8000
};

enum fdc_command_t
{
  FDC_CMD_CLR_BUFFER_PTRS = 0x01,
  FDC_CMD_STEP_OUT = 0x10,
  FDC_CMD_STEP_IN = 0x18,
  FDC_CMD_SPINUP = 0x20,
  FDC_CMD_READ_SECTOR = 0x40
};

struct __dma {
  volatile uint8_t addrlsbtrig;
  volatile uint8_t addrmsb;
  volatile uint8_t addrbank;
  uint8_t en018b;
  uint8_t addrmb;
  volatile uint8_t etrig;
  volatile uint8_t etrig_mapped;
  volatile uint8_t trig_inline;
};

#define CPU_VECTORS (*(struct __cpu_vectors *)0xfffa)
#define FDC         (*(struct __f011 *)       0xd080)
#define DMA         (*(struct __dma *)        0xd700)

#endif // __IO_H
