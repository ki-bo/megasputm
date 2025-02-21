;		.public __program_start
;
;		.section programStart
;
 ; .extern   _judeBackupOwnZP
  ;.extern  main
;
;__program_start:
 ; jsr _judeBackupKernalZP
  ;jmp main


	CPU_IRQ: .equ 	0xFFFE

 .section code
  .public processTest
  .public _prepareKernalWrite
  .public _performKernalWrite
  .public _finishKernalWrite

  .extern kernal_get_last_error
  .extern kernal_close_all
  .extern kernal_set_banks
  .extern kernal_set_banks_long
  .extern kernal_set_logical_file
  .extern kernal_set_name
  .extern kernal_open
  .extern kernal_set_logical_output
  .extern kernal_write_byte
  .extern kernal_close_logical_file
  .extern kernal_reset_channels

  .extern   _judeBankKernal
  .extern   _judeUnbankKernal
  .extern   _judeViewInit
  .extern   judeInit

  .extern   _judeBackupKernalZP
  .extern   _judeBackupOwnZP
  .extern   _judeRestoreKernalZP
  .extern   _judeRestoreOwnZP

  .extern   _judeUserIRQ
  ;.extern   jude_kernirq

kernal_delay:

    ldx #0x01
loop0$:
    ldy #0x0f
loop1$:
    dey 
    bne loop1$

    dex
    bne loop0$

    rts

_finishKernalWrite:
    sei

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    lda #1
    jsr kernal_close_logical_file

    jsr kernal_delay

    ;lda #8
    ;jsr kernal_close_all

    ;sei
    ;jsr kernal_reset_channels
    
    ;lda #.byte0 _judeUserIRQ
    ;sta CPU_IRQ
    ;lda #.byte1 _judeUserIRQ
    ;sta CPU_IRQ + 1

    ;cli

    ;jsr kernal_delay

    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts


_performKernalWrite:
    sei

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    lda kernalWriteSiz
    sta 0x04
    lda kernalWriteSiz + 1
    sta 0x05
    lda kernalWriteSiz + 2
    sta 0x06 
    lda kernalWriteSiz + 3
    sta 0x07

    lda 0x04
    ora 0x05
    ora 0x06
    ora 0x07
    beq done$

    lda kernalWriteSrc
    sta 0x08
    lda kernalWriteSrc + 1
    sta 0x09
    lda kernalWriteSrc + 2
    sta 0x0A 
    lda kernalWriteSrc + 3
    sta 0x0B

    lda #1
    sta 0x0C
    lda #0
    sta 0x0D
    sta 0x0E
    sta 0x0F


loop$:
    jsr kernal_delay


    ldz #0
    lda [0x08], Z

    jsr kernal_write_byte

    clc
    ldq 0x08
    adcq 0x0c
    stq 0x08

    deq 0x04
    bne loop$

done$:
    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts



_prepareKernalWrite:
    phx
    pha

    sei

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    lda #0
    ldx #0
    jsr kernal_set_banks
    ;jsr 0xff6b

    lda #1
    ldx #8
    ldy #2
    jsr kernal_set_logical_file
    ;jsr 0xffba

    lda #14
    ;ldx #.byte0 filename
    plx
    ;ldy #.byte1 filename
    ply
    jsr kernal_set_name
    ;jsr 0xffbd

    jsr kernal_delay


    jsr kernal_open
    ;jsr 0xffc0

    jsr kernal_delay

    ldx #1
    jsr kernal_set_logical_output
    ;jsr 0xffc9

    jsr kernal_delay

    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts



processTest:
    sei
    ;jsr judeInit
    ;rts

    phz
    phy
    phx
    pha

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    ;lda jude_kernirq
    ;sta CPU_IRQ
    ;lda jude_kernirq + 1
    ;sta CPU_IRQ + 1

    cli

    ;bra finish$

    lda #0
    sta 0xd020

    ;bra testerr$

    ;lda #8
    ;;jsr kernal_close_all
    ;jsr 0xff50

    lda #1
    sta 0xd020

    lda #0
    ldx #0
    ;jsr kernal_set_banks
    jsr 0xff6b

    lda #2
    sta 0xd020

    lda #1
    ldx #8
    ldy #2
    ;jsr kernal_set_logical_file
    jsr 0xffba

    lda #3
    sta 0xd020

    lda #14
    ldx #.byte0 filename
    ldy #.byte1 filename
    ;jsr kernal_set_name
    jsr 0xffbd

    lda #4
    sta 0xd020

    ;jsr kernal_open
    jsr 0xffc0

    ;bra finish$

;halt$:
;     inc 0xd020
;     bra halt$



testerr$:
    ;jsr diagnoseerror
    ;bra finish$


    ;cmp #0
    ;bne error$

    lda #5
    sta 0xd020

    ldx #1
    ;jsr kernal_set_logical_output
    jsr 0xffc9

    ;cmp #0
    ;bne error$

    ldy #0xff
loop0$:
    ldx #0xff
loop1$:

    ;lda #65
    txa

    sta 0xd020

    ;jsr kernal_write_byte
    jsr 0xffd2

    ;cmp #0
    ;bne error$
    
    dex
    bne loop1$

    dey
    bne loop0$

    bra success$

error$:
    lda #2
    sta 0xd020

    bra exit$

success$:

    lda #14
    sta 0xd020

    lda #1
    jsr kernal_close_logical_file
    
exit$:
    lda #8
    jsr kernal_close_all
    jsr kernal_reset_channels

finish$:
    sei

    jsr _judeBackupKernalZP

    ;jsr _unbankKernal
    jsr _judeRestoreOwnZP

    ;lda #.byte0 _judeUserIRQ
    ;sta CPU_IRQ
    ;lda #.byte1 _judeUserIRQ
    ;sta CPU_IRQ + 1

    ;jsr judeInit

    pla
    plx
    ply
    plz

    cli

    rts




  .section data, data

  .public kernalWriteSrc
  .public kernalWriteSiz


kernalWriteSrc:
  .long 0x00000000
kernalWriteSiz:
  .long 0x00000000

filename:
  .asciz "@:TEST.DAT,S,W"