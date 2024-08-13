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

		.rtmodel cstartup, "mm"

		.rtmodel version, "1"
		.rtmodel cpu, "*"

		.section data_init_table
		.section stack
		.section cstack
		.section heap
		.section runtime_copy
		.section code
		.section init_copy
		.section code_init
		.section diskio_copy
		.section code_diskio

		.extern main
		.extern _Zp, _Vsp

;;; *** Need to manually provide start address as we use raw binary output format and not prg.
		.section autoboot_load_address
		.word   0x2001

		.pubweak __program_root_section, __program_start

		.section programStart, root
__program_root_section:
		.word nextLine
		.word 10               ; line number
		.byte 0x9e, " 8206", 0 ; SYS $200e
nextLine:	.word 0                ; end of program

		.section startup, root, noreorder
__program_start:
		sei
		; fade out screen
		ldy #60
loop1$:		ldx #0
		ldz #1
loop2$:		lda 0xd100,x
		beq green$
		dec 0xd100,x
		ldz #0
green$:		lda 0xd200,x
		beq blue$
		dec 0xd200,x
		ldz #0
blue$:		lda 0xd300,x
		beq nextidx$
		dec 0xd300,x
		ldz #0
nextidx$:
		inx
		bne loop2$
exit$:		tza
		bne done$
		lda 0xd7fa
frameloop$:	cmp 0xd7fa
		beq frameloop$
		dey
		bne loop1$
done$:		ldy #30
frameloop2$:	lda 0xd7fa
frameloop3$:	cmp 0xd7fa
		beq frameloop3$
		dey
		bne frameloop2$

;;; **** Relocate runtime.
		lda #1
		trb 0xd703					; disable F018B mode
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

;;; **** Relocate diskio.
		sta 0xd707
		.byte 0						; end of job options
		.byte 0						; copy command, chained
		.word (.sectionSize diskio_copy)		; count
		.word (.sectionStart diskio_copy)		; source
		.byte 0						; source bank
		.word 0x2000					; destination
		.byte 1						; destination bank
		.byte 0						; cmd high
		.byte 0						; modulo / ignored

		lda #65						; force 40 MHz
		sta 0
		lda #0x35					; all RAM + I/O (C64 style banking)
		sta 1
		lda #0x47					; MEGA65 I/O personality
		sta 0xd02f
		lda #0x53
		sta 0xd02f
		
		lda #4						; disable C64 ROM banking
		sta 0xd030					; (C65/VIC-III style banking)
		lda #2						; write enable banks 2 and 3
		sta 0xd641
		clv
		ldx #.byte0(.sectionEnd stack)			; set stack high/low
		txs
		ldy #.byte1(.sectionEnd stack)
		tys
		see						; set stack extended disable flag
		lda #.byte0(.sectionEnd cstack)
		sta zp:_Vsp
		lda #.byte1(.sectionEnd cstack)
		sta zp:_Vsp+1
		lda #0
		tax
		tay
		taz
		map
		eom

;;; Initialize data sections.
		.section startup, noroot, noreorder
		.pubweak __data_initialization_needed
__data_initialization_needed:
		lda #.byte0 (.sectionStart data_init_table)
		sta zp:_Zp
		lda #.byte1 (.sectionStart data_init_table)
		sta zp:_Zp+1
		lda #.byte0 (.sectionEnd data_init_table)
		sta zp:_Zp+2
		lda #.byte1 (.sectionEnd data_init_table)
		sta zp:_Zp+3
		.extern __initialize_sections
		jsr __initialize_sections

;;; **** Initialize heap.
		.section startup, noroot, noreorder
		.pubweak __call_heap_initialize
__call_heap_initialize:
		lda #.byte0 __default_heap
		sta zp:_Zp+0
		lda #.byte1 __default_heap
		sta zp:_Zp+1
		lda #.byte0 (.sectionStart heap)
		sta zp:_Zp+2
		lda #.byte1 (.sectionStart heap)
		sta zp:_Zp+3
		lda #.byte0 (.sectionSize heap)
		sta zp:_Zp+4
		lda #.byte1 (.sectionSize heap)
		sta zp:_Zp+5
		.extern __heap_initialize, __default_heap
		jsr __heap_initialize

		lda #.byte0(nmi_handler)
		sta 0xfffa
		lda #.byte1(nmi_handler)
		sta 0xfffb

;;; Jump into C main function.
		jmp main

;;; Disable NMI
		.section code
nmi_handler:
		rti
