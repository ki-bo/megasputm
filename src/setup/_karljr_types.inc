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

  #pragma once

  ;#include <stdint.h>

  ;Debugging:  Show raster time usage on border
	;define	DEBUG_RASTERTIME

;	Debugging:  Don't yeild to IRQ again after suspend
  ;define	DEBUG_NOYIELDIRQ

;	Debugging:  Ensure bounds limits on stacks
	#define	DEBUG_RANGECHECK 1

;	Feature:  Depth of recursion supported by list processing
	FEATURE_CALLDEPTH: .equ 	8

  .extern _zkarljr

	zptrself: .equ  	_zkarljr + 0x04
	zptrowner: .equ 	_zkarljr + 0x08
	zptrscreen: .equ 	_zkarljr + 0x0C
	zptrcolour: .equ 	_zkarljr + 0x10
	zptrtemp0: .equ 	_zkarljr + 0x14
	zptrtemp1: .equ 	_zkarljr + 0x18
	zptrtemp2: .equ 	_zkarljr + 0x1C

	zreg0: .equ 	_zkarljr + 0x20
	zreg0wl: .equ 	_zkarljr + 0x20
	zreg0wh: .equ 	_zkarljr + 0x22
	zreg0b0: .equ 	_zkarljr + 0x20
	zreg0b1: .equ 	_zkarljr + 0x21
	zreg0b2: .equ 	_zkarljr + 0x22
	zreg0b3: .equ 	_zkarljr + 0x23
	zreg1: .equ 	_zkarljr + 0x24
	zreg1wl: .equ 	_zkarljr + 0x24
	zreg1wh: .equ 	_zkarljr + 0x26
	zreg1b0: .equ 	_zkarljr + 0x24
	zreg1b1: .equ 	_zkarljr + 0x25
	zreg1b2: .equ 	_zkarljr + 0x26
	zreg1b3: .equ 	_zkarljr + 0x27
	zreg2: .equ 	_zkarljr + 0x28
	zreg2wl: .equ 	_zkarljr + 0x28
	zreg2wh: .equ 	_zkarljr + 0x2A
	zreg2b0: .equ 	_zkarljr + 0x28
	zreg2b1: .equ 	_zkarljr + 0x29
	zreg2b2: .equ 	_zkarljr + 0x2A
	zreg2b3: .equ 	_zkarljr + 0x2B
	zreg3: .equ 	_zkarljr + 0x2C
	zreg3wl: .equ 	_zkarljr + 0x2C
	zreg3wh: .equ 	_zkarljr + 0x2E
	zreg3b0: .equ 	_zkarljr + 0x2C
	zreg3b1: .equ 	_zkarljr + 0x2D
	zreg3b2: .equ 	_zkarljr + 0x2E
	zreg3b3: .equ 	_zkarljr + 0x2F

	zreg4: .equ 	_zkarljr + 0x30
	zreg4wl: .equ 	_zkarljr + 0x30
	zreg4wh: .equ 	_zkarljr + 0x32
	zreg4b0: .equ 	_zkarljr + 0x30
	zreg4b1: .equ 	_zkarljr + 0x31
	zreg4b2: .equ 	_zkarljr + 0x32
	zreg4b3: .equ 	_zkarljr + 0x33
	zreg5: .equ 	_zkarljr + 0x34
	zreg5wl: .equ 	_zkarljr + 0x34
	zreg5wh: .equ 	_zkarljr + 0x36
	zreg5b0: .equ 	_zkarljr + 0x34
	zreg5b1: .equ 	_zkarljr + 0x35
	zreg5b2: .equ 	_zkarljr + 0x36
	zreg5b3: .equ 	_zkarljr + 0x37
	zreg6: .equ 	_zkarljr + 0x38
	zreg6wl: .equ 	_zkarljr + 0x38
	zreg6wh: .equ 	_zkarljr + 0x3A
	zreg6b0: .equ 	_zkarljr + 0x38
	zreg6b1: .equ 	_zkarljr + 0x39
	zreg6b2: .equ 	_zkarljr + 0x3A
	zreg6b3: .equ 	_zkarljr + 0x3B
	zreg7: .equ 	_zkarljr + 0x3C
	zreg7wl: .equ 	_zkarljr + 0x3C
	zreg7wh: .equ 	_zkarljr + 0x3E
	zreg7b0: .equ 	_zkarljr + 0x3C
	zreg7b1: .equ 	_zkarljr + 0x3D
	zreg7b2: .equ 	_zkarljr + 0x3E
	zreg7b3: .equ 	_zkarljr + 0x3F

	zreg8: .equ 	_zkarljr + 0x40
	zreg8wl: .equ 	_zkarljr + 0x40
	zreg8wh: .equ 	_zkarljr + 0x42
	zreg8b0: .equ 	_zkarljr + 0x40
	zreg8b1: .equ 	_zkarljr + 0x41
	zreg8b2: .equ 	_zkarljr + 0x42
	zreg8b3: .equ 	_zkarljr + 0x43
	zreg9: .equ 	_zkarljr + 0x44
	zreg9wl: .equ 	_zkarljr + 0x44
	zreg9wh: .equ 	_zkarljr + 0x46
	zreg9b0: .equ 	_zkarljr + 0x44
	zreg9b1: .equ 	_zkarljr + 0x45
	zreg9b2: .equ 	_zkarljr + 0x46
	zreg9b3: .equ 	_zkarljr + 0x47
	zregA: .equ 	_zkarljr + 0x48
	zregAwl: .equ 	_zkarljr + 0x48
	zregAwh: .equ 	_zkarljr + 0x4A
	zregAb0: .equ 	_zkarljr + 0x48
	zregAb1: .equ 	_zkarljr + 0x49
	zregAb2: .equ 	_zkarljr + 0x4A
	zregAb3: .equ 	_zkarljr + 0x4B
	zregB: .equ 	_zkarljr + 0x4C
	zregBwl: .equ 	_zkarljr + 0x4C
	zregBwh: .equ 	_zkarljr + 0x4E
	zregBb0: .equ 	_zkarljr + 0x4C
	zregBb1: .equ 	_zkarljr + 0x4D
	zregBb2: .equ 	_zkarljr + 0x4E
	zregBb3: .equ 	_zkarljr + 0x4F

	zregC: .equ 	_zkarljr + 0x50
	zregCwl: .equ 	_zkarljr + 0x50
	zregCwh: .equ 	_zkarljr + 0x52
	zregCb0: .equ 	_zkarljr + 0x50
	zregCb1: .equ 	_zkarljr + 0x51
	zregCb2: .equ 	_zkarljr + 0x52
	zregCb3: .equ 	_zkarljr + 0x53
	zregD: .equ 	_zkarljr + 0x54
	zregDwl: .equ 	_zkarljr + 0x54
	zregDwh: .equ 	_zkarljr + 0x56
	zregDb0: .equ 	_zkarljr + 0x54
	zregDb1: .equ 	_zkarljr + 0x55
	zregDb2: .equ 	_zkarljr + 0x56
	zregDb3: .equ 	_zkarljr + 0x57
	zregE: .equ 	_zkarljr + 0x58
	zregEwl: .equ 	_zkarljr + 0x58
	zregEwh: .equ 	_zkarljr + 0x5A
	zregEb0: .equ 	_zkarljr + 0x58
	zregEb1: .equ 	_zkarljr + 0x59
	zregEb2: .equ 	_zkarljr + 0x5A
	zregEb3: .equ 	_zkarljr + 0x5B
	zregF: .equ 	_zkarljr + 0x5C
	zregFwl: .equ 	_zkarljr + 0x5C
	zregFwh: .equ 	_zkarljr + 0x5E
	zregFb0: .equ 	_zkarljr + 0x5C
	zregFb1: .equ 	_zkarljr + 0x5D
	zregFb2: .equ 	_zkarljr + 0x5E
	zregFb3: .equ 	_zkarljr + 0x5F

	zvaltemp0: .equ _zkarljr + 0x60
	
  zvalkey: .equ _zkarljr + 0x64


	CPU_IRQ: .equ 	0xFFFE
	CPU_RESET: .equ 0xFFFC
	CPU_NMI: .equ 0xFFFA

	CIA1_IRQCTL: .equ 0xDC0D
	CIA2_IRQCTL: .equ 0xDD0D
	CIA1_DDRA: .equ 0xDC02
	CIA1_DDRB: .equ 0xDC03
	CIA1_PRB: .equ 0xDC01

	VIC_XPOS: .equ 0xD000
	VIC_YPOS: .equ 0xD001
	VIC_XPOSMSB: .equ 0xD010
	VIC_CTRLREG: .equ 	0xD011
	VIC_RSTRVAL: .equ 	0xD012
	VIC_IRQFLGS: .equ 	0xD019
	VIC_IRQMASK: .equ 	0xD01A
	VIC_BRDRCLR: .equ 	0xD020
	VIC_BKGDCLR: .equ 	0xD021

	VIC_CLRRAMH: .equ 	0x0FF80000

	SID_ADCONV1: .equ 	0xD419
	SID_ADCONV2: .equ 	0xD41A

	SPRITE_OFFX: .equ 	24
	SPRITE_OFFY: .equ 	50

	MOUSE_LBTN: .equ 	0x10
	MOUSE_RBTN: .equ 	0x01


	KEY_ASC_CR: .equ 	0x0D

	KEY_ASC_SPACE: .equ 	0x20
	KEY_ASC_EXMRK: .equ 	0x21
	KEY_ASC_DQUOTE: .equ 	0x22
	KEY_ASC_POUND: .equ 	0x23
	KEY_ASC_HASH: .equ 	0x23		;Alternate
	KEY_ASC_DOLLAR: .equ 	0x24
	KEY_ASC_PERCENT: .equ 	0x25
	KEY_ASC_AMP: .equ  	0x26
	KEY_ASC_QUOTE: .equ 	0x27
	KEY_ASC_OBRCKT: .equ  	0x28
	KEY_ASC_LBRCKT: .equ 	0x28		;Alternate
	KEY_ASC_CBRCKT: .equ 	0x29
	KEY_ASC_RBRCKT: .equ 	0x29		;Alternate
	KEY_ASC_MULT: .equ 	0x2A
	KEY_ASC_PLUS: .equ 	0x2B
	KEY_ASC_COMMA: .equ 	0x2C
	KEY_ASC_MINUS: .equ 	0x2D
	KEY_ASC_STOP: .equ 	0x2E
	KEY_ASC_DIV: .equ 	0x2F
	KEY_ASC_FSLASH: .equ 	0x2F		;Alternate
	KEY_ASC_0: .equ 	0x30
	KEY_ASC_1: .equ 	0x31
	KEY_ASC_2: .equ 	0x32
	KEY_ASC_3: .equ 	0x33
	KEY_ASC_4: .equ 	0x34
	KEY_ASC_5: .equ 	0x35
	KEY_ASC_6: .equ 	0x36
	KEY_ASC_7: .equ 	0x37
	KEY_ASC_8: .equ 	0x38
	KEY_ASC_9: .equ 	0x39
	KEY_ASC_COLON: .equ 	0x3A
	KEY_ASC_SCOLON: .equ 	0x3B
	KEY_ASC_LESSTH: .equ 	0x3C
	KEY_ASC_EQUALS: .equ 	0x3D
	KEY_ASC_GRTRTH: .equ 	0x3E
	KEY_ASC_QMARK: .equ 	0x3F
	KEY_ASC_AT: .equ 	0x40
	KEY_ASC_A: .equ 	0x41
	KEY_ASC_B: .equ 	0x42
	KEY_ASC_C: .equ 	0x43
	KEY_ASC_D: .equ 	0x44
	KEY_ASC_E: .equ 	0x45
	KEY_ASC_F: .equ 	0x46
	KEY_ASC_G: .equ 	0x47
	KEY_ASC_H: .equ 	0x48
	KEY_ASC_I: .equ 	0x49
	KEY_ASC_J: .equ 	0x4A
	KEY_ASC_K: .equ 	0x4B
	KEY_ASC_L: .equ 	0x4C
	KEY_ASC_M: .equ 	0x4D
	KEY_ASC_N: .equ 	0x4E
	KEY_ASC_O: .equ 	0x4F
	KEY_ASC_P: .equ 	0x50
	KEY_ASC_Q: .equ 	0x51
	KEY_ASC_R: .equ 	0x52
	KEY_ASC_S: .equ 	0x53
	KEY_ASC_T: .equ 0x54
	KEY_ASC_U: .equ 	0x55
	KEY_ASC_V: .equ 	0x56
	KEY_ASC_W: .equ 	0x57
	KEY_ASC_X: .equ 	0x58
	KEY_ASC_Y: .equ 	0x59
	KEY_ASC_Z: .equ 	0x5A
	KEY_ASC_OSQRBR: .equ 	0x5B
  ;Alternate
	KEY_ASC_LSQRBR: .equ 	0x5B		

	KEY_ASC_CSQRBR: .equ 	0x5D
  ;Alternate
	KEY_ASC_RSQRBR: .equ 	0x5D

	KEY_ASC_L_A: .equ 	0x61
	KEY_ASC_L_B: .equ 	0x62
	KEY_ASC_L_C: .equ 	0x63
	KEY_ASC_L_D: .equ 	0x64
	KEY_ASC_L_E: .equ 	0x65
	KEY_ASC_L_F: .equ 	0x66
	KEY_ASC_L_G: .equ 	0x67
	KEY_ASC_L_H: .equ 	0x68
	KEY_ASC_L_I: .equ 	0x69
	KEY_ASC_L_J: .equ 	0x6A
	KEY_ASC_L_K: .equ 	0x6B
	KEY_ASC_L_L: .equ 	0x6C
	KEY_ASC_L_M: .equ 	0x6D
	KEY_ASC_L_N: .equ 	0x6E
	KEY_ASC_L_O: .equ 	0x6F
	KEY_ASC_L_P: .equ 	0x70
	KEY_ASC_L_Q: .equ 	0x71
	KEY_ASC_L_R: .equ 	0x72
	KEY_ASC_L_S: .equ 	0x73
	KEY_ASC_L_T: .equ 	0x74
	KEY_ASC_L_U: .equ 	0x75
	KEY_ASC_L_V: .equ 	0x76
	KEY_ASC_L_W: .equ 	0x77
	KEY_ASC_L_X: .equ 	0x78
	KEY_ASC_L_Y: .equ 	0x79
	KEY_ASC_L_Z: .equ 	0x7A

