/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2025 Robert Steffens
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

#include "adf.h"
#include <ctype.h>
//#include <stdio.h>
#include <string.h>

#define ROOT_BLOCK (40 * 2 * 11)
#define HASH_TABLE_SIZE 72
#define MAX_NAME_LEN 30
#define BYTES_PER_BLOCK 488

#define ENTRY_NAME_LEN *(sector_buffer + 0x1b0)
#define ENTRY_NAME ((char *)(sector_buffer + 0x1b1))

uint8_t sector_buffer[512];
char name_buffer[MAX_NAME_LEN + 1];
uint8_t __huge *image_ptr;
uint16_t current_dir_block;

// private functions
static void read_block(uint16_t block_num);
static int16_t find_entry_block(const char *name);
static uint32_t read_u32(uint16_t offset);

int8_t adf_init(uint8_t __huge *image)
{
  image_ptr = image;
  current_dir_block = ROOT_BLOCK;

  // read root block
  read_block(ROOT_BLOCK);

  uint32_t primary_block_type   = read_u32(0);
  uint32_t secondary_block_type = read_u32(0x1fc);
  
  // root block:
  // 0x00000002 = primary block
  // 0x00000001 = secondary block
  if (primary_block_type != 0x00000002 || secondary_block_type != 0x00000001)
  {
    // no root block (primary block type 2)
    return ADF_ERR_NOROOT;
  }

  // determine DOS version from boot block (0)
  read_block(0);
  if (memcmp(sector_buffer, "DOS", 3) != 0)
  {
    return ADF_ERR_NOTDOS;
  }

  if (sector_buffer[3] != 0x00)
  {
    // not OFS
    return ADF_ERR_FSUNSUPPORTED;
  }

  return ADF_OK;
}

void adf_chroot(void)
{
  current_dir_block = ROOT_BLOCK;
}

int8_t adf_chdir(const char *dir)
{
  // read current dir block
  read_block(current_dir_block);

  int16_t block_num = find_entry_block(dir);
  if (block_num < 0)
  {
    return ADF_ERR_NOTFOUND;
  }

  current_dir_block = block_num;
  return ADF_OK;
}

int8_t adf_read_file(const char *filename, uint8_t __huge *dest, uint32_t *size)
{
  read_block(current_dir_block);
  if (find_entry_block(filename) < 0) {
    return ADF_ERR_NOTFOUND;
  }

  // file header block is currently loaded in sector buffer
  uint32_t bytes_left = read_u32(0x144);
  if (size) {
    *size = bytes_left;
  }
  //printf("BYTES LEFT: %ld\r", bytes_left);

  while (bytes_left)
  {
    // get next data block
    read_block(read_u32(0x10));

    uint16_t bytes_in_block = bytes_left > BYTES_PER_BLOCK ? BYTES_PER_BLOCK : bytes_left;
    for (uint16_t i = 0; i < bytes_in_block; ++i)
    {
      *dest = (sector_buffer + 24)[i];
      ++dest;
    }
    bytes_left -= bytes_in_block;
  }

  return ADF_OK;
}

void read_block(uint16_t block_num)
{
  uint32_t bn = block_num;
  uint32_t offs = bn * 512;

  // read block from disk
  uint8_t __huge *src  = image_ptr + offs;
  uint8_t __huge *dest = (uint8_t __huge *)sector_buffer;

  for (uint16_t i = 0; i < 512; i++)
  {
    *dest = *src;
    ++dest;
    ++src;
  }
}

int16_t find_entry_block(const char *name)
{
  uint8_t  len  = strlen(name);
  uint16_t hash = len;

  if (len > MAX_NAME_LEN)
  {
    return -1;
  }

  // calculate hash value from file name string
  for (uint8_t i = 0; i < len; ++i)
  {
    name_buffer[i] = toupper(name[i]);
    hash = (hash * 13 + (uint8_t)name_buffer[i]) & 0x7ff;
  }
  uint8_t hash_table_entry = hash % HASH_TABLE_SIZE;

  //printf("FILENAME: %s, HASH: %d\r", name_buffer, hash_table_entry);

  // find block number in hash table
  uint16_t block_num  = read_u32(0x18 + hash_table_entry * 4);

  if (block_num == 0)
  {
    // no file matching that hash
    //printf("NO FILE FOUND\r");
    return -1;
  }

  // iterate over blocks and match real strings
  do {
    read_block(block_num);
    //printf("BLOCK_NUM: %d, LEN: %d\r", block_num, (uint16_t)ENTRY_NAME_LEN);
    if (ENTRY_NAME_LEN == len) {
      uint8_t i;
      for (i = 0; i < len; ++i) {
        if (toupper(ENTRY_NAME[i]) != name_buffer[i]) {
          break;
        }
      }
      if (i == len) {
        // found entry block
        //printf("FOUND: %d\r", block_num);
        return block_num;
      }
    }

    // file name in block does not match, check next block
    block_num = read_u32(0x1f0);
  }
  while (block_num != 0);

  //printf("NOTHING FOUND\r");
  return -1;
}

uint32_t read_u32(uint16_t offset)
{
  uint32_t value_be = *((uint32_t *)(sector_buffer + offset));
  uint32_t value_le;

  __asm(" stz %0\n"
        " sty %0 + 1\n"
        " stx %0 + 2\n"
        " sta %0 + 3\n"
        : "=Kzp32"(value_le)
        : "Kq"(value_be)
        :
  );
  return value_le;
}
