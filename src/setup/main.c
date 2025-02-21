#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "hdos.h"
#include "kernal.h"

#include "adf.h"

#define ADF_MEMORY ((uint8_t __huge *)0x08000000)
#define FILE_MEMORY ((uint8_t __huge *)0x40000)

void outputFile(uint8_t roomNo) 
{
  char fileName[7];
  char fileNameDOS[13];

  sprintf(fileName, "%2.2d.LFL", roomNo);
  sprintf(fileNameDOS, "@:%2.2d.LFL,S,W", roomNo);

  printf("%s\r", fileName);

  kernal_close_all(8);
  kernal_set_banks(0, 0);
  kernal_set_logical_file(1, 8, 2);
  kernal_set_name(fileNameDOS, sizeof(fileNameDOS) - 1);

  *(volatile uint8_t *)(0xd020) = 13;

  if (kernal_open()) {
    *(volatile uint8_t *)(0xd020) = 2;

    printf("OPEN ERROR!\r");
    return;
  }

  if (kernal_set_logical_output(1)) {
    *(volatile uint8_t *)(0xd020) = 2;

    printf("OUTPUT ERROR!\r");
    return;
  }

  uint32_t file_size;
  if (adf_read_file(fileName, FILE_MEMORY, &file_size) != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    kernal_close_all(8);
    kernal_reset_channels();

    printf("FAILED TO READ FILE %s\r", fileName);
    return;
  }

  __auto_type data = FILE_MEMORY;
  while(file_size) {
    if (kernal_write_byte(65)) {
      *(volatile uint8_t *)(0xd020) = 2;
      kernal_close_all(8);
      kernal_reset_channels();

      printf("WRITE ERROR!\r");
      __asm(" bra .");
    }

    *(volatile uint8_t *)(0xd020) = *data & 0x0f;
    ++data;
    --file_size;
  }

  kernal_close_logical_file(1);
  kernal_close_all(8);
  kernal_reset_channels();
} 

uint8_t disk1Rooms[] = 
    {0, 30, 33, 40, 44, 45, 49, 50,
    51, 53, 0xff};
uint8_t disk2Rooms[] =
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
    31, 32, 34, 35, 36, 37, 38, 39, 41, 
    42, 43, 44, 46, 47, 48, 52, 53, 0xff};


void process(char *adfFileName, uint8_t *rooms) {
  //apparently, it goes a little something like this...
  struct Device *dev;
  struct Volume *vol;
  struct List *list, *cell;

  hdos_set_filename(adfFileName);
  hdos_load_file_attic(0);

  if (adf_init(ADF_MEMORY) != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    printf("INVALID ADF FILE.");
    return;
  }

  if (adf_chdir("rooms") != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    printf("DIRECTORY ROOMS NOT FOUND.");
    return;
  }

  uint8_t i = 0;
  uint8_t roomNo = rooms[i++];
  while (roomNo < 0xff) {
    outputFile(roomNo);
    roomNo = rooms[i++];
  };
}


__task void main(void) {
  hdos_init(0x0800, 0x0800);

  while (*(uint8_t volatile *)(0xd610) != 0) {
    *(uint8_t volatile *)(0xd610) = 0;
  }

  printf("INSERT DESTINATION DISK #1\r");
  printf("PRESS ANY KEY.\r");
  while (*(uint8_t volatile *)(0xd610) == 0);
  *(uint8_t volatile *)(0xd610) = 0;

  process("MANIACM1.ADF", disk1Rooms);

  printf("INSERT DESTINATION DISK #2\r");
  printf("PRESS ANY KEY.\r");
  while (*(uint8_t *)(0xd610) == 0);
  *(uint8_t *)(0xd610) = 0;

  process("MANIACM2.ADF", disk2Rooms);

  while (1) {
    *(volatile uint8_t *)(0xd020) = *(volatile uint8_t *)(0xd020);
  } 
}