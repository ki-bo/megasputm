//=============================================================================
//Ultimate SID Edition Room Builder
//=============================================================================
//
// Room file builder for Maniac Mansion Ultimate SID Edition
// 
// Copyright (c) 2025 Daniel England, Robert Steffens.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//=============================================================================


#include "room54.h"
#include <stdint.h>

#define NUM_ROOMS 55
#define NUM_SOUNDS 70
#define NUM_GLOBOBJS 256
#define NUM_COSTUMES 25
#define NUM_SCRIPTS 160

struct {
  uint8_t diskno[NUM_ROOMS];
  uint8_t roomno[NUM_ROOMS];
  uint32_t offset[NUM_ROOMS];
} rooms;

struct {
  uint8_t diskno[NUM_SOUNDS];
  uint8_t roomno[NUM_SOUNDS];
  uint32_t offset[NUM_SOUNDS];
} sounds;

uint16_t sectorOffsets[] = {
  0,
  0, 21, 42, 63, 84, 105, 126, 147, 168, 189, 210, 231, 252, 273, 294, 315, 336,
  357, 376, 395, 414, 433, 452, 471,
  490, 508, 526, 544, 562, 580,
  598, 615, 632, 649, 666
};

void readIndexFile(uint8_t __huge *image) {
  uint8_t __huge *f = image;

  //magic
  f += 2;     

  //object state
  f += NUM_GLOBOBJS;  

  //disk numbers
  for (uint8_t i = 0; i < NUM_ROOMS; ++i) {
    rooms.roomno[i] = i;
    rooms.diskno[i] = *f;
    f++;
  }

  //room offsets
  for (uint8_t i = 0; i < NUM_ROOMS; ++i) {
    uint8_t s = *f;
    f++;
    uint8_t t = *f;
    f++;

    uint32_t offs;

    if (i == 0) {
      offs = 0;
    } else {
      offs = (uint32_t)(sectorOffsets[t] + s) * 256;
    }

    rooms.offset[i] = offs;
  }

  //costume room numbers
  f += NUM_COSTUMES;

  //costume offsets
  f += NUM_COSTUMES * 2; 

  //script rooms
  f += NUM_SCRIPTS;

  //script offsets
  f += NUM_SCRIPTS * 2;

  //sound rooms
  for (uint8_t i = 0; i < NUM_SOUNDS; ++i) {
    sounds.roomno[i] = *f;
    f++;
  }

  //sound offsets
  for (uint8_t i = 0; i < NUM_SOUNDS; ++i) {
    uint16_t offs = *((uint16_t __huge *)f);
    f += 2;
    sounds.offset[i] = offs;
  }
}


uint8_t __huge *pushSound(uint8_t __huge *data, uint16_t *offs, uint8_t __huge **index, uint8_t __huge *dest) {
  uint16_t size;
  uint8_t __huge *q = dest;

  if (!data) {
    size = 0;
    *((uint16_t __huge *)(*index)) = 0;
  } else {
    size = *((uint16_t __huge *)data);
    *((uint16_t __huge *)(*index)) = *offs;
  }

  *index += 2;
  *offs += size;

  for (uint16_t i = 0; i < size; i++) {
    *q = data[i];
    q++;
  }

  return q;
}

uint16_t makeRoom54(uint8_t __huge *image1, uint8_t __huge *image2, uint8_t __huge *dest) {
  //space for size and index
  uint16_t offs = 4 + NUM_SOUNDS * 2;
  uint8_t __huge *m = dest + offs;
  uint8_t __huge *p = dest + 4;

  //The first 6 in mm are free
  for (uint8_t i = 0; i < 6; i++) {
    m = pushSound(0, &offs, &p, m);
  }

  for (uint8_t i = 6; i < NUM_SOUNDS; i++) {
    uint8_t __huge *data;
    
    uint8_t room = sounds.roomno[i];
    uint32_t resoffs = sounds.offset[i];

    uint32_t o;
    
    if  (resoffs < 0xffff) {
      o = resoffs + rooms.offset[room];

      if (rooms.diskno[room] == 0x32) {
        data = &image2[o];
      } else {
        data = &image1[o];
      }
    } else {
      data = 0;
      o = 0;
    }

    uint16_t size = *((uint16_t __huge *)data);

    m = pushSound(data, &offs, &p, m);
  }

  *(uint32_t __huge *)dest = (uint32_t)offs;

  for (uint16_t i = 0; i < offs; i++) {
    dest[i] = dest[i] ^ 0xff;
  }

  return offs;
}
