#pragma once

#include <stdint.h>


#define NUM_ROOMS 55
#define NUM_SOUNDS 70
#define NUM_GLOBOBJS 256
#define NUM_COSTUMES 25
#define NUM_SCRIPTS 160


typedef struct RESOURCE {
  uint8_t diskno[NUM_ROOMS];
  uint8_t roomno[NUM_ROOMS];
  uint32_t offset[NUM_ROOMS];
} resource_t;

extern resource_t rooms;
extern resource_t sounds;


void readIndexFile(uint8_t __huge *image);
uint16_t makeRoom90(uint8_t __huge *image1, uint8_t __huge *image2, uint8_t __huge *dest);
