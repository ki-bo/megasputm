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
  .public _prepareKernalWrite
  .public _performKernalWrite
  .public _finishKernalWrite
  .public _performKernalHeaderChange
  .public _performKernalScatchAllRooms

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

  .extern kernal_error

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


kernal_get_status:
    ;lda #0
    ;ldx #0
    ;jsr 0xff6b        ;set_banks

    lda #0xff
    sta kernalDriveStatus
    lda #0xff
    sta kernalDriveStatus + 1


    lda #0x0F         ; Logical file number (0 = auto)
    ldx #0x08         ; Device number (8 = default drive)
    ldy #0x0F         ; Device channel (15 = error channel)
    jsr 0xFFBA        ; SETLFS (Set Logical File System)

    lda #0x00         ; File name length (none needed)
    ldx #0x00
    jsr 0xFFBD        ; SETNAM (Set file name)

    jsr 0xFFC0        ; OPEN file (error channel 15)

    ldx #0x0F         ; Channel number (15)
    jsr 0xFFC6        ; CHKIN (Set input channel)

    ldy #0
read_error$:
    jsr 0xffb7
    bne done$

    jsr 0xFFCF        ; BASIN (Read a byte from input)
    bcs done$          ; If null terminator, we're done
    ;beq done$
    
    ;jsr 0xFFD2        ; Print character to screen
    sta kernalDriveStatus, y
    sta 0x0850, y

    iny
    cpy #0x02
    beq done$
    bra read_error$    ; Always branches, loop until null

done$:
    ;jsr 0xFFCC        ; CLRCHN (Clear input channel)
    
    clc
    lda #0x0F         ; File number (15)
    jsr 0xFFC3        ; CLOSE file

    lda kernalDriveStatus
    cmp #0x30
    bne error$
    lda kernalDriveStatus + 1
    cmp #0x30
    bne error$

    clc
    lda #0x00
    rts

error$:
    sec
    lda #0x01
    rts               ; Return


_finishKernalWrite:
    sei

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    jsr kernal_reset_channels

    lda #1
    ;clears the carry
    jsr kernal_close_logical_file

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

    jsr kernal_reset_channels

    ldx #1
    jsr kernal_set_logical_output

    ;lda #0x01
    ;pha

loop$:
    ldz #0
    lda [0x08], Z

    jsr kernal_write_byte

    ;pla
    ;;beq notest$
    ;bra notest$
    ;jsr kernal_get_status
    ;sta kernal_error
    ;bcs error$ 

cont$:
    ;pha

    clc
    ldq 0x08
    adcq 0x0c
    stq 0x08

    deq 0x04
    bne loop$
    bra done$

;notest$:
    ;lda #0x00
    ;bra cont$

done$:
    ;pla

error$:
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

    jsr kernal_open
    ;jsr 0xffc0

    ;jsr kernal_get_status
    ;sta kernal_error
    ;bcs done$    

    jsr kernal_reset_channels

    ldx #1
    jsr kernal_set_logical_output
    ;jsr 0xffc9


done$:
    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts


_performKernalHeaderChange:
    sei

    pha

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    pla
    clc
    adc #0x30
    sta headerlabelid + 1

    lda #0
    ldx #0
    jsr kernal_set_banks





;-------------------------------------------------------------------------------------------

    ;open 1,8,15,"i0"

;-------------------------------------------------------------------------------------------


    lda #2
    ldx #.byte0 fileinit
    ldy #.byte1 fileinit
    jsr kernal_set_name

    lda #1
    ldx #8
    ldy #15
    jsr kernal_set_logical_file

    jsr kernal_open

    jsr 0xffb7
    sta 0x0800
    cmp #0x00
    lbne done$




;-------------------------------------------------------------------------------------------

    ;open 2,8,2,"#"

;-------------------------------------------------------------------------------------------

    lda #1
    ldx #.byte0 filedata
    ldy #.byte1 filedata
    jsr kernal_set_name

    lda #2
    ldx #8
    ldy #2
    jsr kernal_set_logical_file

    jsr kernal_open

    jsr 0xffb7
    sta 0x0801
    cmp #0x00
    bne donecleanup0$





;-------------------------------------------------------------------------------------------

    ;print#1,"u1:";2;0;40;0:

;-------------------------------------------------------------------------------------------

    jsr kernal_reset_channels

    ldx #0x01
    jsr kernal_set_logical_output
    
    ldy #0x00
loop0$:
    lda commandposition, Y
    jsr kernal_write_byte

    iny
    cpy #13
    bne loop0$



;-------------------------------------------------------------------------------------------

    ;print#1,"b-p:";2;4:

