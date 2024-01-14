#include "diskio.h"
#include "dma.h"
#include "util.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static uint8_t track_list[54];
static uint8_t sector_list[54];
static uint8_t current_track;

static void prepare_drive(void);
static void wait_for_busy_clear(void);

typedef struct __f011 {
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
} f011_t;

#define FDC (*(struct __f011 *)0xd080)

enum
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

enum
{
  FDC_CMD_CLR_BUFFER_PTRS = 0x01,
  FDC_CMD_STEP_OUT = 0x10,
  FDC_CMD_STEP_IN = 0x18,
  FDC_CMD_SPINUP = 0x20,
  FDC_CMD_READ_SECTOR = 0x40
};

static void read_whole_track(uint8_t track);

/**
 * @brief Loads a sector from the floppy disk into the floppy buffer.
 * 
 * As sectors in 1581 format are 512 bytes, they always contain two logical
 * blocks. This function loads the sector containing the specified block into
 * the floppy buffer. 
 *
 * @param track Logical track number (1-80)
 * @param block Logical block number (0-39)
 */
static void load_block(uint8_t track, uint8_t block);

static void led_and_motor_off(void);

/**
 * @brief Reads the directory block in memory at buffer
 * 
 * The directory block contains up to eight file entries. Each file entry is 32
 * bytes long. If there are more blocks to read in the directory, the function will
 * automatically load the next block and return 1. If there are no more
 * blocks to read, the function will return 0.
 *
 * @return uint8_t 0 if there are no more blocks to read, 1 otherwise
 */
static uint8_t read_next_directory_block(void);

/**
 * @brief Reads bytes from the sector buffer and parses them into a file entry.
 * 
 * The function reads bytes from the sector buffer and parses one file entry into
 * track_list and sector_list. The function returns the number of bytes actually
 * read from the sector buffer.
 * 
 * @note The number of bytes actually read from the sector buffer can vary, 
 *       as the function will stop reading when it encounters the first invalid
 *       byte.
 *
 * @return uint8_t The number of bytes actually read from the sector buffer
 */
static uint8_t read_file_entry(void);


// public functions
#pragma clang section text="initcode" rodata="initcdata"
void diskio_init(void)
{
  debug_out("Initializing diskio %d\n", 0);

  memset(track_list, 0, sizeof(track_list));
  memset(sector_list, 0, sizeof(sector_list));

  POKE(0xd680, 0x81); // map FDC buffer to $de00
  *NEAR_U8_PTR(0xd689) &= 0x7f; // see floppy buffer, not SD buffer
  prepare_drive();

  while (!(FDC.status & FDC_TK0_MASK)) {
    // not yet on track 0, so step outwards
    FDC.command = FDC_CMD_STEP_OUT;
    wait_for_busy_clear();
  }
  current_track = 0;

  /*read_whole_track(40);
  POKE(0xd020, 3);
  while(1);*/

  // Loading file list in the directory, starting at track 40, block 3
  POKE(0xd020,0);
  load_block(40, 3);
  while (read_next_directory_block() != 0);
  
  led_and_motor_off();

  POKE(0xd680, 0x82); // unmap FDC buffer from $de00
}

#pragma clang section text="code" rodata="cdata"
void diskio_load_room(uint8_t room, __far uint8_t *address)
{
  debug_out("Loading room %d\n", room);
  POKE(0xd680, 0x81); // map FDC buffer to $de00
  prepare_drive();
  uint8_t next_track = track_list[room];
  uint8_t next_block = sector_list[room];
  uint8_t payload_size = 254;

  while (next_track != 0) {
    debug_out("Loading track %d, block %d", next_track, next_block);
    load_block(next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    if (next_track == 0) {
      payload_size = next_block - 1;
    }
    /*for (uint8_t i = 0; i < payload_size ; ++i) {
      *address = FDC.data ^ 0xff;
      ++address;
    }*/

    address += payload_size;
  }
  
  led_and_motor_off();
  POKE(0xd680, 0x82); // unmap FDC buffer from $de00
}


// private functions

static void prepare_drive(void)
{
  *NEAR_U8_PTR(0xd696) &= 0x7f; // disable auto-tune
  FDC.fdc_control |= FDC_MOTOR_MASK | FDC_LED_MASK; // enable LED and motor
  FDC.command = FDC_CMD_SPINUP;
  wait_for_busy_clear();
}

static void wait_for_busy_clear(void)
{
  while (FDC.status & FDC_BUSY_MASK);
}

static void step_to_track(uint8_t track)
{
  while (track != current_track) {
    // not yet on correct track, so we need to step to the new track
    if (track < current_track) {
      FDC.command = FDC_CMD_STEP_OUT;
      --current_track;
    }
    else {
      FDC.command = FDC_CMD_STEP_IN;
      ++current_track;
    }
    wait_for_busy_clear();
  }
}

static void read_whole_track(uint8_t track)
{
  static uint8_t sectors_read[40];
  static dmalist_t dmalist = {
    .command = 0x00,      //!< DMA copy command
    .count = 0x0200,
    .src_addr = 0xde00,
    .src_bank = 0x80,     //!< enable I/O to read from $de00
    .dst_addr = 0x0000,
    .dst_bank = 0x04
  };

  int8_t num_sectors_left;

  step_to_track(track);
  *NEAR_U8_PTR(0xd6a1) |= 0x02; // enable TARGANY

  FDC.command = FDC_CMD_CLR_BUFFER_PTRS;
  int8_t sector_side_offset = 0;

  for (uint8_t side = 0; side < 2; ++side) {
    memset(sectors_read, 0, sizeof(sectors_read));
    num_sectors_left = 20;
    FDC.fdc_control |= FDC_SIDE_MASK; // select side 0

    while (num_sectors_left > 0) {

      *NEAR_U8_PTR(0xd6a1) |= 0x02; // enable TARGANY
      FDC.command = FDC_CMD_READ_SECTOR;
      //while(!(FDC.status & FDC_RDREQ_MASK)) POKE(0xd021, PEEK(0xd021) + 1);
      wait_for_busy_clear();
      while(!(FDC.status & (FDC_DRQ_MASK | FDC_EQ_MASK))) POKE(0xd020, PEEK(0xd020) + 1);
      
      uint8_t sector = FDC.sector + sector_side_offset - 1;

      if (sectors_read[sector] == 0) {
        sectors_read[sector] = 1;
        --num_sectors_left;
        dmalist.dst_addr = sector * 512;
        dma_trigger(&dmalist);
      }


      // RNF or CRC error flag check
      if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
        // error
        led_and_motor_off();
        fatal_error("Error reading sector");
      }
    }

    sector_side_offset = 20;
    POKE(0xd020, 1);
    FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 0

  }
    POKE(0xd020, 0);
}

