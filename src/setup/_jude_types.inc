;=============================================================================
;jude
;=============================================================================
;
; Simple text-based GUI system and widget library.
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

  #pragma once

	CLR_BACK: .equ		0x00FD		;Background on C64
	CLR_EMPTY: .equ		0x00FE		;Border on C64
	CLR_CURSOR: .equ		0x00FF		
	CLR_TEXT: .equ		0x0000
	CLR_FOCUS: .equ		0x0001
	CLR_INSET: .equ		0x0002
	CLR_FACE: .equ		0x0003
	CLR_SHADOW: .equ		0x0004
	CLR_PAPER: .equ	0x0005
	CLR_MONEY: .equ		0x0006
	CLR_ITEM: .equ		0x0007
	CLR_INFORM: .equ		0x0008
	CLR_ACCEPT: .equ		0x0009
	CLR_APPLY: .equ		0x000A
	CLR_ABORT: .equ		0x000B

	CLR_INTN_THME:	.equ 0x0000		;Theme colour
	CLR_SYSS_TEXT:	.equ 0x1000		;Specific system text colour
	CLR_SYSS_CTRL: .equ	0x2000		;Specific system control colour 

	MPTR_NONE: .equ      0xFF
	MPTR_NORMAL: .equ		0x00
	MPTR_WAIT: .equ		  0x01

  VIEW__width: .equ 21
  VIEW__height: .equ 22
  VIEW__location: .equ 23
  VIEW__cellsize: .equ 27
  VIEW__layers_p: .equ 28 
  VIEW__layerscnt: .equ 32
  VIEW__bars_p: .equ 33
  VIEW__barscnt: .equ 37
  VIEW__actvpage_p: .equ 38
  VIEW__pages_p: .equ 42
  VIEW__pagescnt: .equ 46
  VIEW__linelen: .equ 47

  sizeof_ELEMENT: .equ 35
  ELEMENT__present: .equ 21
  ELEMENT__keypress: .equ 23
  ELEMENT__owner: .equ 25
  ELEMENT__colour: .equ 29
  ELEMENT__posx: .equ 31
  ELEMENT__posy: .equ 32
  ELEMENT__width: .equ 33
  ELEMENT__height: .equ 34

  sizeof_CONTROL: .equ 42
  CONTROL__text_p: .equ 35
  CONTROL__textoffx: .equ 39
  CONTROL__textaccel: .equ 40
  CONTROL__accelchar: .equ 41

	PANEL__controls_p: .equ 39
  PANEL__controlscnt: .equ 43

  UINTERFACE__mouseloc: .equ 38
  UINTERFACE__mptrloc: .equ 42
  UINTERFACE__mousepal: .equ 46
  UINTERFACE__views_p: .equ 47
  UINTERFACE__viewscnt: .equ 51

  LAYER__width: .equ 21

  PAGE__panels_p: .equ 48
  PAGE__panelscnt: .equ 52
