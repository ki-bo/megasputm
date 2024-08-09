/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "diskio.h"
#include "dma.h"
#include "error.h"
#include "index.h"
#include "io.h"
#include "map.h"
#include "resource.h"
#include "util.h"
#include "vm.h"
#include <string.h>
#include <stdint.h>

//-----------------------------------------------------------------------------------------------

#define FDC_BUF FAR_U8_PTR(0xffd6c00)
#define DISK_CACHE 0x8000000UL
#define UNBANKED_PTR(ptr) ((void __far *)((uint32_t)(ptr) + 0x10000UL))

//-----------------------------------------------------------------------------------------------

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
  uint8_t global_game_objects[NUM_GAME_OBJECTS];
  uint8_t num_room_resources;
  uint8_t room_disk_num[NUM_ROOMS];
  uint16_t room_offset[NUM_ROOMS];
  uint8_t num_costume_resources;
  uint8_t costume_room[NUM_COSTUMES];
  uint16_t costume_offset[NUM_COSTUMES];
  uint8_t num_script_resources;
  uint8_t script_room[NUM_SCRIPTS];
  uint16_t script_offset[NUM_SCRIPTS];
  uint8_t num_sound_resources;
  uint8_t sound_room[NUM_SOUNDS];
  uint16_t sound_offset[NUM_SOUNDS];
} lfl_index_file_contents;

  struct directory_entry {
    uint8_t  next_track;
    uint8_t  next_block;
    uint8_t  file_type;
    uint8_t  first_track;
    uint8_t  first_block;
    char     filename[16];
    uint16_t sss_block;
    uint8_t  record_length;
    uint8_t  unused[6];
    uint16_t file_size_blocks;
  };

  struct directory_block {
    struct directory_entry entries[8];
  };

//-----------------------------------------------------------------------------------------------

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
  uint8_t room_disk_num[NUM_ROOMS];
  uint16_t room_offset[NUM_ROOMS];
  
  uint8_t costume_room[NUM_COSTUMES];
  uint16_t costume_offset[NUM_COSTUMES];
  
  uint8_t script_room[NUM_SCRIPTS];
  uint16_t script_offset[NUM_SCRIPTS];

  uint8_t sound_room[NUM_SOUNDS];
  uint16_t sound_offset[NUM_SOUNDS];
} lfl_index;

struct bam_entry {
  uint8_t num_free_blocks;
  uint8_t block_usage[5];
};

struct bam_block {
  uint8_t next_track;
  uint8_t next_sector;
  uint8_t version;
  uint8_t version_1complement;
  uint16_t disk_id;
  uint8_t io_byte;
  uint8_t auto_boot_loader;
  uint8_t reserved[8];
  struct bam_entry bam_entries[40];
};

static uint16_t times_1600[MAX_DISKS + 1];
static uint8_t current_disk;
static uint8_t enable_prompt_for_disk_change;
static uint8_t room_list_disk_num;
static uint8_t room_track_list[54];
static uint8_t room_block_list[54];
static uint8_t current_track;
static uint8_t last_disk;
static uint8_t last_physical_track;
static uint8_t last_physical_sector;
static uint8_t last_side;
static uint8_t dir_cache_sectors;
static uint8_t next_track;
static uint8_t next_block;
static uint8_t cur_block_read_ptr;
static uint16_t cur_chunk_size;
static uint8_t drive_spinning;
static uint8_t jiffies_elapsed_since_last_drive_access;
static uint8_t drive_in_use;
static uint8_t writebuf_res_slot;
static uint8_t num_write_blocks;
static uint8_t write_file_first_track;
static uint8_t write_file_first_block;
static uint8_t write_file_current_track;
static uint8_t write_file_current_block;
static uint8_t *write_file_data_ptr;

// DMA lists
static dmalist_two_options_t   dmalist_copy_from_cache;
static dmalist_two_options_t   dmalist_copy_to_cache;
static dmalist_single_option_t dmalist_copy_block;

// Private init functions
static void init_dma_lists(void);
static uint8_t read_next_directory_block(uint8_t disk_num);
static uint8_t read_lfl_file_entry(void);
static void invalidate_disk_cache(void);

// Private disk I/O functions
static void wait_for_busy_clear(void);
static void check_and_prompt_for_disk(uint8_t disk_num);
static uint8_t check_disk(uint8_t disk_num);
static void read_directory(uint8_t disk_num);
static void step_to_track(uint8_t track);
static void search_file(const char *filename, uint8_t file_type);
static void seek_to(uint16_t offset);
static int8_t __far *get_cache_ptr(uint8_t disk_num, uint8_t track, uint8_t sector);
static void copy_sector_buf_to_cache(int8_t __far *cache_ptr);
static void copy_cache_to_sector_buf(int8_t __far *cache_ptr);
static void set_fdc_swap(uint8_t block);
static void load_block(uint8_t disk_num, uint8_t track, uint8_t block);
static void read_whole_track(uint8_t track);
static void prepare_drive(void);
static void acquire_drive(void);
static void release_drive(void);
static void disk_error(error_code_t error_code);
static void led_and_motor_off(void);
static uint16_t allocate_sector(uint8_t start_track);
static uint8_t find_free_block_on_track(uint8_t track);
static uint8_t find_free_sector(struct bam_entry *entry);
static void free_blocks(uint8_t track, uint8_t block);
static void free_block(uint8_t track, uint8_t block);
static void load_sector_to_bank(uint8_t disk_num, uint8_t track, uint8_t block, uint8_t __far *target);
static void write_block(uint8_t track, uint8_t block, uint8_t __far *block_data_far);
static void write_sector(uint8_t track, uint8_t sector, uint8_t __far *sector_buf_far);
static void write_sector_from_fdc_buf(uint8_t track, uint8_t sector);

/**
  * @defgroup diskio_init Disk I/O Init Functions
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
  * Will map CS to the diskio module and keep it mapped when the function returns.
  * 
  * Code section: code_init
  */
