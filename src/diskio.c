#include "diskio.h"
#include "util.h"
#include <mega65/debug.h>
#include <mega65/memory.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static uint8_t track_list[54];
static uint8_t sector_list[54];

static void prepare_drive(void);
static void wait_for_busy_clear(void);

/**
 * @brief Loads a sector from the floppy disk into the floppy buffer.
 * 
 * As sectors in 1581 format are 512 bytes, they always contain two logical
 * blocks. This function loads the sector containing the specified block into
 * the floppy buffer. The function provides a pointer to the floppy buffer
  * position containing the block.
 *
 * @param track Logical track number (1-80)
 * @param block Logical block number (0-39)
 * @return uint8_t * Pointer to the floppy buffer position containing the block
 */
static uint8_t *load_block(uint8_t track, uint8_t block);
static void led_and_motor_off(void);
/**
 * @brief Reads the directory block in memory at buffer
 * 
 * The directory block contains up to eight file entries. Each file entry is 32
 * bytes long. If there are more blocks to read in the directory, the function will
 * automatically load the next block and return a pointer to it. If there are no more
 * blocks to read, the function will return NULL.
 *
 * @param buffer Pointer to the floppy buffer position containing the last block loaded
 * @return uint8_t* Pointer to the next floppy buffer position
 */
static uint8_t *read_next_directory_block(uint8_t *buffer);
static void read_file_entry(uint8_t *read_ptr);


// public functions

void diskio_init(void)
{
  POKE(0xd680, 0x81); // map FDC buffer to $de00
  clear_bits(0xd689, 0x80); // see floppy buffer, not SD buffer
  prepare_drive();
  memset(track_list, 0, sizeof(track_list));
  memset(sector_list, 0, sizeof(sector_list));

  // Loading file list in the directory, starting at track 40, block 3
  uint8_t *buffer_ptr = load_block(40, 3);
  while (buffer_ptr) {
    buffer_ptr = read_next_directory_block(buffer_ptr);
  }
  
  led_and_motor_off();

  POKE(0xd680, 0x81); // unmap FDC buffer from $de00
}


// private functions

void prepare_drive(void)
{
  POKE(0xd080, 0x60); // enable LED and motor
  POKE(0xd081, 0x20); // spinup command
  wait_for_busy_clear();
}

void wait_for_busy_clear(void)
{
  while (PEEK(0xd082) & 0x80);
}

static uint8_t *load_block(uint8_t track, uint8_t block)
{
  uint8_t physical_sector;
  uint8_t side;

  POKE(0xd084, track - 1); // physical track
  if (block < 20) {
    physical_sector = block / 2 + 1;
    side = 0;
  }
  else {
    physical_sector = (block - 20) / 2 + 1;
    side = 1;
  }
  
  POKE(0xd085, physical_sector);
  POKE(0xd086, side);

  POKE(0xd081, 0x40); // read sector command (buffered mode)
  wait_for_busy_clear();

  // RNF or CRC error flag check
  if (PEEK(0xd082) & 0x18) {
    // error
    led_and_motor_off();
    fatal_error("Error reading sector");
  }

  // all even blocks are in first half of buffer, all odd blocks in second half
  // we can therefore return a pointer to the correct half of the buffer
  return (block & 1) ? (uint8_t *)0xdf00 : (uint8_t *)0xde00;
}

void led_and_motor_off(void)
{
  POKE(0xd080, 0x00);
}

uint8_t *read_next_directory_block(uint8_t *buffer) 
{
  uint8_t next_track = buffer[0];
  uint8_t next_block = buffer[1];
  for (uint8_t i = 0; i < 8; ++i) {
    read_file_entry(buffer);
    buffer += 32;
  }
  if (next_track == 0) {
    return NULL;
  }

  return load_block(next_track, next_block);
}

void read_file_entry(uint8_t *read_ptr)
{
  uint8_t i = 2;
  if (read_ptr[i] != 0x82) {
    // not a PRG file
    return;   
  }
  ++i;
  uint8_t file_track = read_ptr[i];
  if (file_track == 0 || file_track > 80) {
    // invalid track number
    return;
  }
  ++i;
  uint8_t file_block = read_ptr[i];
  if (file_block >= 40) {
    // invalid block number
    return;
  }
  ++i;
  if (read_ptr[i] < 0x30 || read_ptr[i] > 0x39) {
    // invalid room number
    return;
  }
  uint8_t room_number = (read_ptr[i] - 0x30) * 10;
  ++i;
  if (read_ptr[i] < 0x30 || read_ptr[i] > 0x39) {
    // invalid room number
    return;
  }
  room_number += read_ptr[i] - 0x30;
  ++i;
  const char *file_suffix = ".LFL\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0";
  for (uint8_t j = 0; j < 14; ++j) {
    if (read_ptr[i] != file_suffix[j]) {
      // invalid file suffix
      return;
    }
    ++i;
  }

  // all checks passed, we found a valid xx.lfl file with xx being the room number
  track_list[room_number] = file_track;
  sector_list[room_number] = file_block;
}
