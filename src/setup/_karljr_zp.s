;=============================================================================
;KarlJr
;=============================================================================
;
; Simple object life-time management.
;
; Copyright (c) 2022, 2025 Daniel England.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;
;=============================================================================


		.rtmodel cpu, "*"

		.extern _Zp


;===========================================================
		.section zzpage, bss
		.public _zkarljr
		.public _zkarljr2
;===========================================================
_zkarljr:
_zkarljr2:
		.space		0x68
