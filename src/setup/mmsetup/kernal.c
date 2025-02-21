#include <stdint.h>


uint8_t kernal_error = 0;


uint8_t kernal_get_last_error(void) {
  return kernal_error;
}

void kernal_close_all(uint8_t device) {
  //CLOSEALL
  __asm volatile (
  //"   lda #0x08 \n"
    "   jsr 0xff50 \n"
    :
    : "Ka" (device)
    : "a"
  );

  kernal_error = 0;
}

uint8_t kernal_set_banks(uint8_t memory, uint8_t fileName) {
  uint8_t result = 1;

  uint8_t mem = memory & 0x7f;
  uint8_t file = fileName & 0x7f;

  //SETBNK
  __asm(
  //"   lda #0x00 \n"
  //"   ldx #0x00 \n"
    "   jsr 0xff6b \n"
    "   sta kernal_error \n"
    "   lda #0x00 \n"
    "   rol a \n"
    : "=Ka" (result)
    : "Ka" (mem), "Kx" (file)
    :"a", "x"
  );

  return result;
}

uint8_t kernal_set_banks_long(uint32_t memory, uint32_t fileName) {
//  unimplemented
  kernal_error = 0xff;
  return 1;
}

void kernal_set_logical_file(uint8_t logical, uint8_t device, uint8_t secondary) {
  //SETLFS  1, 8, 2
  __asm(
    //" lda #0x01 \n"
    //" ldx #0x08 \n"
    //" ldy #0x02 \n"
    "   jsr 0xffba \n"
    :
    : "Ka" (logical), "Kx" (device), "Ky" (secondary)
    : "a", "x", "y"
  );

  kernal_error = 0;
}

void kernal_set_name(char * fileName, uint8_t len) {
  //SETNAM filename
  //uint8_t fileLo = (uint16_t)fileNameDOS & 0xff;
  //uint8_t fileHi = ((uint16_t)fileNameDOS & 0xff00) >> 8;

  __asm(
    //" lda #0x0c \n"
    "   ldx %[filename] \n"
    "   ldy %[filename] + 1 \n"
    "   jsr 0xffbd \n"
    : 
    : "Ka" (len), [filename] "Kzp16" (fileName)
    : "a", "x", "y"
  );
}

uint8_t kernal_open(void) {
  uint8_t result = 1;

  //OPEN
  __asm(
    "   jsr 0xffc0 \n"
    "   sta kernal_error \n"
    "   lda #0 \n"
    "   rol a \n"
    : "=Ka" (result)
    :
    : "a"
  );

  return result;
}

uint8_t kernal_set_logical_output(uint8_t logical) {
  uint8_t result;

  //CKOUT
  __asm(
    //" ldx #0x01 \n"
    "   jsr 0xffc9 \n"
    "   sta kernal_error \n"
    "   lda #0 \n"
    "   rol a \n"
    : "=Ka" (result)
    : "Kx" (logical)
    : "a", "x"
  );

  return result;
}

uint8_t kernal_write_byte(uint8_t data) {
  uint8_t result;

  //BSOUT
  __asm volatile (
    //" lda %[data] \n"
    "   jsr 0xffd2 \n"
    "   sta kernal_error \n"
    "   lda #0x00 \n"
    "   rol a \n"
    : "=Ka" (result)
    : "Ka" (data)
    : "a"
  );

  return result;
}

uint8_t kernal_close_logical_file(uint8_t logical) {
  uint8_t result = 1;

  //CLOSE
  __asm(
    "   clc \n"
    //" lda #0x01 \n"
    "   jsr 0xffc3 \n"
    "   sta kernal_error \n"
    "   lda 0x00 \n"
    "   rol a \n"
    : "=Ka"(result)
    : "Ka" (logical)
    : "a"
  );

  return result;
}

void kernal_reset_channels(void) {
  //CLRCH
  __asm(
    "   jsr 0xffcc \n"
    ::: "a"
  );
}