;!!Needs screen code xlat
	KEY_ASC_BSLASH: .equ 	0x5C		
;!!Needs screen code xlat
	KEY_ASC_CARET: .equ 	0x5E		
;!!Needs screen code xlat
	KEY_ASC_USCORE: .equ 	0x5F
;!!Needs screen code xlat (Non c64)
	KEY_ASC_BQUOTE: .equ 	0x60
;!!Needs screen code xlat (non c64)
	KEY_ASC_OCRLYB: .equ 	0x7B		
;alternate
	KEY_ASC_LCRLYB: .equ 	0x7B
;!!Needs screen code xlat
	KEY_ASC_PIPE: .equ 	0x7C	
;!!Needs screen code xlat (non c64)
	KEY_ASC_CCRLYB: .equ 	0x7D	
;alternate
	KEY_ASC_RCRLYB: .equ 	0x7D		
;!!Needs screen code xlat
	KEY_ASC_TILDE: .equ 	0x7E		


	KEY_C64_STOP: .equ 	0x03
	KEY_C64_HOME: .equ 	0x13
	KEY_C64_POUND: .equ 	0xA3		
	KEY_C64_ARRUP: .equ 	0xAF		
	KEY_C64_ARRLEFT: .equ 	0x5F

	KEY_C64_CRIGHT: .equ 	0x1D
	KEY_C64_CDOWN: .equ 	0x11

	KEY_C64_DEL: .equ 		0x14

	KEY_M65_ESC: .equ 		0x1B
	KEY_M65_TAB: .equ 		0x09
  KEY_M65_SHTAB: .equ   0x0F

	KEY_M65_NONE: .equ 	0x00


  KEY_M65_F1: .equ 		0xF1
  KEY_M65_F2: .equ 		0xF2
	KEY_M65_F3: .equ 		0xF3
	KEY_M65_F4: .equ 		0xF4
	KEY_M65_F5: .equ 		0xF5
	KEY_M65_F6: .equ 		0xF6
	KEY_M65_F7: .equ 		0xF7
	KEY_M65_F8: .equ 		0xF8
	KEY_M65_F9: .equ 		0xF9
	KEY_M65_F10: .equ 		0xFA
	KEY_M65_F11: .equ 		0xFB
	KEY_M65_F12: .equ 		0xFC
	KEY_M65_F13: .equ 		0xFD
	KEY_M65_F14: .equ 		0xFE

	KEY_M65_HELP: .equ 	0x1F


	KEY_MOD_SHFTR: .equ 	0x01
	KEY_MOD_SHFTL: .equ 	0x02
	
	KEY_MOD_SHIFT: .equ 	0x03

	KEY_MOD_CTRL: .equ 	0x04
	KEY_MOD_SYS: .equ 		0x08
	KEY_MOD_ALT: .equ 		0x10
	KEY_MOD_NSCRL: .equ 	0x20
	KEY_MOD_CAPLK: .equ 	0x40


	STATE_CHANGED: .equ 	0x80
	STATE_DIRTY: .equ 		0x40
	STATE_PREPARED: .equ 	0x20
	STATE_VISIBLE: .equ 	0x01
	STATE_ENABLED: .equ 	0x02
	STATE_PICKED: .equ 	0x04
	STATE_ACTIVE: .equ 	0x08
	STATE_DOWN: .equ 		0x10
	STATE_EXPRESENT: .equ 	0x0100

	STATE_DEFAULT: .equ 	STATE_ENABLED
	STATE_DEFCTRL: .equ 	(STATE_VISIBLE | STATE_ENABLED)

	OPT_NOAUTOINVL: .equ 	0x01
	OPT_NONAVIGATE: .equ 	0x02
	OPT_NODOWNACTV: .equ 	0x04
	OPT_DOWNCAPTURE: .equ  0x10
	OPT_AUTOCHECK: .equ 	0x20
	OPT_TEXTACCEL2X: .equ 	0x40
	OPT_TEXTCONTMRK: .equ  0x80
	OPT_NOAUTOCHKOF: .equ  0x0100
	OPT_AUTOTRACK: .equ 	0x0200

	ERROR_ABORT: .equ 		0xFF
	ERROR_NONE: .equ 		0x00
	ERROR_MODULE: .equ 	0x10
	ERROR_UNIT: .equ 		0x20
	ERROR_OBJECT: .equ 	0x40
	ERROR_SYSTEM: .equ 	0x80

  sizeof_NAME: .equ  17

  sizeof__OBJECT: .equ  21

  sizeof__NAMEDOBJECT: .equ 38
  NAMEDOBJECT__name: .equ 21

  OBJECT__prepare: .equ  5
  OBJECT__initialise: .equ  7  
  OBJECT__change: .equ 9
  OBJECT__state: .equ  13
  OBJECT__oldstate: .equ  15
  OBJECT__options: .equ 17
  OBJECT__tag: .equ 19

  sizeof_MODULE: .equ 43
  MODULE__units_p: .equ  38
  MODULE__unitscnt: .equ  42

  sizeof_UNIT: .equ 38

  sizeof_CALLCONTEXT: .equ  3
  sizeof_PROCCONTEXT: .equ  21