void diskio_init(void)
{
  MAP_CS_DISKIO

  init_dma_lists();

  for (uint8_t i = 0; i <= MAX_DISKS; ++i) {
    times_1600[i] = i * 1600;
  }

  current_disk                            = 0xff;
  room_list_disk_num                      = 0xff;
  last_physical_track                     = 0xff;
  drive_spinning                          = 0;
  drive_in_use                            = 0;
  enable_prompt_for_disk_change           = 0;
  jiffies_elapsed_since_last_drive_access = 0;

  memset(room_track_list, 0, sizeof(room_track_list));
  memset(room_block_list, 0, sizeof(room_block_list));

  prepare_drive();
  while (!(FDC.status & FDC_TK0_MASK)) {
    // not yet on track 0, so step outwards
    FDC.command = FDC_CMD_STEP_OUT;
    wait_for_busy_clear();
  }
  invalidate_disk_cache();
  release_drive();
}

/**
  * @brief Loads the index from disk into memory.
  *
  * The index is in file 00.lfl and contains room numbers (=file numbers) and
  * offsets for each resource within that file. We cache this in memory to
  * speed up access to resources.
  *
  * Code section: code_init
  */
uint8_t diskio_load_index(void)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_DISKIO

  // at this point, gfx should be initialised, so we can enable prompt for disk change
  enable_prompt_for_disk_change = 1;
  read_directory(0);

  uint8_t bytes_left_in_block;
  next_track = room_track_list[0];
  next_block = room_block_list[0];
  uint8_t *address = (uint8_t *)&lfl_index_file_contents;
  uint16_t num_bytes_expected = sizeof(lfl_index_file_contents);

  while (next_track != 0 && num_bytes_expected != 0) {
    load_block(0, next_track, next_block);
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
      --num_bytes_expected;
      if (num_bytes_expected == 0 && (next_track != 0 || bytes_left_in_block != 0)) {
          release_drive();
          return 0;
      }
    }
  }

  if (num_bytes_expected != 0) {
    release_drive();
    return 0;
  }

  memcpy(&vm_state.global_game_objects, &lfl_index_file_contents.global_game_objects, sizeof(lfl_index_file_contents.global_game_objects));
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

  release_drive();
  return 1;
}

static void init_dma_lists(void)
{
  dmalist_copy_from_cache = (dmalist_two_options_t){
    .opt_token1     = 0x80,
    .opt_arg1       = 0x80,
    .opt_token2     = 0x81,
    .opt_arg2       = 0xff,
    .end_of_options = 0x00,
    .command        = DMA_CMD_COPY,
    .count          = 0x0200,
    .src_addr       = 0x0000,
    .src_bank       = 0x00,
    .dst_addr       = 0x6c00,
    .dst_bank       = 0x0d
  };

  dmalist_copy_to_cache = (dmalist_two_options_t) {
    .opt_token1     = 0x80,
    .opt_arg1       = 0xff,
    .opt_token2     = 0x81,
    .opt_arg2       = 0x80,
    .end_of_options = 0x00,
    .command        = DMA_CMD_COPY,
    .count          = 0x0200,
    .src_addr       = 0x6c00,
    .src_bank       = 0x0d,
    .dst_addr       = 0x0000,
    .dst_bank       = 0x00
  };

  dmalist_copy_block = (dmalist_single_option_t) {
    .opt_token      = 0x80,
    .opt_arg        = 0xff,
    .end_of_options = 0x00,
    .command        = DMA_CMD_COPY,
    .count          = 0x00fe,
    .src_addr       = 0x6c02,
    .src_bank       = 0x0d,
    .dst_addr       = 0x0000,
    .dst_bank       = 0x00
  };
}

/**
  * @brief Marks all blocks in disk cache as not-available
  *
  * Disk cache is in attic ram, starting at 0x8000000. Each physical sector
  * is 512 bytes long. The cache can hold MAX_DISKS*20*80=MAX_DISKS*1600 sectors.
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

  uint16_t max_sector = times_1600[MAX_DISKS];

  for (uint16_t sector = 0; sector < max_sector; ++sector) {
    *ptr = -1; // mark cache sector as unused
    ptr += 0x200;
  }
}

/** @} */ // end of diskio_init

//-----------------------------------------------------------------------------------------------

/**
  * @defgroup diskio_public Disk I/O Public Functions
  * @{
  */
#pragma clang section text="code_diskio" rodata="cdata_diskio" data="data_diskio" bss="bss_diskio"

uint8_t diskio_is_real_drive(void)
{
  return *(uint8_t *)(0xd6a1) & 1;
}

void diskio_switch_to_real_drive(void)
{
  *(uint8_t *)(0xd68b) &= ~1;
  *(uint8_t *)(0xd6a1) |= 1;
  *NEAR_U8_PTR(0xd696) &= 0x7f; // disable auto-tune
  FDC.fdc_control |= FDC_MOTOR_MASK | FDC_LED_MASK; // enable LED and motor
  FDC.command = FDC_CMD_SPINUP;
  wait_for_busy_clear();
  while (!(FDC.status & FDC_TK0_MASK)) {
    // not yet on track 0, so step outwards
    FDC.command = FDC_CMD_STEP_OUT;
    wait_for_busy_clear();
  }
  current_track = 0;
}

/**
  * @brief Checks whether drive motor should be turned off
  * 
  * The function will check whether the drive has been accessed within the last 60 jiffies.
  * If not, it will turn off the drive motor and LED.
  * The function should be called regularly (several times per second) from the main loop, 
  * so the jiffy counter used for the check is correctly evaluated.
  *
  * The function will accumulate the elapsed jiffies since the last call and turn off the
  * drive motor and LED if the accumulated jiffies exceed 60.
  *
  * @param elapsed_jiffies The number of jiffies elapsed since the last call to this function
  *
  * Code section: code_diskio
  */
