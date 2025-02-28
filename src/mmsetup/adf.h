//=============================================================================
//Amiga Disk Image File interaface unit
//=============================================================================
//
// Interface module for Amiga Disk Image files for MMSetup.
// 
// Copyright (c) 2025 Robert Steffens, Daniel England.
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

#define ADF_OK                       0
#define ADF_ERR_NOROOT              -1
#define ADF_ERR_NOTDOS              -2
#define ADF_ERR_FSUNSUPPORTED       -3
#define ADF_ERR_NOTFOUND            -4

int8_t adf_init(uint8_t __huge *image);
void adf_chroot(void);
int8_t adf_chdir(const char *dir);
int8_t adf_read_file(const char *filename, uint8_t __huge *dest, uint32_t *size);
