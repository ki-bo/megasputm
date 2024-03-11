#include "diskio.h"
#include "dma.h"
#include "error.h"
#include "io.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <stdint.h>

// The diskio functions are banked in when used

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
 * BSS section: bss_init
 */
static struct {
  uint16_t magic_number;
  uint16_t num_global_game_objects;
  uint8_t global_game_objects[780];
  uint8_t num_room_resources;
  uint8_t room_disk_num[61];
  uint16_t room_offset[61];
  uint8_t num_costume_resources;
  uint8_t costume_room[40];
  uint16_t costume_offset[40];
  uint8_t num_script_resources;
  uint8_t script_room[179];
  uint16_t script_offset[179];
  uint8_t num_sound_resources;
  uint8_t sound_room[120];
  uint16_t sound_offset[120];
} lfl_index_file_contents;

#pragma clang section data="data_diskio" bss="bss_diskio"

/*
 * The index is in file 00.lfl and contains room numbers (=file numbers) and
 * offsets for each resource within that file. We cache this in memory to
 * speed up access to resources.
 * The numbers are hard-coded for the Maniac Mansion (Scumm V2) game.
 *
 * BSS section: bss_diskio
 */
static struct {
  uint8_t room_disk_num[61];
  uint16_t room_offset[61];
  
  uint8_t costume_room[40];
  uint16_t costume_offset[40];
  
  uint8_t script_room[179];
  uint16_t script_offset[179];

  uint8_t sound_room[120];
  uint16_t sound_offset[120];
} lfl_index;

static uint8_t room_track_list[54];
static uint8_t room_block_list[54];
static uint8_t current_track;
static uint8_t last_physical_track;
static uint8_t last_physical_sector;
static uint8_t last_side;
static uint8_t last_block;
static uint8_t next_track;
static uint8_t next_block;
static uint8_t cur_block_read_ptr;
static uint16_t cur_chunk_size;
static uint8_t drive_ready;
static uint8_t last_drive_access_frame;
static uint8_t drive_in_use;

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
#define FDC_BUF FAR_U8_PTR(0xffd6c00)
#define DISK_CACHE 0x8000000UL
#define UNBANKED_PTR(ptr) ((void __far *)((uint32_t)(ptr) + 0x10000UL))

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

// Private init functions
static uint8_t read_next_directory_block(void);
static uint8_t read_lfl_file_entry(void);
static void load_index(void);
static void invalidate_disk_cache(void);

// Private disk I/O functions
static void wait_for_busy_clear(void);
static void step_to_track(uint8_t track);
static void search_file(const char *filename);
static void seek_to(uint16_t offset);
static void load_block(uint8_t track, uint8_t block);
static void prepare_drive(void);
static void acquire_drive(void);
static void release_drive(void);
static void read_error(error_code_t error_code);
static void led_and_motor_off(void);

/**
 * @defgroup diskio_init Init Functions
 * @{
 */
#pragma clang section text="code_init" rodata="cdata_init" data="data_init" bss="bss_init"

/**
 * @brief Initialises the diskio module.
 *
 * This function must be called before any other diskio function.
 * It reads the start tracks and sectors of each room from the disk
 * directory and caches them in memory.
 * 
 * Code section: code_init
 */
void diskio_init(void)
{
  memset(room_track_list, 0, sizeof(room_track_list));
  memset(room_block_list, 0, sizeof(room_block_list));
  invalidate_disk_cache();
  last_physical_track = 255;
  drive_ready = 0;
  drive_in_use = 0;

  *NEAR_U8_PTR(0xd689) &= 0x7f; // see floppy buffer, not SD buffer
  prepare_drive();

  while (!(FDC.status & FDC_TK0_MASK)) {
    // not yet on track 0, so step outwards
    FDC.command = FDC_CMD_STEP_OUT;
    wait_for_busy_clear();
  }
  current_track = 0;

  // Loading file list in the directory, starting at track 40, block 3
  load_block(40, 3);
  while (read_next_directory_block() != 0);
  
  load_index();

  release_drive();
}