void diskio_check_motor_off(uint8_t elapsed_jiffies)
{
  if (!drive_spinning || drive_in_use) {
    // motor is already off or we are asked to keep it on
    return;
  }

  jiffies_elapsed_since_last_drive_access += elapsed_jiffies;
  if (jiffies_elapsed_since_last_drive_access < 60) {
    return;
  }
  led_and_motor_off();
  jiffies_elapsed_since_last_drive_access = 0;
}

uint8_t diskio_file_exists(const char *filename)
{
  search_file(filename, 0x81);
  release_drive();
  return next_track != 0;
}

/**
  * @brief Loads a file from disk into memory.
  * 
  * @param filename Null-terminated string containing the filename to load.
  * @param address Far pointer address in memory to load the file to.
  *
  * Code section: code_diskio
  */
void diskio_load_file(uint8_t disk_num, const char *filename, uint8_t __far *address)
{
  check_and_prompt_for_disk(disk_num);

  search_file(filename, 0x82);
  
  if (next_track == 0) {
    disk_error(ERR_FILE_NOT_FOUND);
  }

  dmalist_copy_block.count = 254;

  while (next_track != 0) {
    load_block(disk_num, next_track, next_block);
    dmalist_copy_block.src_addr = next_block % 2 == 0 ? 0x6c02 : 0x6d02;

    next_track = FDC.data;
    next_block = FDC.data;

    if (next_track == 0) {
      dmalist_copy_block.count = next_block - 1;
    }
    dmalist_copy_block.dst_addr = (uint16_t)address;
    dmalist_copy_block.dst_bank = BANK(address);
    dma_trigger(&dmalist_copy_block);
    address += 254;
  }
  
  release_drive();
}

/**
  * @brief Loads the global game objects from disk into memory.
  *
  * The global game objects are stored in the index file 00.lfl. The function
  * will load the global game objects from the index file into memory.
  *
  * Code section: code_diskio
  */
