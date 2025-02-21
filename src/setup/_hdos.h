#pragma once

#include <stdint.h>

extern uint8_t ptrhdosBufHi;
extern uint8_t ptrhdosXfrHi;
extern uint8_t flghdosErr;

void _hdosSetFileName(void);
void _hdosOpenFile(void);
void _hdosCloseFile(void);
void _hdosReadByte(void);
void _hdosCloseDir(void);
void _hdosOpenDir(void);
void _hdosReadDir(void);
void _hdosChangeDir(void);
void _hdosLoadFileAttic(void);