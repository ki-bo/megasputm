//=============================================================================
//HYPPO DOS Interface unit
//=============================================================================
//
// Interface module for the HYPPO DOS functions.
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
void _hdos_closeall();