static void load_block(uint8_t track, uint8_t block)
{
  static uint8_t last_physical_track = 255;
  static uint8_t last_physical_sector;
  static uint8_t last_side;
  uint8_t physical_sector;
  uint8_t side;

  if (block < 20) {
    physical_sector = block / 2 + 1;
    side = 0;
  }
  else {
    physical_sector = (char)(block - 20) / 2 + 1;
    side = 1;
  }
  --track; // logical track numbers are 1-80, physical track numbers are 0-79

  FDC.command = FDC_CMD_CLR_BUFFER_PTRS;

  if (physical_sector != last_physical_sector || track != last_physical_track || side != last_side) {
    if (side == 0) {
      FDC.fdc_control |= FDC_SIDE_MASK; // select side 0
    }
    else {
      FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 1
    }

    //step_to_track(track);

    FDC.track = track;
    FDC.sector = physical_sector;
    FDC.side = side;
    FDC.command = FDC_CMD_READ_SECTOR;
    while(!(FDC.status & FDC_RDREQ_MASK));
    wait_for_busy_clear();
    while(!(FDC.status & (FDC_DRQ_MASK | FDC_EQ_MASK)));

    // RNF or CRC error flag check
    if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
      // error
      led_and_motor_off();
      fatal_error("Error reading sector");
    }

    last_physical_track = track;
    last_physical_sector = physical_sector;
    last_side = side;
  }

  if (block & 1) {
    FDC.fdc_control |= FDC_SWAP_MASK; // swap upper and lower halves of data buffer
  }
  else {
    FDC.fdc_control &= ~FDC_SWAP_MASK; // disable swap
  }
}

static void led_and_motor_off(void)
{
  FDC.fdc_control &= ~(FDC_MOTOR_MASK | FDC_LED_MASK); // disable LED and motor
}


// private init functions

#pragma clang section text="initcode" rodata="initcdata"
static uint8_t read_next_directory_block() 
{
  uint8_t next_track = FDC.data;
  uint8_t next_block = FDC.data;
  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t skip = 32 - read_file_entry();
    for (uint8_t j = 0; j < skip; ++j) {
      FDC.data;
    }
  }
  if (next_track == 0) {
    return 0;
  }

  load_block(next_track, next_block);
  return 1;
}

uint8_t read_file_entry()
{
  uint8_t i = 1;
  uint8_t tmp;

  if (FDC.data != 0x82) {
    // not a PRG file
    return i;   
  }
  ++i;
  uint8_t file_track = FDC.data;
  if (file_track == 0 || file_track > 80) {
    // invalid track number
    return i;
  }
  ++i;
  uint8_t file_block = FDC.data;
  if (file_block >= 40) {
    // invalid block number
    return i;
  }
  ++i;
  tmp = FDC.data;
  if (tmp < 0x30 || tmp > 0x39) {
    // invalid room number
    return i;
  }
  uint8_t room_number = (tmp - 0x30) * 10;
  ++i;
  tmp = FDC.data;
  if (tmp < 0x30 || tmp > 0x39) {
    // invalid room number
    return i;
  }
  room_number += tmp - 0x30;
  
  const char *file_suffix = ".LFL\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0";
  for (uint8_t j = 0; j < 14; ++j) {
    ++i;
    if (FDC.data != file_suffix[j]) {
      // invalid file suffix
      return i;
    }
  }

  // all checks passed, we found a valid xx.lfl file with xx being the room number
  track_list[room_number] = file_track;
  sector_list[room_number] = file_block;

  return i;
}
