/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
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

#pragma clang section data="cdata_diskio"

char disk_header[29] = { 0x28, 0x03, 0x44, 0x00, 
                         'M', 'A', 'N', 'I', 'A', 'C', ' ', 'M', 'A', 'N', 'S', 'I', 'O', 'N', 0xa0, 0xa0, 
                         0xa0, 0xa0, 
                         'M', 0x00,
                         0xa0, 0x33, 0x44, 0xa0, 0xa0 };
