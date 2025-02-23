#include "room90.h"
#include <stdint.h>
//#include <stdio.h>


resource_t rooms;
resource_t sounds;


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
    uint8_t lo = *f;
    f++;
    uint8_t hi = *f;
    f++;
    
    uint16_t offs = (uint16_t)lo | (uint16_t)(hi << 8);

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

uint16_t makeRoom90(uint8_t __huge *image1, uint8_t __huge *image2, uint8_t __huge *dest) {
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