void diskio_load_game_objects(void)
{
  uint8_t bytes_left_in_block;
  int16_t num_bytes_left;
  read_directory(0);

  next_track = room_track_list[0];
  next_block = room_block_list[0];

  uint8_t first = 1;
  uint8_t *address = (uint8_t *)&vm_state.global_game_objects;

  do {
    load_block(0, next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    if (next_track == 0) {
      bytes_left_in_block = next_block - 1;
    }
    else {
      bytes_left_in_block = 254;
    }

    if (first) {
      first = 0;

      // skip magic number, 2 bytes
      FDC.data;
      FDC.data;

      // number of global game objects, 2 bytes
      uint8_t low  = FDC.data ^ 0xff;
      uint8_t high = FDC.data ^ 0xff;
      num_bytes_left = make16(low, high);

      num_bytes_left <<= 1;
      bytes_left_in_block -= 4;
    }

    uint8_t bytes_to_read = min(bytes_left_in_block, num_bytes_left);
    num_bytes_left -= bytes_to_read;
    while (bytes_to_read != 0) {
      *address++ = FDC.data ^ 0xff;
      --bytes_to_read;
    }
  }
  while (next_track != 0 && num_bytes_left > 0);

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
  * @return The size of the resource in bytes
  *
  * Code section: code_diskio
  */
uint16_t diskio_start_resource_loading(uint8_t type, uint8_t id)
{
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
    disk_error(ERR_RESOURCE_NOT_FOUND);
  }

  // debug_out("res t%d i%d r%d t%d b%d", type, id, room_id, room_track_list[room_id], room_block_list[room_id]);

  // check whether requested file is on current disk
  if (room_track_list[room_id] == 0) {
    // it is not available, determine needed disk number and prompt for disk
    uint8_t disk_num = lfl_index.room_disk_num[room_id] - 0x31;
    if (disk_num >= MAX_DISKS) {
      disk_error(ERR_DISK_NUM_OUT_OF_RANGE);
    }
    read_directory(disk_num);
    if (room_track_list[room_id] == 0) {
      disk_error(ERR_LFL_FILE_NOT_FOUND);
    }
  }

  // the requested file is on the current disk
  load_block(room_list_disk_num, room_track_list[room_id], room_block_list[room_id]);
  next_track = FDC.data;
  next_block = FDC.data;
  cur_block_read_ptr = 0;

  seek_to(offset);
  uint8_t chunksize_low = FDC.data ^ 0xff;
  ++cur_block_read_ptr;
  if (cur_block_read_ptr == 254) {
    load_block(room_list_disk_num, next_track, next_block);
    next_track = FDC.data;
    next_block = FDC.data;
    cur_block_read_ptr = 0;
  }
  uint8_t chunksize_high = FDC.data ^ 0xff;
  cur_chunk_size = make16(chunksize_low, chunksize_high);
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
void diskio_continue_resource_loading(uint8_t __huge *target_ptr)
{
  *(uint16_t __huge *)target_ptr = cur_chunk_size;
  target_ptr += 2;
  cur_chunk_size -= 2;

  while (cur_chunk_size != 0) {
    uint8_t bytes_left_in_block = next_track == 0 ? next_block - 1 : 254;
    bytes_left_in_block -= cur_block_read_ptr;
    
    uint8_t bytes_to_read = min(cur_chunk_size, bytes_left_in_block);
    for (uint8_t i = 0; i < bytes_to_read; ++i) {
      *target_ptr++ = FDC.data ^ 0xff;
    }
    
    cur_chunk_size -= bytes_to_read;

    if (cur_chunk_size != 0) {
      load_block(room_list_disk_num, next_track, next_block);
      next_track = FDC.data;
      next_block = FDC.data;
      cur_block_read_ptr = 0;
    }

  }

  release_drive();
}

void diskio_open_for_reading(const char *filename, uint8_t file_type)
{
  search_file(filename, file_type);
  
  if (next_track == 0) {
    disk_error(ERR_FILE_NOT_FOUND);
  }

  // load first block of file, configuring disk to 0xff will effectively disable caching
  load_block(0xff, next_track, next_block);
  next_track = FDC.data;
  next_block = FDC.data;
  cur_block_read_ptr = 0;
}

void diskio_read(uint8_t *target_ptr, uint16_t size)
{
  while (size) {
    uint8_t bytes_left_in_block = next_track == 0 ? next_block - 1 : 254;
    bytes_left_in_block -= cur_block_read_ptr;
    
    uint8_t bytes_to_read = min(size, bytes_left_in_block);
    for (uint8_t i = 0; i < bytes_to_read; ++i) {
      *target_ptr = FDC.data;
      //debug_out("%x=%x", ((uint16_t)target_ptr), *target_ptr);
      ++target_ptr;
    }
    
    size -= bytes_to_read;
    cur_block_read_ptr += bytes_to_read;

    if (size != 0) {
      if (next_track == 0) {
        disk_error(ERR_FILE_READ_BEYOND_EOF);
      }
      load_block(0xff, next_track, next_block);
      next_track = FDC.data;
      next_block = FDC.data;
      cur_block_read_ptr = 0;
    }
  }
}

void diskio_close_for_reading(void)
{
  release_drive();
}

void diskio_open_for_writing(void)
{
  // allocating 4 blocks (2 for bam and 2 as sector write buffers)
  writebuf_res_slot = res_reserve_heap(4);

  // load BAM
  load_block(0xff, 40, 1);
  memcpy_far((void __far *)res_get_huge_ptr(writebuf_res_slot)    , (void __far *)0xffd6d00, 0x100);
  load_block(0xff, 40, 2);
  memcpy_far((void __far *)res_get_huge_ptr(writebuf_res_slot + 1), (void __far *)0xffd6c00, 0x100);

  num_write_blocks         = 0;
  write_file_first_track   = 0;
  write_file_current_track = 39; // start searching for free blocks on track 39
  write_file_data_ptr      = NEAR_U8_PTR(0x8400); // end of buffer to indicate new sector needs to get allocated
}

void diskio_write(const uint8_t __huge *data, uint16_t size)
{
  if (!size) {
    return;
  }

  SAVE_DS_AUTO_RESTORE

  uint8_t *ts_field;
  uint8_t *sector_buf_ptr = write_file_data_ptr;

  map_ds_resource(writebuf_res_slot);
  uint8_t __far *sector_buf_far = (void __far *)res_get_huge_ptr(writebuf_res_slot + 2);

  while (size != 0) {
    if (sector_buf_ptr == NEAR_U8_PTR(0x8300)) {
      // switch to next block in the sector
      ts_field        = NEAR_U8_PTR(0x8200);
      ts_field[0]     = write_file_current_track;
      ts_field[1]     = write_file_current_block + 1;
      sector_buf_ptr += 2; // skip over the track and block fields
    }
    else if (sector_buf_ptr == NEAR_U8_PTR(0x8400)) {
      // write pointer is at the end of the buffer,
      // allocate next sector (=2 blocks) for the file
      uint16_t track_sector = allocate_sector(write_file_current_track);
      if (track_sector == 0) {
        disk_error(ERR_DISK_FULL);
      }
      uint8_t write_file_next_track = MSB(track_sector);
      uint8_t write_file_next_block = LSB(track_sector) * 2;
      //debug_out("Allocated block %d:%d\n", write_file_next_track, write_file_next_block);

      if (write_file_first_track == 0) {
        write_file_first_track = write_file_next_track;
        write_file_first_block = write_file_next_block;
      }
      else {
        ts_field    = NEAR_U8_PTR(0x8300); // start of second block in sector
        ts_field[0] = write_file_next_track;
        ts_field[1] = write_file_next_block;
        // write current (full) sector to disk
        write_sector(write_file_current_track, write_file_current_block / 2, sector_buf_far);
      }

      write_file_current_track = write_file_next_track;
      write_file_current_block = write_file_next_block;
      sector_buf_ptr           = NEAR_U8_PTR(0x8200); // start of first block in sector
      
      memset(sector_buf_ptr, 0, 0x200);
      sector_buf_ptr += 2; // skip over the track and block fields
      num_write_blocks += 2;
    }

    *sector_buf_ptr++ = *data++;
    --size;
  }

  write_file_data_ptr = sector_buf_ptr;
}

void diskio_close_for_writing(const char *filename, uint8_t file_type)
{
  if (write_file_first_track == 0) {
    // no data written, nothing to do
    res_free_heap(writebuf_res_slot);
    release_drive();
    return;
  }

  SAVE_DS_AUTO_RESTORE

  uint8_t filename_len       = strlen(filename);
  uint8_t filename_match     = 0;
  uint8_t dir_track          = 40;
  uint8_t dir_block          = 3;
  uint8_t old_file_track     = 0;
  uint8_t old_file_block;

  // make bam and additional block buffers available at 0x8000
  map_ds_resource(writebuf_res_slot);
  // sector_buf_far is the unbanked pointer to the mapped memory at 0x8200 (needed for DMA)
  __auto_type sector_buf_far = (uint8_t __far *)res_get_huge_ptr(writebuf_res_slot + 2);

  // finalize and write last sector to disk
  uint8_t *ts_field;
  if (write_file_data_ptr <= NEAR_U8_PTR(0x8300)) {
    ts_field = NEAR_U8_PTR(0x8200);
    free_block(write_file_current_track, write_file_current_block + 1);
    --num_write_blocks;
  }
  else {
    ts_field = NEAR_U8_PTR(0x8300);
  }
  ts_field[0] = 0;
  ts_field[1] = LSB(write_file_data_ptr) - 1;
  write_sector(write_file_current_track, write_file_current_block / 2, sector_buf_far);
  
  // search for filename in directory
  load_sector_to_bank(0xff, dir_track, dir_block, sector_buf_far);

  __auto_type dir_block_data = (struct directory_block *)(0x8300);

  do {
    struct directory_entry *free_dir_entry = NULL;
    for (uint8_t i = 0; i < 8; ++i) {
      __auto_type cur_entry = &dir_block_data->entries[i];
      //debug_out(" type %x filename %d: %s", cur_entry->file_type, i, cur_entry->filename);
      if (!free_dir_entry && cur_entry->file_type == 0) {
        free_dir_entry = cur_entry;
      }
      else if (cur_entry->file_type == file_type) {
        filename_match = 1;
        __auto_type dir_filename = cur_entry->filename;
        for (uint8_t j = 0; j < 16; ++j) {
          if (j < filename_len) {
            if (filename[j] != dir_filename[j]) {
              filename_match = 0;
              break;
            }
          }
          else {
            if (dir_filename[j] != '\xa0') {
              filename_match = 0;
              break;
            }
          }
        }
        if (filename_match) {
          // found existing file entry, overwrite it
          old_file_track              = cur_entry->first_track;
          old_file_block              = cur_entry->first_block;
          cur_entry->first_track      = write_file_first_track;
          cur_entry->first_block      = write_file_first_block;
          cur_entry->file_size_blocks = num_write_blocks;
          break;
        }
      }
    }

    if (!filename_match) {
      // filename not found in current directory block, read next one
      __auto_type entry = &dir_block_data->entries[0];
      dir_track = entry->next_track;
      if (dir_track != 0) {
        if (dir_track != 40) {
          filename_match = 0;
          break;
        }
        //debug_out(" going to next block");
        dir_block = entry->next_block;
        load_sector_to_bank(0xff, dir_track, dir_block, sector_buf_far);
        dir_block_data = (struct directory_block *)(dir_block & 1 ? 0x8300 : 0x8200);
      }
      else {
        if (free_dir_entry) {
          // no more directory blocks, but available entry in current block
          memset(free_dir_entry, 0, sizeof(struct directory_entry));
        }
        else {
          // no more directory blocks, and last block already full, create new one
          uint8_t new_dir_block = find_free_block_on_track(40);
          if (new_dir_block == 0xff) {
            filename_match = 0;
            break;
          }
          //debug_out(" chaining new block %d:%d to old block %d:%d", 40, new_dir_block, 40, dir_block);
          // reference new block in first entry of current block
          entry->next_track = 40;
          entry->next_block = new_dir_block;

          if ((dir_block / 2) != (new_dir_block / 2)) {
            // the last block and the new block are not in the same sector, write back the old block
            //debug_out(" not in same sector, writing back old block");
            write_sector(40, dir_block / 2, sector_buf_far);
            // load block first to get the other block in that sector into the FDC buffer
            //debug_out(" loading new block %d", new_dir_block);
            load_sector_to_bank(0xff, 40, new_dir_block, sector_buf_far);
          }

          dir_block = new_dir_block;
          dir_block_data = (struct directory_block *)(dir_block & 1 ? 0x8300 : 0x8200);

          // erase newly allocated block
          memset(dir_block_data, 0, 0x100);

          free_dir_entry             = &dir_block_data->entries[0];
          free_dir_entry->next_block = 0xff;
        }

        free_dir_entry->file_type        = file_type;
        free_dir_entry->first_track      = write_file_first_track;
        free_dir_entry->first_block      = write_file_first_block;
        free_dir_entry->file_size_blocks = num_write_blocks;

        __auto_type dir_filename = free_dir_entry->filename;
        for (uint8_t i = 0; i < 16; ++i) {
          if (i < filename_len) {
            dir_filename[i] = filename[i];
          }
          else {
            dir_filename[i] = '\xa0';
          }
        }

        filename_match = 1;
      }
    }
  }
  while (!filename_match);

  if (!filename_match) {
    disk_error(ERR_DISK_FULL);
  }

  // free blocks of old file in case it was overwritten with new sectors
  if (old_file_track != 0) {
    free_blocks(old_file_track, old_file_block);
  }

  // write back BAM to disk
  __auto_type bam_block_far = FAR_U8_PTR(sector_buf_far) - 0x200;
  write_block(40, 1, bam_block_far);
  write_block(40, 2, bam_block_far + 0x100);

  // write back directory block to disk
  write_sector(40, dir_block / 2, sector_buf_far);

  res_free_heap(writebuf_res_slot);

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

static void check_and_prompt_for_disk(uint8_t disk_num)
{
  // if drive is already spinning with confirmed correct disk, we're done
  if (drive_spinning && current_disk == disk_num) {
    return;
  }

  // check whether correct disk is inserted
  uint8_t disk_found = 0;
  while (!disk_found) {
    last_physical_track = 0xff;
    disk_found = check_disk(disk_num);
    if (!disk_found) {
      if (enable_prompt_for_disk_change) {
        vm_handle_error_wrong_disk(disk_num + 1);
      }
      else {
        disk_error(ERR_WRONG_DISK);
      }
    }
  }

  current_disk = disk_num;
}
static uint8_t check_disk(uint8_t disk_num)
{
  load_block(0xff, 40, 0);
  for (uint8_t i = 0; i < sizeof(disk_header); ++i) {
    uint8_t read_byte = FDC.data;
    if (i == 23) {
      if (read_byte != 0x30 + disk_num + 1) {
        return 0;
      }
    }
    else {
      if (read_byte != disk_header[i]) {
        return 0;
      }
    }
  }
  return 1;
} 

static void read_directory(uint8_t disk_num)
{
  memset(room_track_list, 0, sizeof(room_track_list));
  memset(room_block_list, 0, sizeof(room_block_list));

  // Loading file list in the directory, starting at track 40, block 3, disable caching
  load_block(disk_num, 40, 3);
  while (read_next_directory_block(disk_num) != 0);
  room_list_disk_num = disk_num;
  last_physical_track = 0xff;
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
  * Code section: code_diskio
  * Private function
  *
  * @return 0 if there are no more blocks to read, 1 otherwise
  */
static uint8_t read_next_directory_block(uint8_t disk_num) 
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

  load_block(disk_num, next_track, next_block);
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
  * Code section: code_diskio
  * Private function
  *
  * @return The number of bytes actually read from the sector buffer
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
static void search_file(const char *filename, uint8_t file_type)
{
  int8_t __far *dir_cache_ptr = get_cache_ptr(MAX_DISKS, 0, 0);
  uint8_t sectors_read = 0;
  uint8_t file_track;
  uint8_t file_block;
  next_track = 40;
  next_block = 3;

  while (next_track != 0)
  {
    // debug_out("sr %d dcs%d dcp%lx", sectors_read, dir_cache_sectors, (uint32_t)dir_cache_ptr);
    if (sectors_read < dir_cache_sectors) {
      // debug_out("dir cache %d %d", next_track, next_block);
      copy_cache_to_sector_buf(dir_cache_ptr);
      set_fdc_swap(next_block);
      last_physical_track = 0xff;
    }
    else {
      load_block(0xff, next_track, next_block);
      copy_sector_buf_to_cache(dir_cache_ptr);
      ++dir_cache_sectors;
    }
    ++sectors_read;
    dir_cache_ptr += 512;

    next_track = FDC.data;
    next_block = FDC.data;
    int8_t skip = 0;
    for (uint8_t i = 0; i < 8; ++i) {
      while (--skip >= 0) {
        FDC.data;
      }
      skip = 31;
      if (FDC.data != file_type) {
        // file type mismatch
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
    load_block(room_list_disk_num, next_track, next_block);
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

static int8_t __far *get_cache_ptr(uint8_t disk_num, uint8_t track, uint8_t sector)
{
  uint16_t cache_block = times_1600[disk_num];
  cache_block += track * 20;
  cache_block += sector;
  uint32_t cache_offset = (uint32_t)cache_block * 512UL;
  return FAR_I8_PTR(DISK_CACHE + cache_offset);
}

static void copy_sector_buf_to_cache(int8_t __far *cache_ptr)
{
  dmalist_copy_to_cache.dst_addr = LSB16(cache_ptr);
  dmalist_copy_to_cache.dst_bank = BANK(cache_ptr);
  dmalist_copy_to_cache.opt_arg2 = (uint8_t)0x80 | MB_LO(cache_ptr);
  dma_trigger(&dmalist_copy_to_cache);
}

static void copy_cache_to_sector_buf(int8_t __far *cache_ptr)
{
  dmalist_copy_from_cache.src_addr = LSB16(cache_ptr);
  dmalist_copy_from_cache.src_bank = BANK(cache_ptr);
  dmalist_copy_from_cache.opt_arg1 = (uint8_t)0x80 | MB_LO(cache_ptr);
  dma_trigger(&dmalist_copy_from_cache);
}

static void set_fdc_swap(uint8_t block)
{
  FDC.command = FDC_CMD_CLR_BUFFER_PTRS;
  if (block & 1) {
    FDC.fdc_control |= FDC_SWAP_MASK; // swap upper and lower halves of data buffer
  }
  else {
    FDC.fdc_control &= ~FDC_SWAP_MASK; // disable swap
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
  * The floppy buffer read pointer will be reset to point to the first byte of
  * the block after the function returns. Thus, reading from the floppy buffer
  * with FDC.data will return the first byte of the block.
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
static void load_block(uint8_t disk_num, uint8_t track, uint8_t block)
{
  //debug_out("ldbl d%d t%d b%d", disk_num, track, block);
  if (track == 0 || track > 80 || block > 39) {
    disk_error(ERR_INVALID_DISK_LOCATION);
  }

  uint8_t use_cache = (disk_num < MAX_DISKS) ? 1 : 0;

  uint8_t physical_sector;
  uint8_t side;

  physical_sector = block / 2;
  --track; // logical track numbers are 1-80, physical track numbers are 0-79

  __auto_type cache_block = get_cache_ptr(disk_num, track, physical_sector);

  ++physical_sector;
  if (physical_sector > 10) {
    physical_sector -= 10;
    side = 1;
  }
  else {
    side = 0;
  }

  //FDC.command = FDC_CMD_CLR_BUFFER_PTRS;
  *NEAR_U8_PTR(0xd689) &= 0x7f; // see floppy buffer, not SD buffer

  if (use_cache && *cache_block >= 0) {
    // block is in cache
    copy_cache_to_sector_buf(cache_block);
  }
  else {
    if (use_cache) {
      check_and_prompt_for_disk(disk_num);
    }
    else {
      current_disk = 0xff;
    }
    //FDC.command = FDC_CMD_CLR_BUFFER_PTRS;
    if (use_cache && diskio_is_real_drive()) {
      read_whole_track(track);
      if (*cache_block < 0) { // should never happen
        disk_error(ERR_READ_TRACK_FAILED);
      }
      // block is in cache
      copy_cache_to_sector_buf(cache_block);
    }
    else {
      // we are either on a virtual disk image or have cache disabled
      prepare_drive();
      //debug_out("nocach");

      if (disk_num != last_disk || 
          physical_sector != last_physical_sector || 
          track != last_physical_track || 
          side != last_side) {
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

        set_fdc_swap(0); // disable swap so CPU read pointer will be reset to beginning of sector buffer
      
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
          //debug_out("unable to find track %d, sector %d, side %d\n", track, physical_sector, side);
          disk_error(ERR_SECTOR_NOT_FOUND);
        }

        wait_for_busy_clear(); // sector reading to buffer completed when BUSY is cleared

        //if ((status & (FDC_RNF_MASK | FDC_CRC_MASK | FDC_DRQ_MASK | FDC_EQ_MASK)) != (FDC_DRQ_MASK | FDC_EQ_MASK)) {
        if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
          //debug_out("FDC status: %x", FDC.status)
          disk_error(ERR_SECTOR_DATA_CORRUPT);
        }

        // copy the sector to the cache
        if (use_cache) {
          copy_sector_buf_to_cache(cache_block);
        }

        // reset jiffy counter for drive access check
        jiffies_elapsed_since_last_drive_access = 0;
      }
    }
  }

  last_disk = disk_num;
  last_physical_track = track;
  last_physical_sector = physical_sector;
  last_side = side;

  set_fdc_swap(block);
}

/**
  * @brief Reads a whole track from the floppy disk into the cache.
  *
  * The function will read a whole track from the floppy disk into the cache.
  * In most cases, this is the fastest way to read sectors of a file as those
  * typically are stored in sequential sectors of tracks. read_block() is automatically
  * calling this function if an uncached block is requested and the physical drive is in
  * use (as there usually is no performance gain in reading a whole track at once with
  * virtual disk images).
  *
  * The track to be read is specified with its physical track number. The cache
  * is updated for the disk selected via the global variable current_disk.
  * If the cache is disabled, this function should not be called, as it doesn't
  * check for the cache being disabled.
  *
  * The function will read all 20 sectors of the track starting at physical sector
  * 1 of side 0 up to physical sector 10. Then, it will read the 10 sectors of side 1.
  * 
  * The variables last_physical_track, last_physical_sector and last_side will be
  * updated to reflect the last track and sector read. This enables other functions like
  * read_block() to check whether the requested block is already in the drive's sector buffer.
  *
  * Note that this function only works with real floppy drives and will not work with virtual
  * disk images.
  *
  * @param track Physical track number (0-79)
  */
static void read_whole_track(uint8_t track)
{
  char buf[512];

  prepare_drive();
  step_to_track(track);
  FDC.fdc_control &= ~FDC_SWAP_MASK; // disable swap so CPU read pointer will be reset to beginning of sector buffer
  FDC.fdc_control |= FDC_SIDE_MASK; // select side 0
  int8_t __far *cache_block = NULL;
  uint8_t sector = 1;
  uint8_t side   = 0;

  for (uint8_t s = 0; s < 21; ++s, ++sector) {

    if (s != 0 && cache_block && *cache_block == -1) {
      memcpy_far(cache_block, (uint8_t __far *)buf, 512);
    }
    if (s == 10) {
      FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 1
      side = 1;
      sector -= 10;
    } else if (s == 20) {
      break;
    }

    FDC.track  = current_track;
    FDC.sector = sector;
    FDC.side   = side;

    //debug_out("loading track %d, sector %d, side %d\n", current_track, s + 1, side);
    FDC.command = FDC_CMD_CLR_BUFFER_PTRS;
    FDC.command = FDC_CMD_READ_SECTOR;

    while (!(FDC.status & (FDC_RDREQ_MASK | FDC_RNF_MASK | FDC_CRC_MASK))) continue; // wait for sector found (RDREQ) or error (RNF or CRC)
    if (!(FDC.status & FDC_RDREQ_MASK)) { // sector not found (RNF or CRC error)
      //debug_out("unable to find track %d, sector %d, side %d\n", track, physical_sector, side);
      disk_error(ERR_SECTOR_NOT_FOUND);
    }

    cache_block = get_cache_ptr(current_disk, current_track, s);

    while (!(FDC.status & FDC_DRQ_MASK)) continue; // wait for DRQ set

    if (*cache_block == -1) {
      *NEAR_U8_PTR(0xd689) &= 0x7f; // see floppy buffer, not SD buffer
      for (uint16_t i = 0; i < 512; ++i) {
        while (FDC.status & FDC_EQ_MASK && FDC.status & FDC_BUSY_MASK) continue; // wait for DRQ set and EQ cleared
        buf[i] = FDC.data;
      }
    }
    else {
      wait_for_busy_clear();
    }

    last_disk            = current_disk;
    last_physical_track  = current_track;
    last_physical_sector = sector;
    last_side            = side;
  }

  // reset jiffy counter for drive access check
  jiffies_elapsed_since_last_drive_access = 0;
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
  drive_in_use = 1; // acquire drive and prevent motor from turning off
  if (drive_spinning) {
    return;
  }
  *NEAR_U8_PTR(0xd696) &= 0x7f; // disable auto-tune
  FDC.fdc_control |= FDC_MOTOR_MASK | FDC_LED_MASK; // enable LED and motor
  FDC.command = FDC_CMD_SPINUP;
  wait_for_busy_clear();
  drive_spinning = 1;
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
  * only after 60 jiffies have passed without the drive being accessed.
  * This prevents continuous spinning up and down of the drive when multiple
  * disk I/O operations are performed in quick succession.
  *
  * The main loop will regularly call diskio_check_motor_off() to turn off the
  * drive motor when it has not been accessed for 60 jiffies (eg. video frames).
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
static void disk_error(error_code_t error_code)
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
  drive_spinning      = 0;
  last_physical_track = 0xff;
  dir_cache_sectors   = 0;
}

static uint16_t allocate_sector(uint8_t start_track)
{
  uint8_t track     = start_track;
  int8_t  direction = track > 40 ? 1 : -1;
  struct bam_block *bam;
  struct bam_entry *bam_entry;

  if (track == 40) {
    // don't allocate file sectors on track 40
    --track;
  }

  do {
    uint8_t bam_entry_idx;
    if (track > 40) {
      bam           = (struct bam_block *)0x8100;
      bam_entry_idx = track - 41;
    }
    else {
      bam           = (struct bam_block *)0x8000;
      bam_entry_idx = track - 1;
    }
    bam_entry  = bam->bam_entries;
    //debug_out("bam_e1 %x", (uint16_t)bam_entry);
    bam_entry += bam_entry_idx;
    //debug_out("bei %d bam_e2 %x, num free %d", bam_entry_idx, (uint16_t)bam_entry, bam_entry->num_free_blocks);

    uint8_t sector = find_free_sector(bam_entry);
    if (sector != 0xff) {
      return make16(sector, track);
    }

    track += direction;
    if (track == 0) {
      track     = 41;
      direction = 1;
    }
    else if (track == 81) {
      track     = 39;
      direction = -1;
    }
  }
  while (track != start_track);

  // no free sector found
  return 0xff;
}

static uint8_t find_free_block_on_track(uint8_t track)
{
  __auto_type bam = (struct bam_block *)0x8000;
  if (track > 40) {
    ++bam;
    track -= 40;
  }
  __auto_type bam_entry = &(bam->bam_entries[track - 1]);
  if (bam_entry->num_free_blocks == 0) {
    // no free blocks on this track
    return 0xff;
  }

  uint8_t block = 0;
  uint8_t mask;
  for (uint8_t i = 0; i < 5; ++i) {
    mask = 1;
    for (uint8_t j = 0; j < 8; ++j) {
      if (bam_entry->block_usage[i] & mask) {
        bam_entry->block_usage[i] &= ~mask;
        bam_entry->num_free_blocks--;
        return block;
      }
      mask <<= 1;
      ++block;
    }
  }

  fatal_error(ERR_INCONSISTENT_BAM);
  return 0xff;
}

static uint8_t find_free_sector(struct bam_entry *entry)
{
  if (entry->num_free_blocks >= 2) {
    uint8_t sector = 0;
    uint8_t mask;
    for (uint8_t i = 0; i < 5; ++i) {
      mask = 3;
      for (uint8_t j = 0; j < 4; ++j) {
        if ((entry->block_usage[i] & mask) == mask) {
          entry->block_usage[i] &= ~mask;
          entry->num_free_blocks -= 2;
          return sector;
        }
        mask <<= 2;
        ++sector;
      }
    }
  }

  return 0xff;
}

static void free_blocks(uint8_t track, uint8_t block)
{
  while (track) {
    free_block(track, block);
    load_block(0xff, track, block);
    track = FDC.data;
    block = FDC.data;
  }
}

/**
  * @brief Frees a block on a track
  *
  * The function will free a block on a track by setting the corresponding bit in
  * the BAM to 1.
  *
  * @param track Logical track number (1-80)
  * @param block Logical block number (0-39)
  */
static void free_block(uint8_t track, uint8_t block)
{
  //debug_out("freeing block %d:%d", track, block);
  __auto_type bam = (struct bam_block *)0x8000;
  if (track > 40) {
    ++bam;
    track -= 40;
  }
  __auto_type bam_entry = &(bam->bam_entries[track - 1]);

  uint8_t block_idx = block / 8;
  uint8_t mask = 1 << (block % 8);
  bam_entry->block_usage[block_idx] |= mask;
}

/**
  * @brief Loads a sector from disk into a far memory location
  *
  * The function will load a sector from disk into a far memory location. The
  * sector is specified by its track and block number. The sector will be loaded
  * into the FDC buffer and then copied into the far memory location.
  * Note that there are always 2 blocks in a sector, so the sector containing the
  * specified block will be loaded, along with the other block in the sector.
  *
  * @param track Logical track number (1-80)
  * @param block Logical block number (0-39)
  */
static void load_sector_to_bank(uint8_t disk_num, uint8_t track, uint8_t block, uint8_t __far *target)
{
  load_block(disk_num, track, block);
  memcpy_far(target, FDC_BUF, 0x200);
}

static void write_block(uint8_t track, uint8_t block, uint8_t __far *block_data_far)
{
  //debug_out("writing track %d, block %d from buffer %lx", track, block, (uint32_t)block_data_far);
  __auto_type fdc_dst = FAR_U8_PTR(0xffd6c00);
  if (block & 1) {
    fdc_dst += 0x100;
    --block;
  }

  // makes sure we have the other block of the sector in the FDC buffer
  load_block(0xff, track, block);

  // overwrite loaded block with our data
  memcpy_far(fdc_dst, block_data_far, 0x100);

  // write the modified sector to disk
  write_sector_from_fdc_buf(track, block / 2);
}

static void write_sector(uint8_t track, uint8_t sector, uint8_t __far *sector_buf_far)
{
  //debug_out("writing track %d, sector %d from buffer %lx", track, sector, (uint32_t)sector_buf_far);
  __auto_type fdc_dst = FAR_U8_PTR(0xffd6c00);
  memcpy_far(fdc_dst, sector_buf_far, 0x200);
  write_sector_from_fdc_buf(track, sector);
}

static void write_sector_from_fdc_buf(uint8_t track, uint8_t sector)
{
  --track; // logical (1-80) to physical (0-79) track

  if (track > 79 || sector > 19) {
    disk_error(ERR_INVALID_DISK_LOCATION);
  }

  uint8_t side;
  ++sector; // logical (0-19) to physical (1-20) sector
  if (sector > 10) {
    // sectors 11-20 are on side 1
    sector -= 10;
    side = 1;
  }
  else {
    // sectors 1-10 are on side 0
    side = 0;
  }

  prepare_drive();

  if (side == 0) {
    FDC.fdc_control |= FDC_SIDE_MASK; // select side 0
  }
  else {
    FDC.fdc_control &= ~FDC_SIDE_MASK; // select side 1
  }

  step_to_track(track);

  //debug_out(" write t %d, s %d, side %d", track, sector, side);
  FDC.track  = track;
  FDC.sector = sector;
  FDC.side   = side;

  set_fdc_swap(0); // disable swap so CPU pointer will be reset to beginning of sector buffer

  FDC.command = FDC_CMD_WRITE_SECTOR;

  wait_for_busy_clear(); // sector writing from buffer completed when BUSY is cleared

  //if ((status & (FDC_RNF_MASK | FDC_CRC_MASK | FDC_DRQ_MASK | FDC_EQ_MASK)) != (FDC_DRQ_MASK | FDC_EQ_MASK)) {
  if (FDC.status & (FDC_RNF_MASK | FDC_CRC_MASK)) {
    disk_error(ERR_SECTOR_DATA_CORRUPT);
  }

  last_disk            = current_disk;
  last_physical_track  = track;
  last_physical_sector = sector;
  last_side            = side;

  // reset jiffy counter for drive access check
  jiffies_elapsed_since_last_drive_access = 0;
}

/** @} */ // end of diskio_private

//-----------------------------------------------------------------------------------------------
