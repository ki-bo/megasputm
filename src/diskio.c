#include "diskio.h"
#include "dma.h"
#include "error.h"
#include "map.h"
#include "script.h"
#include "util.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// The diskio functions are banked in when used
// Use the bank_diskio() before calling any functions in this file
// Use unbank_diskio() to restore the previous bank

#pragma clang section bss="bss_init"

/**
 * @brief Temporary storage for the index file contents
 *
 * The index is in file 00.lfl and contains room numbers (=file numbers) and
 * offsets for each resource within that file.
 *
 * We put this into bss_init so it won't be taking up space after the init
 * routine is finished.
 *
 * @note BSS section bss_init
 */
static struct {
  uint16_t magic_number;
  uint16_t num_global_game_objects;
  uint8_t global_game_objects[780];
  uint8_t num_room_resources;
  uint8_t room_disk_num[61];
  uint16_t room_offset[61];
  uint8_t num_costume_resources;
  uint8_t costume_room[24];
  uint16_t costume_offset[24];
  uint8_t num_script_resources;
  uint8_t script_room[171];
  uint16_t script_offset[171];
  uint8_t num_sound_resources;
  uint8_t sound_room[66];
  uint16_t sound_offset[66];
} lfl_index_file_contents;

#pragma clang section bss="bss_diskio"

static uint8_t track_list[54];
static uint8_t sector_list[54];
static uint8_t current_track;
static uint8_t file_track;
static uint8_t file_block;

/*
 * The index is in file 00.lfl and contains room numbers (=file numbers) and
 * offsets for each resource within that file. We cache this in memory to
 * speed up access to resources.
 * The numbers are hard-coded for the Maniac Mansion (Scumm V2) game.
 */
static struct {
  uint8_t room_disk_num[61];
  uint16_t room_offset[61];
  
  uint8_t costume_room[24];
  uint16_t costume_offset[24];
  
  uint8_t script_room[171];
  uint16_t script_offset[171];

  uint8_t sound_room[66];
  uint16_t sound_offset[66];
} lfl_index;

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

// Debug functions
static void read_whole_track(uint8_t track);
static void scan_sector_order(uint8_t track, uint8_t side);

/**
 * @brief Searches for a file in the directory.
 *
 * The function will provide start track and sector of the file in the static
 * variables file_track and file_block. If the file is not found, file_track
 * will be set to 0.
 *
 * @note The drive needs to be ready before calling this function. Use
 *       prepare_drive() to do that.
 *
 * @param filename Null-terminated string with the filename
 *
 * @note Code section code_diskio
 */
static void search_file(const char *filename);

/**
 * @brief Loads a sector from the floppy disk into the floppy buffer.
 * 
 * As sectors in 1581 format are 512 bytes, they always contain two logical
 * blocks. This function loads the sector containing the specified block into
 * the floppy buffer. 
 *
 * @param track Logical track number (1-80)
 * @param block Logical block number (0-39)
 *
 * @note Code section code_diskio
 */
static void load_block(uint8_t track, uint8_t block);

static void read_error(void);

static void led_and_motor_off(void);

/**
 * @brief Loads the index from disk into memory.
 *
 * The index is in file 00.lfl and contains room numbers (=file numbers) and
 * offsets for each resource within that file. We cache this in memory to
 * speed up access to resources.
 *
 * @note Code section code_init
 */
static void load_index(void);

/**
 * @brief Parses lfl file entries in the FDC buffer and caches them
 * 
 * The function assumes a directory block has been loaded into the FDC buffer.
 * It will call read_lfl_file_entry() for each file entry in the buffer and
 * cache the track and block numbers for each valid lfl file entry.
 * Start track and sector of the file in the static variables file_track and
 * file_block.
 * 
 * The directory block contains up to eight file entries. Each file entry is 32
 * bytes long. If there are more blocks to read in the directory, the function will
 * automatically load the next block and return 1. If there are no more
 * blocks to read, the function will return 0.
 *
 * @note Code section code_init
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
 * @note Code section code_init
 *
 * @return uint8_t The number of bytes actually read from the sector buffer
 */
static uint8_t read_lfl_file_entry(void);





// ************************************************************************************************
// code_init functions
// ************************************************************************************************
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

//-----------------------------------------------------------------------------------------------

void diskio_init_entry(void)
{
  memset(track_list, 0, sizeof(track_list));
  memset(sector_list, 0, sizeof(sector_list));

  *NEAR_U8_PTR(0xd689) &= 0x7f; // see floppy buffer, not SD buffer
  prepare_drive();

  while (!(FDC.status & FDC_TK0_MASK)) {
    // not yet on track 0, so step outwards
    FDC.command = FDC_CMD_STEP_OUT;
    wait_for_busy_clear();
  }
  current_track = 0;

  // Loading file list in the directory, starting at track 40, block 3
  POKE(0xd020,0);
  debug_out("loading dir");
  load_block(40, 3);
  debug_msg("first block loaded");
  while (read_next_directory_block() != 0);
  
  load_index();

  led_and_motor_off();
}

//-----------------------------------------------------------------------------------------------

