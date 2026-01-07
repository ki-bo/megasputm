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

		.rtmodel cpu, "*"

		.section runtime_copy
		.section code

		.section code_init
		.public relocate_runtime
relocate_runtime:
		; execute inline DMA job to copy the runtime section
		sta 0xd707
		.byte 0						; end of job options
		.byte 0						; copy command
		.word (.sectionSize runtime_copy)		; count
		.word (.sectionStart runtime_copy)		; source
		.byte 0						; source bank
		.word (.sectionStart code)			; destination
		.byte 0						; destination bank
		.byte 0						; cmd high
		.byte 0						; modulo / ignored
		rts
