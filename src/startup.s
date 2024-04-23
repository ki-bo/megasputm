		.rtmodel cstartup, "mm"

		.rtmodel version, "1"
		.rtmodel cpu, "*"

		.section data_init_table
		.section stack
		.section cstack
		.section heap
		.section init_copy

		.extern _Zp, _Vsp

		.section autoboot_load_address
		.word   0x2001

		.pubweak __program_root_section, __program_start

		.section programStart, root
__program_root_section:
		.word nextLine
		.word 10            ; line number
		.byte 0x9e, " 8206", 0 ; SYS $200e
nextLine:	.word 0             ; end of program

		.section startup, root, noreorder
__program_start:
		jsr load_runtime_and_diskio
		lda #65						; set 40 MHz
		sta 0
		lda #0x35					; all RAM + I/O (C64 style banking)
		sta 1
		lda #0x47					; MEGA65 I/O personality
		sta 0xd02f
		lda #0x53
		sta 0xd02f
		
		lda #1          				; select real drive
    		;trb 0xd68b
    		;tsb 0xd6a1

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
		;jsr __heap_initialize

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

		.section startup, root, noreorder
		.pubweak __low_level_init
__low_level_init:
		rts

;;; ***************************************************************************
;;;
;;; load_runtime - load runtime sections from disk into memory
;;;
;;; This routine loads the runtime code into memory at $200, and the diskio
;;; code into memory at $12000. The runtime is temporarily loaded to $8000, 
;;; and then copied to its final destination.
;;;
;;; Will disable interrupts before the runtime is copied to $200, and will
;;; keep them disabled when returning.
;;;
;;; ***************************************************************************

		.section startup, root, noreorder
load_runtime_and_diskio:
		lda reloc_source_runtime
		sta file_load_address
		lda reloc_source_runtime+1
		sta file_load_address+1
		lda #3
		ldx #.byte0 filename_runtime
		ldy #.byte1 filename_runtime
		jsr load_file
		stx reloc_size_runtime
		tya
		sec
		sbc reloc_source_runtime+1
		sta reloc_size_runtime+1

		; load diskio code to 0x12000
		lda #0x00
		sta file_load_address
		lda #0x20
		sta file_load_address+1
		lda #0x01
		sta file_load_address+2
		lda #3
		ldx #.byte0 filename_diskio
		ldy #.byte1 filename_diskio
		jsr load_file

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

relocate_runtime:
		; move runtime to 0x0200
		lda #1
		trb 0xd703		; disable F018B mode
		sta 0xd707
		.byte 0			; end of job options
		.byte 0			; copy command
reloc_size_runtime:
		.word 0			; count
reloc_source_runtime:
		.word 0x8000		; source
		.byte 0			; source bank
		.word 0x200		; destination
		.byte 0			; destination bank
		.byte 0			; cmd high
		.byte 0			; modulo / ignored

relocate_init:
		sta 0xd707
		.byte 0				; end of job options
		.byte 0				; copy command, chained
		.word (.sectionSize init_copy)	; count
		.word (.sectionStart init_copy)	; source
		.byte 0				; source bank
		.word 0x4000			; destination
		.byte 0				; destination bank
		.byte 0				; cmd high
		.byte 0				; modulo / ignored
		
		rts

;;; ***************************************************************************
;;;
;;; load_file - load a file from disk into memory
;;;
;;; This routine loads a file from disk into memory. It needs to be called
;;; with the filename ptr in x/y and the load address in file_load_address.
;;;
;;; It returns the pointer to the first byte after the loaded file in x/y.
;;;
;;; ***************************************************************************

		.section startup, root, noreorder
load_file:
		jsr 0xffbd		; SETNAM
		lda #32
		ldx #8
		ldy #0
		jsr 0xffba		; SETLFS
		;; need to comment out as MEGA65 ROM is broken and doesn't support SETBNK
		;lda file_load_address+2
		;ldx #0
		;jsr 0xff6b		; SETBNK
		;; set BANK and MSB of load address via ZP instead ...
		sta 0xb0
		lda file_load_address+2
		sta 0xaf
		ldy file_load_address+1
		ldx file_load_address
		lda #0b01000000		; bit6 = raw read (don't skip first 2 bytes)
		jsr 0xffd5		; LOAD

		bcc loadok$
		lda #2
		sta 0xd020
		pla
		pla
		pla
		pla
		rts			; exit to basic
loadok$:
		rts
file_load_address:
		.word 0
		.byte 0

filename_runtime:
		.ascii "M00"
		
filename_diskio:
		.ascii "M11"