static uint8_t read_next_directory_block() 
{
  uint8_t next_track = FDC.data;
  uint8_t next_block = FDC.data;
  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t skip = 32 - read_lfl_file_entry();
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

//-----------------------------------------------------------------------------------------------

static uint8_t read_lfl_file_entry()
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

//-----------------------------------------------------------------------------------------------

static void load_index(void)
{
  uint8_t bytes_left_in_block;
  file_track = track_list[0];
  file_block = sector_list[0];
  uint8_t *address = (uint8_t *)&lfl_index_file_contents;

  while (file_track != 0) {
    load_block(file_track, file_block);
    file_track = FDC.data;
    file_block = FDC.data;
    if (file_track == 0) {
      bytes_left_in_block = file_block - 1;
    }
    else {
      bytes_left_in_block = 254;
    }

    while (bytes_left_in_block-- != 0) {
      *address = FDC.data ^ 0xff;
      ++address;
    }
  }

  memcpy(&global_game_objects, &lfl_index_file_contents.global_game_objects, sizeof(lfl_index_file_contents.global_game_objects));
  for (uint8_t i = 0; i < sizeof(lfl_index.room_disk_num); ++i) {
    lfl_index.room_disk_num[i] = lfl_index_file_contents.room_disk_num[i];
    lfl_index.room_offset[i] = lfl_index_file_contents.room_offset[i];
  }
  for (uint8_t i = 0; i < sizeof(lfl_index.costume_room); ++i) {
    lfl_index.costume_room[i] = lfl_index_file_contents.costume_room[i];
    lfl_index.costume_offset[i] = lfl_index_file_contents.costume_offset[i];
  }
  for (uint8_t i = 0; i < sizeof(lfl_index.script_room); ++i) {
    lfl_index.script_room[i] = lfl_index_file_contents.script_room[i];
    lfl_index.script_offset[i] = lfl_index_file_contents.script_offset[i];
  }
  for (uint8_t i = 0; i < sizeof(lfl_index.sound_room); ++i) {
    lfl_index.sound_room[i] = lfl_index_file_contents.sound_room[i];
    lfl_index.sound_offset[i] = lfl_index_file_contents.sound_offset[i];
  }
}


// ************************************************************************************************
// code_diskio functions
// ************************************************************************************************
#pragma clang section text="code_diskio" rodata="cdata_diskio" data="data_diskio" bss="bss_diskio"

//-----------------------------------------------------------------------------------------------

void diskio_load_file(const char *filename, uint8_t __far *address)
{
  static dmalist_single_option_t dmalist_copy = {
    .opt_token = 0x80,
    .opt_arg = 0xff,
    .end_of_options = 0x00,
    .command = 0x00,      //!< DMA copy command
    .count = 0x00fe,
    .src_addr = 0x6c02,
    .src_bank = 0x0d,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  debug_out("Loading file %s\n", filename);
  prepare_drive();

  search_file(filename);
  
  if (file_track == 0) {
    led_and_motor_off();
    fatal_error(ERR_FILE_NOT_FOUND);
  }

  DMA.addrbank = 1;
  DMA.addrmsb = MSB(&dmalist_copy);
  dmalist_copy.count = 254;

  while (file_track != 0) {
    debug_out("Loading track %d, block %d", file_track, file_block);
    load_block(file_track, file_block);
    dmalist_copy.src_addr = file_block % 2 == 0 ? 0x6c02 : 0x6d02;

    file_track = FDC.data;
    file_block = FDC.data;

    if (file_track == 0) {
      dmalist_copy.count = file_block - 1;
    }
    dmalist_copy.dst_addr = (uint16_t)address;
    dmalist_copy.dst_bank = BANK(address);
    DMA.etrig = LSB(&dmalist_copy);
    address += 254;
  }
  
  led_and_motor_off();
}

void diskio_load_room(uint8_t room, __far uint8_t *address)
{
  debug_out("Loading room %d\n", room);
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
    for (uint8_t i = 0; i < payload_size ; ++i) {
      *address = FDC.data ^ 0xff;
      ++address;
    }
  }
  
  led_and_motor_off();
}

//-----------------------------------------------------------------------------------------------

static void prepare_drive(void)
{
  *NEAR_U8_PTR(0xd696) &= 0x7f; // disable auto-tune
  FDC.fdc_control |= FDC_MOTOR_MASK | FDC_LED_MASK; // enable LED and motor
  FDC.command = FDC_CMD_SPINUP;
  wait_for_busy_clear();
}

//-----------------------------------------------------------------------------------------------

inline static void wait_for_busy_clear(void)
{
  while (FDC.status & FDC_BUSY_MASK);
}

//-----------------------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------------------------

static void scan_sector_order(uint8_t track, uint8_t side)
{
  --track;

  step_to_track(track);

  if (side == 0) {
    FDC.fdc_control |= FDC_SIDE_MASK; // select side 0
  }
  else {
    FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 1
  }

  uint8_t cur_sector = *NEAR_VU8_PTR(0xd6a4);
  while (cur_sector == *NEAR_VU8_PTR(0xd6a4));
  cur_sector = *NEAR_VU8_PTR(0xd6a4);
  while (cur_sector == *NEAR_VU8_PTR(0xd6a4));
  cur_sector = *NEAR_VU8_PTR(0xd6a4);
  uint8_t num_sectors_read = 0;
  uint8_t sectors[20];

  while (num_sectors_read < 20) {
    if (cur_sector != *NEAR_VU8_PTR(0xd6a4)) {
      // sector has been read
      cur_sector = *NEAR_VU8_PTR(0xd6a4);
      sectors[num_sectors_read] = cur_sector;
      ++num_sectors_read;
    }
  }

  //debug_out("%02d %02d %02d %02d %02d %02d %02d %02d %02d %02d", sectors[0], sectors[1], sectors[2], sectors[3], sectors[4], sectors[5], sectors[6], sectors[7], sectors[8], sectors[9]);
  //debug_out("%02d %02d %02d %02d %02d %02d %02d %02d %02d %02d", sectors[10], sectors[11], sectors[12], sectors[13], sectors[14], sectors[15], sectors[16], sectors[17], sectors[18], sectors[19]);
  //debug_out("\n");

}

//-----------------------------------------------------------------------------------------------

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
        read_error();
      }
    }

    sector_side_offset = 20;
    POKE(0xd020, 1);
    FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 0

  }
    POKE(0xd020, 0);
}

