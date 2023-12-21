#include "diskio.h"
#include "util.h"
#include <mega65/memory.h>
#include <stdint.h>
#include <string.h>

static uint8_t track_list[54];
static uint8_t sector_list[54];

static void prepare_drive(void);
static void wait_for_busy_clear(void);

/**
 * @brief Loads a sector from the floppy disk into the floppy buffer.
 * 
 * @param track Logical track number (1-80)
 * @param block Logical block number (0-39)
 * @return uint8_t 0 if block is in first half of buffer, 1 if in second half
 */
static uint8_t load_block(uint8_t track, uint8_t block);
static void led_and_motor_off(void);


// public functions

void diskio_init(void)
{
  POKE(0xd680, 0x81); // map sector data to $de00
  clear_bits(0xd689, 0x80); // see floppy buffer, not SD buffer
  prepare_drive();
  memset(track_list, 0, sizeof(track_list));
  memset(sector_list, 0, sizeof(sector_list));

  for (uint8_t i = 0; i < 39; i++) {
    load_block(40, i);
  }
  //load_block(40, 3);
  
  led_and_motor_off();
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

static uint8_t load_block(uint8_t track, uint8_t block)
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

  return (block & 1);
}

void led_and_motor_off(void)
{
  POKE(0xd080, 0x00);
}