/**
 * @brief Parses lfl file entries in the FDC buffer and caches them
 * 
 * The function assumes a directory block has been loaded into the FDC buffer.
 * It will call read_lfl_file_entry() for each file entry in the buffer and
 * cache the track and block numbers for each valid lfl file entry.
 * Start track and sector of the file in the static variables room_track_list and
 * room_block_list.
 * 
 * The directory block contains up to eight file entries. Each file entry is 32
 * bytes long. If there are more blocks to read in the directory, the function will
 * automatically load the next block and return 1. If there are no more
 * blocks to read, the function will return 0.
 *
 * Code section: code_init
 * Private function
 *
 * @return uint8_t 0 if there are no more blocks to read, 1 otherwise
 */
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

/**
 * @brief Reads bytes from the sector buffer and parses them into a file entry.
 * 
 * The function reads bytes from the sector buffer and parses one file entry into
 * room_track_list and room_block_list. The function returns the number of bytes actually
 * read from the sector buffer.
 * 
 * @note The number of bytes actually read from the sector buffer can vary, 
 *       as the function will stop reading when it encounters the first invalid
 *       byte.
 *
 * Code section: code_init
 * Private function
 *
 * @return uint8_t The number of bytes actually read from the sector buffer
 */
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
  room_track_list[room_number] = file_track;
  room_block_list[room_number] = file_block;

  return i;
}

/**
 * @brief Loads the index from disk into memory.
 *
 * The index is in file 00.lfl and contains room numbers (=file numbers) and
 * offsets for each resource within that file. We cache this in memory to
 * speed up access to resources.
 *
 * Code section: code_init
 * Private function
 */
