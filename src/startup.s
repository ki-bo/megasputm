		.rtmodel cstartup, "mm"

		.rtmodel version, "1"
		.rtmodel cpu, "*"

		.section data_init_table
		.section stack
		.section cstack
		.section heap

		.extern _Zp, _Vsp

		.section loadaddress
_LoadAddress:	.word   0x2001

		.pubweak __program_root_section, __program_start

		.section programStart, root
__program_root_section:
		.word nextLine
		.word 10            ; line number
		.byte 0x9e, " 8206", 0 ; SYS $200e
nextLine:	.word 0             ; end of program

		.section startup, root, noreorder
__program_start:
		sei
		lda #65						; set 40 MHz
		sta 0
		lda #0x35					; all RAM + I/O (C64 style banking)
		sta 1
		lda #0x47					; MEGA65 I/O personality
		sta 0xd02f
		lda #0x53
		sta 0xd02f
		lda #0						; disable C64 ROM banking
		sta 0xd030					; (C65/VIC-III style banking)
		ldx #.byte0(.sectionEnd stack)			; set stack hich/low
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
		jsr __low_level_init

;;; Initialize data sections if needed.
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

		.section startup, noroot, noreorder
		.pubweak __call_initialize_global_streams
__call_initialize_global_streams:
		.extern __initialize_global_streams
		jsr __initialize_global_streams

;;; **** Initialize heap if needed.
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

		.section startup, root, noreorder
		.extern main
		jmp main

;;; ***************************************************************************
;;;
;;; __low_level_init - custom low level initialization
;;;
;;; This default routine just returns doing nothing. You can provide your own
;;; routine, either in C or assembly for doing custom low leve initialization.
;;;
;;; ***************************************************************************

		.section initcode
		.pubweak __low_level_init
__low_level_init:
		rts

		.section registers, noinit
		.public _MapShadow	
_MapShadow:   	.space  2