;-------------------------------------------------------------------------------------------

    jsr kernal_reset_channels

    ldx #0x01
    jsr kernal_set_logical_output


    ldy #0x00
loop1$:
    lda commandpointer, Y
    jsr kernal_write_byte

    iny
    cpy #9
    bne loop1$



;-------------------------------------------------------------------------------------------

    ;print#2,"NEW LABEL       ";

;-------------------------------------------------------------------------------------------


    jsr kernal_reset_channels

    ldx #0x02
    jsr kernal_set_logical_output

    ldy #0x00
loop2$:
    lda headerlabel, Y
    jsr kernal_write_byte
    iny
    cpy #25
    bne loop2$


;    ldy #0x00
;loop2$:
;    lda labeldata, Y
;    jsr kernal_write_byte
;    iny
;    cpy #16
;    bne loop2$



;-------------------------------------------------------------------------------------------

    ;print#1,"u2:";2;0;40;0

;-------------------------------------------------------------------------------------------


    jsr kernal_reset_channels

    ldx #0x01
    jsr kernal_set_logical_output
    
    ldy #0x00
loop3$:
    lda commandupdate, Y
    jsr kernal_write_byte

    iny
    cpy #13
    bne loop3$




;-------------------------------------------------------------------------------------------

    ;print#1,"i0"

;-------------------------------------------------------------------------------------------

    jsr kernal_reset_channels

    ldx #0x01
    jsr kernal_set_logical_output


    ldy #0x00
loop4$:
    lda fileinit, Y
    jsr kernal_write_byte

    iny
    cpy #0x03
    bne loop4$


donecleanup1$
    ;bra done$
 
    ;close 2


    jsr kernal_reset_channels

    lda #0x02
    jsr kernal_close_logical_file

    lda kernal_error
    sta 0x0802

donecleanup0$
    ;bra done$

    ;close 1

    jsr kernal_reset_channels

    lda #0x01
    jsr kernal_close_logical_file

    lda kernal_error
    sta 0x0803

    jsr kernal_get_status


done$:
    ;lda #8
    ;jsr kernal_close_all;

    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts



_performKernalScatchAllRooms:
    sei

    jsr _judeBackupOwnZP
    jsr _judeRestoreKernalZP

    cli

    lda #0
    ldx #0
    ldy #0
    jsr kernal_set_name

    lda #1
    ldx #8
    ldy #15
    jsr kernal_set_logical_file

    jsr kernal_open

    jsr 0xffb7
    sta 0x0800
    cmp #0x00
    lbne done$


    jsr kernal_reset_channels

    ldx #0x01
    jsr kernal_set_logical_output
    
    ldy #0x00
loop3$:
    lda scratchall, Y
    jsr kernal_write_byte

    iny
    cpy #9
    bne loop3$

    jsr kernal_reset_channels

    lda #0x01
    jsr kernal_close_logical_file

    lda kernal_error
    sta 0x0803

    jsr kernal_get_status


done$:
    sei

    jsr _judeBackupKernalZP
    jsr _judeRestoreOwnZP

    cli

    rts




  .section data, data

  .public kernalWriteSrc
  .public kernalWriteSiz


kernalWriteSrc:
  .long 0x00000000
kernalWriteSiz:
  .long 0x00000000

kernalDriveStatus:
  .word 0xffff

scratchall:
  .ascii "S0:??.LFL"

filename:
  .asciz "@:TEST.DAT,S,W"

fileinit:
  .ascii "I0"
  .byte 0x0d

filedata:
  .ascii "#0"
  .byte 0x0d

commandcoldstart:
  .byte "UJ", 0x0d

commandposition:
  .byte "U1: 2 0 40 0", 0x0d   ; 0x02 0x00, 0x28, 0x00, 0x0a

commandupdate:
  .byte "U2: 2 0 40 0", 0x0d   ; 0x02 0x00, 0x28, 0x00, 0x0a

commandpointer:
  .byte "B-P: 2 4", 0x0d       ;, 0x02, 0x00, 0x0a

labeldata:
  .ascii "LABEL TEST      " 

headerdata:
  .byte 0x28, 0x03, 0x44, 0x00        ;4
headerlabel:
  .ascii "MANIAC MANSION"
  .byte 0xa0, 0xa0                    ;16
headerpadding0:
  .byte 0xa0, 0xa0                    ;2
headerlabelid:
  .byte 'M', 0xa0                     ;2
headerpadding1:
  .byte 0xa0                          ;1
headerdiskid:
  .byte 0x31, 0x44                    ;2
headerpadding2:
  .byte 0xa0, 0xa0                    ;2      - 29 bytes