//-----------------------------------------------------------------------------------------------

static void search_file(const char *filename)
{
  uint8_t next_track = 40;
  uint8_t next_block = 3;

  while (next_track != 0)
  {
    load_block(next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    int8_t skip = 0;
    for (uint8_t i = 0; i < 8; ++i) {
      while (--skip >= 0) {
        FDC.data;
      }
      skip = 31;
      if (FDC.data != 0x82) {
        // not a PRG file
        debug_msg("not a PRG file");
        continue;
      }
      --skip;
      file_track = FDC.data;
      if (file_track == 0 || file_track > 80) {
        // invalid track number
        debug_msg("invalid track number");
        continue;
      }
      --skip;
      file_block = FDC.data;
      if (file_block >= 40) {
        // invalid block number
        debug_msg("invalid block number");
        continue;
      }
      uint8_t no_match = 0;
      uint8_t k;
      for (k = 0; k < 16; ++k) {
        if (filename[k] == '\0') {
          break;
        }
        --skip;
        if (FDC.data != filename[k]) {
          // not the file we are looking for
          debug_out("no match k=%d", k);
          no_match = 1;
          break;
        }
      }
      if (no_match) {
        continue;
      }
      for (; k < 16; ++k) {
        --skip;
        if (FDC.data != '\xa0') {
          // not the file we are looking for
          debug_msg("no match 2");
          no_match = 1;
          break;
        }
      }
      if (no_match) {
        continue;
      }

      return;
    }
  }

  // file not found
  file_track = 0;
}

//-----------------------------------------------------------------------------------------------

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

    step_to_track(track);

    FDC.track = track;
    FDC.sector = physical_sector;
    FDC.side = side;

    FDC.fdc_control &= ~FDC_SWAP_MASK; // disable swap so CPU read pointer will be reset to beginning of sector buffer
  
    FDC.command = FDC_CMD_READ_SECTOR;

    // RNF or CRC flags indicate an error (RNF = not found, CRC = data error)
    // DRQ and EQ flags both set indicates that the sector was completely read
    // We need to make sure the CPU read pointer is reset to the beginning of the sector buffer
    // (we made sure to disable SWAP first, otherwise the CPU read pointer will be reset to the
    // middle of the sector buffer and EQ won't get set)
    //
    // NOTE: While this is true for real drives, seems the FDC in the MEGA65 does not set EQ
    //       when the CPU read pointer is reset to the beginning of the sector buffer and
    //       disk images are used. So, we dont check for EQ when the sector is read.
    //       Usually, when BUSY is cleared, the status should report DRQ and EQ set.
    //       We just rely on the data being in the buffer once BUSY is cleared.
    while (!(FDC.status & (FDC_RDREQ_MASK | FDC_RNF_MASK | FDC_CRC_MASK))) continue; // wait for sector found (RDREQ) or error (RNF or CRC)
    if (!(FDC.status & FDC_RDREQ_MASK)) { // sector not found (RNF or CRC error)
      read_error();
    }

    wait_for_busy_clear(); // sector reading to buffer completed when BUSY is cleared

    //if ((status & (FDC_RNF_MASK | FDC_CRC_MASK | FDC_DRQ_MASK | FDC_EQ_MASK)) != (FDC_DRQ_MASK | FDC_EQ_MASK)) {
    if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
      read_error();
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

//-----------------------------------------------------------------------------------------------

static void read_error(void)
{
  led_and_motor_off();
  fatal_error(ERR_SECTOR_READ_FAILED);
}

//-----------------------------------------------------------------------------------------------

static void led_and_motor_off(void)
{
  FDC.fdc_control &= ~(FDC_MOTOR_MASK | FDC_LED_MASK); // disable LED and motor
}
