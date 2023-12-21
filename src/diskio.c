#include "diskio.h"
#include "util.h"
#include <mega65/memory.h>
#include <string.h>

static uint8_t track_sector_list[54 * 2];

static void prepare_drive(void);
static void wait_for_busy_clear(void);

static void load_sector(uint8_t track, uint8_t sector);
static void led_and_motor_off(void);


// public functions

void diskio_init()
{
  POKE(0xd680, 0x81); // map sector data to $de00
  clear_bits(0xd689, 0x80); // see floppy buffer, not SD buffer
  prepare_drive();
  memset(track_sector_list, 0, sizeof(track_sector_list));

  load_sector(40, 3);

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

static void load_sector(uint8_t track, uint8_t sector)
{
  POKE(0xd084, track - 1); // physical track

}

void led_and_motor_off(void)
{
  POKE(0xd080, 0x00);
}