MvDWZ   .macro    dst
    lda  #0x00
    sta  \dst
    sta  \dst + 1
    sta  \dst + 2
    sta  \dst + 3
       .endm

MvDWImmW  .macro dst, wsrc
    lda  #.byte0 \wsrc
    sta  \dst
    lda  #.byte1 \wsrc
    sta  \dst + 1
    lda  #0
    sta  \dst + 2
    sta  \dst + 3
       .endm

MvDWImm   .macro  dst, dwsrc
    lda  #.byte0 \dwsrc
    sta  \dst
    lda  #.byte1 \dwsrc
    sta  \dst + 1
    lda  #.byte2 \dwsrc
    sta  \dst + 2
    lda  #.byte3 \dwsrc
    sta  \dst + 3
       .endm

MvDWMem   .macro  dst, src
    phx
    phy

    ldq \src

    stq \dst

    ply
    plx
      .endm

MvDWObjMem  .macro dst, src, offset
    phx
    phy

    lda  \offset
    taz

    ldq [\src], z

    stq \dst

    ply
    plx
      .endm

MvDWObjImm .macro  dst, src, offset
    phx
    phy

    ldz  #\offset
    ldq [\src], z

    stq \dst

    ply
    plx
      .endm

ASLWMem  .macro src
    clc
    lda  \src
    asl  a
    sta  \src
    lda  \src + 1
    rol  a
    sta  \src + 1
      .endm

LSRWMem .macro  src
    clc
    lda  \src + 1
    lsr  a
    sta  \src + 1
    lda  \src
    ror  a
    sta  \src
      .endm

LDQmem .macro src
    ldq \src
      .endm

LDQIndWZ .macro  src
    ldq (\src), z
      .endm

LDQIndDWZ  .macro  dst
    ldq [\dst], z
      .endm

STQIndW  .macro  dst
    stq (\dst)
      .endm

STQMem  .macro  dst
    stq \dst
      .endm

ADQMem  .macro  src
    adcq  \src
      .endm

SBQMem .macro  src
    sbcq  \src
      .endm

LSQMem .macro src
    lsrq \src
      .endm

CPQMem  .macro  src
    cmpq \src
      .endm
