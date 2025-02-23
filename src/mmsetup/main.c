//#pragma require __preserve_zp

#include "karljr.h"
#include "jude.h"
#include "mmsetup.h"
#include "mmsetup_core.h"

#include "hdos.h"


void updateCheckGlyph(void) {
  static const uint8_t glyphlo[] = {
    0b00000000,
    0b00000000,
    0b00111100,
    0b01111110,
    0b01111110,
    0b01111110,
    0b00011000,
    0b00000000
  };
  static const uint8_t glyphhi[] = {
    0b00000000,
    0b00011000,
    0b01111110,
    0b01111110,
    0b01111110,
    0b00111100,
    0b00000000,
    0b00000000
  };


  uint16_t offnrm = 0x7A * 8;
  uint16_t offrev = (0x7A + 128) * 8;

  uint8_t __attribute__((huge)) *outlo = (uint8_t __attribute__((huge)) *)0x0FF7E000;
  uint8_t __attribute__((huge)) *outhi = (uint8_t __attribute__((huge)) *)0x0FF7E800;

  for (uint8_t i = 0; i < 8; i++) {
    outlo[offnrm] = glyphlo[i];
    outlo[offrev] = glyphlo[i] ^ 0xff;
    outhi[offnrm] = glyphhi[i];
    outhi[offrev] = glyphhi[i] ^ 0xff;

    ++outlo;
    ++outhi;
  }
}

void attemptLoadFont(void) {
  if (!hdos_set_filename("COURIERM65.TCR") && !hdos_open_file()) {
    uint8_t __attribute__((huge)) *out = (uint8_t __attribute__((huge)) *)0x0FF7E000;
    uint8_t data;

    while (!hdos_read_byte(&data)) {
      *out = data;
      ++out;
    }

    hdos_close_file();

    updateCheckGlyph();

    *(uint8_t *)(0xd07a) = *(uint8_t *)(0xd07a) | 0x10;
  };
}


int main(void) {
  //hide our shinanegans....
  *(uint8_t *)0xd011 &= 0xEF;

  hdos_init(0x0800, 0x0800);
  _judeBackupKernalZP();

  attemptLoadFont();
  core_init();

  jude_initflags = INIT_PRESERVEKERNAL;
  karlInit();
  judeInit();

  karlModAttach((karlFarPtr_t)&mod_mmsetup_app);

  //does and sei but we should be fine by now
  judeInstallIdle((void *)&updateProcess);

  //Bring the screen back
  *(uint8_t *)0xd011 = *(uint8_t *)0xd011 | 0x10;

  judeViewInit((karlFarPtr_t)&vew_mmsetup_main);
  judeMain();

  return -1;
}
