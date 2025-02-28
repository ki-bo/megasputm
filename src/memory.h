/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * MEGASPUTM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#define RES_MAPPED          0x8000
#define HEAP                0x8000
#define BACKBUFFER_SCREEN   0xa000
#define BACKBUFFER_COLRAM   0xb800
#define SCREEN_RAM          0x10000UL
#define DISKIO_SECTION      0x12000UL
#define GFX_SECTION         0x14000UL
#define RESOURCE_BASE       0x18000UL
#define FLASHLIGHT_CHARS    0x28000
#define BG_BITMAP           0x28100
#define MUSIC_DATA          0x53800
#define COLRAM              0xff80800UL

#define HEAP_SIZE           0x2000
