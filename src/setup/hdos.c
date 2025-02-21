#include "hdos.h"
#include "_hdos.h"


void hdos_init(uint16_t dataBuf, uint16_t xferBuf) {
  ptrhdosBufHi = (dataBuf & 0xFF00) >> 8;
  ptrhdosXfrHi = (xferBuf & 0xFF00) >> 8;
  
/*__asm(
      "   lda #.byte1 dataBuf \n"
      "   sta ptrhdosBufHi \n"
      "   lda #.byte1 xferBuf \n"
      "   sta ptrhdosXfrHi \n"
      ::: "a"
  );*/
}

/*void __hdosSetFileName(void) {
  _hdosSetFileName();
}*/

//extern void _hdosSetFileName(void);

err_t hdos_set_filename(const char *fileName) {
  err_t result = 0;
  uint8_t lo = (uint16_t)fileName & 0xff;
  uint8_t hi = ((uint16_t)fileName & 0xff00) >> 8;

  __asm(
      " .extern _hdosSetFileName \n"
//    "   ldx #.byte0 %[fileName] \n"
//    "   ldy #.byte1 %[fileName] \n"
      "   jsr _hdosSetFileName \n"
      "   lda #0x00 \n"
      "   rol a\n"
//    "   sta %[result]"
      : "=Ka" (result)
      : "Kx" (lo), "Ky" (hi)
      : "a", "x", "y"
  );

  return result;
}

/*void __hdosOpenFile(void) {
  _hdosOpenFile();
}*/

err_t hdos_open_file(void) {
  err_t result = 0;

  __asm(
      " .extern _hdosOpenFile \n"
      "   jsr _hdosOpenFile \n"
      "   lda #0x00 \n"
      "   rol a \n"
//    "   sta result \n"
      : "=Ka" (result)
      :
      : "a"
  );

  return result;
}

/*void __hdosCloseFile(void) {
  _hdosCloseFile();
}*/

void hdos_close_file(void) {
  __asm(
    " .extern _hdosCloseFile \n"
    "   jsr _hdosCloseFile \n"
    ::: "a"
  );
}

/*void __hdosReadByte(void) {
  _hdosReadByte();
}*/

err_t hdos_read_byte(uint8_t *data) {
  err_t result = 0;
  uint8_t read = 0;

   __asm(
    " .extern _hdosReadByte \n"
    "   jsr _hdosReadByte \n"
    "   sta %1 \n"
    "   lda #0x00 \n"
    "   rol a \n"
//  "   sta result \n"
    : "=Ka" (result)
    : "Kzp8" (read)
    : "a"
  );

  if (!result) 
    *data = read;

  return result;
}

/*void __hdosCloseDir(void) {
  _hdosCloseDir();
}*/

void hdos_close_dir(uint8_t desc) {
  __asm(
    " .extern _hdosCloseDir \n"
//  "   lda desc"
    "   jsr _hdosCloseDir \n"
    : "=Ka" (desc)
    :
    : "a"
  );
}

/*void __hdosOpenDir(void) {
  _hdosOpenDir();
}*/

err_t hdos_open_dir(uint8_t *desc) {
  err_t result = 0;
  uint8_t read = 0;

  __asm(
    " .extern _hdosOpenDir \n"
    "   jsr _hdosOpenDir \n"
    "   sta %1 \n"
    "   lda #0x00 \n"
    "   rol a \n"
//  "   sta result \n"
    : "=Ka" (result)
    : "Kzp8" (read)
    : "a"
  );

  if (!result) 
    *desc = read;

  return result;
}

/*void __hdosReadDir(void) {
  _hdosReadDir();
}*/

err_t hdos_read_dir(uint8_t desc) {
  err_t result = 0;

  __asm(
//  "   lda desc \n"
    " .extern _hdosReadDir \n"
    "   jsr _hdosReadDir \n"
    "   lda #0x00 \n"
    "   rol a \n"
//  "   sta result \n"
    : "=Ka" (result)
    : "Ka" (desc)
    : "a"
  );

  return result;
}

/*void __hdosChangeDir(void) {
  _hdosChangeDir();
}*/

err_t hdos_change_dir(void) {
  err_t result = 0;

  __asm(
    " .extern _hdosChangeDir \n"
    "   jsr _hdosChangeDir \n"
    "   lda #0x00 \n"
    "   rol a \n"
//  "   sta result \n"
    : "=Ka" (result)
    :
    : "a"
  );

  return result;
}

err_t hdos_load_file_attic(uint32_t offset) {
  err_t result = 0;

  __asm(
    " .extern _hdosLoadFileAttic \n"
    "   ldx %[offs] \n"
    "   ldy %[offs] + 1 \n"
//  "   ldz %[offs] + 2 \n"
    "   lda %[offs] + 2 \n"
    "   taz \n"
    "   jsr _hdosLoadFileAttic \n"
    "   lda #0x00 \n"
    "   rol a \n"
//  "   sta result \n"
    : "=Ka" (result)
    : [offs] "Kzp32" (offset)
    : "a", "x", "y", "z"
  );

  return result;
}