static void load_index(void)
{
  uint8_t bytes_left_in_block;
  next_track = room_track_list[0];
  next_block = room_block_list[0];
  uint8_t *address = (uint8_t *)&lfl_index_file_contents;

  while (next_track != 0) {
    load_block(next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    if (next_track == 0) {
      bytes_left_in_block = next_block - 1;
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

/**
 * @brief Marks all blocks in disk cache as not-available
 *
 * Disk cache is in attic ram, starting at 0x8000000. Each physical sector
 * is 512 bytes long. The cache can hold 20*80=1600 sectors.
 * The first two bytes of each block are used to store the track and block
 * number of the next block. Legal values are only 0-80 for the track
 * number (0 = last block). Therefore, we use the unsused value 0xff to mark 
 * a sector as not available (such a value should never occur on the disk
 * for the 'next track' byte).
 *
 * Code section: code_init
 * Private function
 */
static void invalidate_disk_cache(void)
{
  int8_t __huge *ptr = HUGE_I8_PTR(DISK_CACHE);

  for (uint16_t sector = 0; sector < 20 * 80; ++sector) {
    *ptr = -1; // mark cache sector as unused
    // ptr += 0x200;  <-- this is the original code, but results in "internal error: labeling failed"
    ptr = ptr + 0x200;
  }
}

/** @} */ // end of diskio_init

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup diskio_public Disk I/O Public Functions
 * @{
 */
#pragma clang section text="code_diskio" rodata="cdata_diskio" data="data_diskio" bss="bss_diskio"

/**
 * @brief Checks whether drive motor should be turned off
 * 
 * The function will check whether the drive has been accessed within the last 60 frames.
 * If not, it will turn off the drive motor and LED.
 * The function should be called regularly (several times per second) from the main loop, 
 * so the frame counter used for the check is correctly evaluated.
 *
 * Code section: code_diskio
 */
void diskio_check_motor_off(void)
{
  if (!drive_ready || drive_in_use) {
    return;
  }
  uint8_t cur_frame = FRAMECOUNT;
  uint8_t elapsed_frames = cur_frame - last_drive_access_frame;
  if (elapsed_frames < 60) {
    return;
  }
  led_and_motor_off();
}

/**
 * @brief Loads a file from disk into memory.
 * 
 * @param filename Null-terminated string containing the filename to load.
 * @param address Far pointer address in memory to load the file to.
 *
 * Code section: code_diskio
 */
void diskio_load_file(const char *filename, uint8_t __far *address)
{
  static dmalist_single_option_t dmalist_copy = {
    .opt_token = 0x80,
    .opt_arg = 0xff,
    .end_of_options = 0x00,
    .command = 0x00,      // DMA copy command
    .count = 0x00fe,
    .src_addr = 0x6c02,
    .src_bank = 0x0d,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  acquire_drive();

  search_file(filename);
  
  if (next_track == 0) {
    led_and_motor_off();
    fatal_error(ERR_FILE_NOT_FOUND);
  }

  dmalist_copy.count = 254;

  while (next_track != 0) {
    load_block(next_track, next_block);
    dmalist_copy.src_addr = next_block % 2 == 0 ? 0x6c02 : 0x6d02;

    next_track = FDC.data;
    next_block = FDC.data;

    if (next_track == 0) {
      dmalist_copy.count = next_block - 1;
    }
    dmalist_copy.dst_addr = (uint16_t)address;
    dmalist_copy.dst_bank = BANK(address);
    dma_trigger_far_ext(UNBANKED_PTR(&dmalist_copy));
    address += 254;
  }
  
  release_drive();
}

/**
 * @brief Loads a room from disk into memory.
 *
 * @param room The room number to load.
 * @param address The address in memory to load the room to.
 *
 * Code section: code_diskio
 */
void diskio_load_room(uint8_t room, __far uint8_t *address)
{
  acquire_drive();

  next_track = room_track_list[room];
  next_block = room_block_list[room];
  uint8_t payload_size = 254;

  while (next_track != 0) {
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
  
  release_drive();
}

/**
 * @brief Starts loading a resource from disk into memory.
 *
 * The function will start loading a resource from disk into memory. The resource
 * type and id are passed as parameters. The function will return the size of the
 * resource in bytes. The actual loading of the resource data into memory is done
 * by diskio_continue_resource_loading().
 *
 * Use this function to seek to the start of a resource and get its size. 
 * Memory needs to be allocated for the resource and the resource memory be
 * mapped to DEFAULT_RESOURCE_ADDRESS. Then call diskio_continue_resource_loading()
 * to load the resource data into memory.
 *
 * @param type The resource type (RES_TYPE_ROOM, RES_TYPE_COSTUME, RES_TYPE_SCRIPT, RES_TYPE_SOUND)
 * @param id The resource id
 *
 * @return uint16_t The size of the resource in bytes
 *
 * Code section: code_diskio
 */
uint16_t diskio_start_resource_loading(uint8_t type, uint8_t id)
{
  acquire_drive();

  uint8_t room_id = 0;
  uint16_t offset;

  switch (type) {
    case RES_TYPE_ROOM:
      room_id = id;
      offset = 0;
      break;
    case RES_TYPE_COSTUME:
      room_id = lfl_index.costume_room[id];
      offset = lfl_index.costume_offset[id];
      break;
    case RES_TYPE_SCRIPT:
      room_id = lfl_index.script_room[id];
      offset = lfl_index.script_offset[id];
      break;
    case RES_TYPE_SOUND:
      room_id = lfl_index.sound_room[id];
      offset = lfl_index.sound_offset[id];
      break;
  }

  if (room_id == 0) {
    led_and_motor_off();
    fatal_error(ERR_RESOURCE_NOT_FOUND);
  }

  load_block(room_track_list[room_id], room_block_list[room_id]);
  next_track = FDC.data;
  next_block = FDC.data;
  cur_block_read_ptr = 0;

  seek_to(offset);
  cur_chunk_size = FDC.data ^ 0xff;
  ++cur_block_read_ptr;
  if (cur_block_read_ptr == 254) {
    load_block(next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    cur_block_read_ptr = 0;
  }
  cur_chunk_size |= (FDC.data ^ 0xff) << 8;
  ++cur_block_read_ptr;

  return cur_chunk_size;
}

/**
 * @brief Continues loading a resource from disk into memory.
 *
 * The function will continue loading a resource from disk into memory. The resource
 * data is loaded into memory starting at DEFAULT_RESOURCE_ADDRESS. The size of the
 * resource data is passed as a parameter. The function will load the resource data
 * into memory and return when the resource data has been completely loaded.
 *
 * You need to call diskio_start_resource_loading() before calling this function.
 *
 * @param size The size of the resource in bytes
 *
 * Code section: code_diskio
 */
void diskio_continue_resource_loading(void)
{
  uint8_t *target_ptr = NEAR_U8_PTR(0x8000);
  *(uint16_t *)target_ptr = cur_chunk_size;
  target_ptr += 2;
  cur_chunk_size -= 2;

  while (cur_chunk_size != 0) {
    uint8_t bytes_left_in_block = next_track == 0 ? next_block - 1 : 254;
    bytes_left_in_block -= cur_block_read_ptr;
    
    uint8_t bytes_to_read = min(cur_chunk_size, bytes_left_in_block);
    for (uint8_t i = 0; i < bytes_to_read; ++i) {
      target_ptr[i] = FDC.data ^ 0xff;
    }
    
    cur_chunk_size -= bytes_to_read;

    if (cur_chunk_size != 0) {
      load_block(next_track, next_block);
      next_track = FDC.data;
      next_block = FDC.data;
      cur_block_read_ptr = 0;
      target_ptr += bytes_to_read;
    }

  }

  release_drive();
}

/** @} */ // end of diskio_public

//-----------------------------------------------------------------------------------------------

/**
 * @defgroup diskio_private Disk I/O Private Functions
 * @{
 */

/**
 * @brief Waits until the FDC busy flag is cleared
 *
 * Code section: code_diskio
 * Private function
 */
inline static void wait_for_busy_clear(void)
{
  while (FDC.status & FDC_BUSY_MASK);
}

/**
 * @brief Moves the floppy drive head to the provided track
 * 
 * @param track Physical track number (0-79)
 *
 * Code section: code_diskio
 * Private function
 */
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

/**
 * @brief Searches for a file in the directory.
 *
 * The function will provide start track and sector of the file in the static
 * variables next_track and next_block. If the file is not found, next_track
 * will be set to 0.
 *
 * 
 * @note The drive needs to be ready before calling this function. Use
 *       prepare_drive() to do that.
 *
 * @param filename Null-terminated string with the filename
 *
 * Code section: code_diskio
 * Private function
 */
static void search_file(const char *filename)
{
  uint8_t file_track;
  uint8_t file_block;
  next_track = 40;
  next_block = 3;

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
        continue;
      }
      --skip;
      file_track = FDC.data;
      if (file_track == 0 || file_track > 80) {
        // invalid track number
        continue;
      }
      --skip;
      file_block = FDC.data;
      if (file_block >= 40) {
        // invalid block number
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
          no_match = 1;
          break;
        }
      }
      if (no_match) {
        continue;
      }

      // file found, set next_track and next_block for reading it
      next_track = file_track;
      next_block = file_block;
      return;
    }
  }

  // file not found, next_track is 0
}

/**
 * @brief Seeks forward to a specific offset in the file
 *
 * The function will seek forward to the specified offset in the file, relative
 * to the current read position. The function will update the current read
 * position accordingly. The current track and block need to be loaded into the
 * FDC buffer before calling this function and next_track and next_block need to
 * be set to the next track and block after the current one.
 * cur_block_read_ptr needs to be set to the current read position in the block.
 *
 * When returning from this function, the data at the file read position will be
 * in the FDC buffer and cur_block_read_ptr will be set.
 * 
 * FDC.data can be used to continue reading bytes at the current read position.
 * 
 * @note cur_block_read_ptr is relative to the third byte of the block on disk, as
 *       the first two bytes are used to store the next track and block number.
 *
 * @param offset Offset in bytes relative to the current read position
 *
 * Code section: code_diskio
 * Private function
 */
static void seek_to(uint16_t offset)
{
  uint8_t bytes_left_in_block = (next_track == 0) ? next_block - 1 : 254;
  bytes_left_in_block -= cur_block_read_ptr;

  while (bytes_left_in_block <= offset) {
    load_block(next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    offset -= bytes_left_in_block;
    bytes_left_in_block = next_track == 0 ? next_block - 1 : 254;
    cur_block_read_ptr = 0;
  }

  cur_block_read_ptr += offset;

  while (offset-- != 0) {
    FDC.data;
  }
}

/**
 * @brief Loads a sector from disk cache or floppy disk into the floppy buffer
 *
 * The function will load a block from the disk cache or floppy disk into the
 * floppy buffer. The data of the block will be available in the floppy buffer
 * after the function returns. If the block is in the disk cache, the function
 * will load the block from the cache into the FDC buffer. If the block is not
 * in the cache, the function will read the block from the floppy disk into the
 * FDC buffer and store it in the cache.
 *
 * As sectors in 1581 format are 512 bytes, they always contain two logical
 * blocks. This function loads the sector containing the specified block into
 * the floppy buffer. Therefore, a second block is always implicitly
 * loaded into the floppy buffer as well.
 *
 * @param track Logical track number (1-80)
 * @param block Logical block number (0-39)
 *
 * Code section: code_diskio
 * Private function
 */
static void load_block(uint8_t track, uint8_t block)
{
  static dmalist_two_options_t dmalist_copy_from_cache = {
    .opt_token1 = 0x80,
    .opt_arg1 = 0x80,
    .opt_token2 = 0x81,
    .opt_arg2 = 0xff,
    .end_of_options = 0x00,
    .command = 0x00,      // DMA copy command
    .count = 0x0200,
    .src_addr = 0x0000,
    .src_bank = 0x00,
    .dst_addr = 0x6c00,
    .dst_bank = 0x0d
  };

  static dmalist_two_options_t dmalist_copy_to_cache = {
    .opt_token1 = 0x80,
    .opt_arg1 = 0xff,
    .opt_token2 = 0x81,
    .opt_arg2 = 0x80,
    .end_of_options = 0x00,
    .command = 0x00,      // DMA copy command
    .count = 0x0200,
    .src_addr = 0x6c00,
    .src_bank = 0x0d,
    .dst_addr = 0x0000,
    .dst_bank = 0x00
  };

  if (track == 0 || track > 80 || block > 39) {
    led_and_motor_off();
    fatal_error(ERR_INVALID_DISK_LOCATION);
  }

  uint8_t physical_sector;
  uint8_t side;

  physical_sector = block / 2;
  --track; // logical track numbers are 1-80, physical track numbers are 0-79

  const uint32_t cache_offset = (uint32_t)(track * 20 + physical_sector) * 512UL;
  const int8_t __far *cache_block = FAR_I8_PTR(DISK_CACHE + cache_offset);

  ++physical_sector;
  if (physical_sector > 10) {
    physical_sector -= 10;
    side = 1;
  }
  else {
    side = 0;
  }

  FDC.command = FDC_CMD_CLR_BUFFER_PTRS;

  if (*cache_block >= 0) {
    // block is in cache
    dmalist_copy_from_cache.src_addr = LSB16(cache_block);
    dmalist_copy_from_cache.src_bank = BANK(cache_block);
    dma_trigger_far_ext(UNBANKED_PTR(&dmalist_copy_from_cache));
    last_physical_track = track;
    last_physical_sector = physical_sector;
    last_side = side;
    last_block = block;
  }
  else {
    prepare_drive();
    // block is not in cache - need to read it from disk

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
        debug_out("unable to find track %d, sector %d, side %d\n", track, physical_sector, side);
        read_error(ERR_SECTOR_NOT_FOUND);
      }

      wait_for_busy_clear(); // sector reading to buffer completed when BUSY is cleared

      //if ((status & (FDC_RNF_MASK | FDC_CRC_MASK | FDC_DRQ_MASK | FDC_EQ_MASK)) != (FDC_DRQ_MASK | FDC_EQ_MASK)) {
      if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
        debug_out("FDC status: %04x", FDC.status)
        read_error(ERR_SECTOR_DATA_CORRUPT);
      }

      // copy the sector to the cache
      dmalist_copy_to_cache.dst_addr = LSB16(cache_block);
      dmalist_copy_to_cache.dst_bank = BANK(cache_block);
      dma_trigger_far_ext(UNBANKED_PTR(&dmalist_copy_to_cache));

      last_physical_track = track;
      last_physical_sector = physical_sector;
      last_side = side;

      last_drive_access_frame = FRAMECOUNT;
    }
    last_block = block;
  }

  if (block & 1) {
    FDC.fdc_control |= FDC_SWAP_MASK; // swap upper and lower halves of data buffer
  }
  else {
    FDC.fdc_control &= ~FDC_SWAP_MASK; // disable swap
  }
}

/**
 * @brief Makes sure the motor and led of the drive are ready
 *
 * The function will make sure the motor and LED of the drive are ready. If the
 * drive is not ready, the function will enable the motor and LED and spin up
 * the drive. It will block until the drive reports the motor completed
 * spinning up.
 * 
 * If the drive is already ready, the function will return immediately.
 *
 * @note The function implicitly calls acquire_drive()
 *
 * Code section: code_diskio
 * Private function
 */
static void prepare_drive(void)
{
  if (drive_ready) {
    return;
  }
  *NEAR_U8_PTR(0xd696) &= 0x7f; // disable auto-tune
  FDC.fdc_control |= FDC_MOTOR_MASK | FDC_LED_MASK; // enable LED and motor
  FDC.command = FDC_CMD_SPINUP;
  wait_for_busy_clear();
  drive_ready = 1;
  acquire_drive();
}

/**
 * @brief Marks the drive as currently being used.
 *
 * This prevents the drive motor from being turned off by diskio_check_motor_off().
 *
 * Code section: code_diskio
 * Private function
 */
static void acquire_drive(void)
{
  drive_in_use = 1;
}

/**
 * @brief Release the drive again after it has been used
 * 
 * The motor will not turn off immediately when calling this function, but
 * only after 60 frames have passed without the drive being accessed.
 * This prevents continuous spinning up and down of the drive when multiple
 * disk I/O operations are performed in quick succession.
 *
 * The main loop will regularly call diskio_check_motor_off() to turn off the
 * drive motor when it has not been accessed for 60 video frames.
 *
 * Code section: code_diskio
 * Private function
 */
static void release_drive(void)
{
  drive_in_use = 0;
}

/**
 * @brief Aborts the current disk I/O operation and reports a fatal error
 * 
 * @param error_code The error code to be reported via fatal_error()
 *
 * Code section: code_diskio
 * Private function
 */
static void read_error(error_code_t error_code)
{
  led_and_motor_off();
  fatal_error(error_code);
}

/**
 * @brief Turns off the drive motor and LED
 *
 * Code section: code_diskio
 * Private function
 */
static void led_and_motor_off(void)
{
  FDC.fdc_control &= ~(FDC_MOTOR_MASK | FDC_LED_MASK); // disable LED and motor
  drive_ready = 0;
}
