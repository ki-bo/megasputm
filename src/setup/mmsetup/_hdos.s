;===========================================================
;HDOS simplified SD card DOS replacement for MEGA65
;
;Version 0.20A
;Written by Daniel England of Ecclestial Solutions.
;
;Copyright 2021, Daniel England. All Rights Reserved.
;
;-----------------------------------------------------------
;
;A simple DOS replacement library.  Use hdosSetFileName
;to describe your file and hdosOpenFile to open it.
;
;You must put the pages to use for the disk read buffer and
;file name transfer buffer into ptrhdosBufHi and 
;ptrhdosXfrHi, respectively.
;
;Supports only character reads but block reads will be added
;in the future.  Read a byte with hdosReadByte.
;
;-----------------------------------------------------------
;
;I want to release this under the LGPL.  I'll make the 
;commitment and include the licensing infomation soon.
;
;===========================================================


		.rtmodel cpu, "*"

		.extern _Zp


;===========================================================
    .section zzpage, bss
    .public ptrhdosBufHi
    .public ptrhdosXfrHi
;===========================================================

ptrhdosBufHi:   
    .space 1 //=	$DE
ptrhdosXfrHi:
    .space 1 //=	$DF

//ptrhdosFNmHi	=	$DF		;deprecated

ptrhdosBufOff:
    .space 2 //=	$E2
ptrhdosBufDir:
    .space 2 //=	$E4
ptrhdosBufFNm:
    .space 2 //=	$E6

;===========================================================



;===========================================================
    .section data, data
;===========================================================

flghdosErr:
		.byte	0x01
sizhdosBuf:
		.word	0x0000
adrhdosFN:
		.word	0x0000

valhdosFType:
     .byte 0 //=	$E8


lsthdosDMA:
//  Copy $FFD6E00 - $FFD6FFF down to low memory 
//  MEGA65 Enhanced DMA options
        .byte 0x0A                // Request format is F018A
        .byte 0x80, 0xFF          // Source is $FFxxxxx
        .byte 0x81, 0x00          // Destination is $FF
        .byte 0x00                // No more options
//  F018A DMA list (MB offsets get set in routine)
        .byte 0x00                // copy + last request in chain
        .word 0x0200              // size of copy is 512 bytes
        .word 0x6E00              // starting at $6E00
        .byte 0x0D                // of bank $D
adrhdosDest:
        .word 0x8000              // destination address is $8000
        .byte 0x00                // of bank $0
        .word 0x0000               // modulo (unused)

;===========================================================



;===========================================================
		.section code
    .public _hdosSetFileName
    .public _hdosOpenFile
    .public _hdosCloseFile
    .public _hdosReadByte
    .public _hdosCloseDir
    .public _hdosOpenDir
    .public _hdosReadDir
    .public _hdosChangeDir
    .public _hdosLoadFileAttic
    .public _hdos_selectdrive
    .public _hdos_getcurrdrive
    .public _hdos_cdrootdir
    .public _hdos_closeall
    .public _hdos_getdefdrive
;===========================================================

_hdos_closeall:
    lda #0x22
    sta 0xD640
    clv

    rts

_hdos_getdefdrive:
		lda	#0x02
		sta	0xD640
		clv

    rts

_hdos_getcurrdrive:
      lda #0x04
      sta 0xD640
      clv
      rts

_hdos_cdrootdir:
      lda #0x3c
      sta 0xD640
      clv

      rts


_hdos_selectdrive:
      lda #0x06
      sta 0xD640
      clv
; do not use this .X value it is invalid
; you must restore prior value before 
; calling cdrootdir (be sure not to optimise this away)
      ldx #0x00

      rts


;-----------------------------------------------------------
_hdosSetFileName:
;-----------------------------------------------------------
;	.X		IN	File Name Addr Lo
;	.Y		IN	File Name Addr Hi
;
;	.C		OUT	Set if error
;-----------------------------------------------------------
;***Do error checking 'cause this is the interface

		phx
		phy

		jsr	__hdosCloseAll

		ply
		plx

		stx	zp:ptrhdosBufOff
		sty	zp:ptrhdosBufOff + 1

		lda	zp:ptrhdosXfrHi
		sta	nmsta$ + 2

		ldy	#0x00
NameCopyLoop$:
		lda	(zp:ptrhdosBufOff), y
nmsta$:
		sta	0x0300, y
		iny
		cmp	#0x00
		bne	NameCopyLoop$

		ldx	#0x00
		ldy	zp:ptrhdosXfrHi

		jsr	__hdosSetFN

		rts


;-----------------------------------------------------------
_hdosOpenFile:
;-----------------------------------------------------------
		lda	flghdosErr
		bne	err$

		jsr	__hdosOpenFile

		rts

err$:
		sec
		rts


;-----------------------------------------------------------
_hdosCloseFile:
;-----------------------------------------------------------
		jsr	__hdosCloseAll

		rts


;-----------------------------------------------------------
_hdosReadByte:
;-----------------------------------------------------------
;	.A		OUT	Data
;	.ST_C	OUT	Set if error
;-----------------------------------------------------------
;***Do more error checking 'cause this is the interface
		phx
		phy
		phz

		jsr	__hdosReadByte

		plz
		ply
		plx

		rts


;-----------------------------------------------------------
_hdosCloseDir:
;-----------------------------------------------------------
; closedir takes file descriptor as argument (appears in A)
;-----------------------------------------------------------
		phx

		tax
		lda	#0x16
		sta	0xd640
		clv

;		ldx	#0x00
		plx

		rts
	

;-----------------------------------------------------------
_hdosOpenDir:
;-----------------------------------------------------------
; Opendir takes no arguments and returns File descriptor in A
;-----------------------------------------------------------
;		ldx	#0x00
;		ldy	#0x00
;		ldz	#0x00
;
;		ldy	ptrhdosBufHi
;		ldx	#0x00

		lda	#0x12
		sta	0xd640
		clv

;		ldx	#0x00


		bcc	error$

		clc
		rts

error$:
		sec
		rts


;-----------------------------------------------------------
_hdosReadDir:
;-----------------------------------------------------------
; readdir takes the file descriptor returned by opendir as argument
; and gets a pointer to a MEGA65 DOS dirent structure.
; Again, the annoyance of the MEGA65 Hypervisor requiring a page aligned
; transfer area is a nuisance here. We will use $0400-$04FF, and then
; copy the result into a regular C dirent structure
;
; d_ino = first cluster of file
; d_off = offset of directory entry in cluster
; d_reclen = size of the dirent on disk (32 bytes)
; d_type = file/directory type
; d_name = name of file
;-----------------------------------------------------------
		phx
		phy
		phz

		pha
	
;;	FIRST, CLEAR OUT THE DIRENT
;		ldx	#0
;		txa
;l1$:
;		sta	_readdir_dirent, X
;		dex
;		bne	l1$

;@halt0:
;		lda	#0x0e
;		sta	0xd020
;		jmp	halt0$
;		lda	#0x00
;		sta	0xd020

;	Third, call the hypervisor trap
;	File descriptor gets passed in in X.
;	Result gets written to transfer area we setup 
		plx
		ldy	zp:ptrhdosXfrHi
		lda	#0x14
		sta	0xd640
		clv

		bcs	readDirSuccess$

;	Return end of directory
;		lda #0x00
;		ldx #0x00

		plz
		ply
		plx

		sec

		rts

readDirSuccess$:
		plz
		ply
		plx

    clc
    rts


		lda	#0x00
		sta	zp:ptrhdosBufDir
		lda	zp:ptrhdosXfrHi
		sta	zp:ptrhdosBufDir + 1

		lda	#0x00
		sta	zp:ptrhdosBufFNm
		lda	zp:ptrhdosBufHi
		sta	zp:ptrhdosBufFNm + 1

;	Copy file name
		ldy	#0x3F
l2$:
		lda	(zp:ptrhdosBufDir), y
		sta	(zp:ptrhdosBufFNm), y

		dey
		bpl	l2$


;	make sure it is null terminated

;   ldx 0x0400+64
;   lda #0x00
;   sta _readdir_dirent+4+2+4+2,x

		ldy	#64
		lda	(zp:ptrhdosBufDir), y
		tay
		lda	#0x00
		sta	(zp:ptrhdosBufFNm), y


;	;; Inode = cluster from offset 64+1+12 = 77
;	ldx #$03
;@l3:	
;	lda $0477,x
;	sta _readdir_dirent+0,x
;	dex
;	bpl @l3
;
;	;; d_off stays zero as it is not meaningful here
;	
;	;; d_reclen we preload with the length of the file (this saves calling stat() on the MEGA65)
;	ldx #3
;@l4:	
;	lda $0400+64+1+12+4,x
;	sta _readdir_dirent+4+2,x
;	dex
;	bpl @l4

;	File type and attributes
;	lda $0400+64+1+12+4+4
;	sta _readdir_dirent+4+2+4

		ldy	#64 + 1 + 12 + 4 + 4
		lda	(zp:ptrhdosBufDir), y
		sta	valhdosFType


;	Return address of dirent structure
;	lda #<_readdir_dirent
;	ldx #>_readdir_dirent
	
		plz
		ply
		plx

		clc

		rts


;-----------------------------------------------------------
_hdosChangeDir:
;-----------------------------------------------------------
		pha
		phx
		phy
		phz

		lda	#0x34
		sta	0xd640
		clv
		bcc	fail$

		lda	#0x0c
		sta	0xd640
		clv
		bcc	fail$

		lda	#0x18
		sta	0xd640
		clv

		plz
		ply
		plx
		pla

		clc

		rts

fail$:
		plz
		ply
		plx
		pla

		sec

		rts


;-----------------------------------------------------------

_hdosLoadFileAttic:

;-----------------------------------------------------------

    pha
    lda #0x3e
    sta 0xd640
    clv
    bcc fail$
    clc
    pla
    rts
fail$:
    pla
    sec
    rts


;===========================================================


;===========================================================
; Private....
;===========================================================


;-----------------------------------------------------------
__hdosOpenFile:
;-----------------------------------------------------------
;		jsr	__hdosCloseAll

		ldx	adrhdosFN
		ldy	adrhdosFN + 1

;		jsr	__hdosSetFN

		lda	#0x34
		sta	0xd640
		clv
		bcc	fail$

		lda	#0x00
		sta	0xd640
		clv

		lda	#0x18
		sta	0xd640
		clv

		lda	#0x00
		sta	flghdosErr
		sta	sizhdosBuf
		sta	sizhdosBuf + 1

		clc

		rts

fail$:
		lda	#0x01
		sta	flghdosErr

		sec
		rts


;-----------------------------------------------------------
__hdosReadByte:
;-----------------------------------------------------------
		lda	flghdosErr
		beq	begin$

		sec
		rts

begin$:
		lda	sizhdosBuf
		ora	sizhdosBuf + 1

		bne	cont0$

		jsr	__hdosReadSect

		lda	sizhdosBuf
		ora	sizhdosBuf + 1

		bne	cont0$

		lda	#0x01
		sta	flghdosErr

		sec
		rts

cont0$:
		ldy	#0x00
		lda	(zp:ptrhdosBufOff), y

		pha


;FIXME:  Can use INW??

		clc
		lda	zp:ptrhdosBufOff
		adc	#0x01
		sta	zp:ptrhdosBufOff
		lda	zp:ptrhdosBufOff + 1
		adc	#0x00
		sta	zp:ptrhdosBufOff + 1

;FIXME:  Can use DEW?

		sec
		lda	sizhdosBuf
		sbc	#0x01
		sta	sizhdosBuf
		lda	sizhdosBuf + 1
		sbc	#0x00
		sta	sizhdosBuf + 1

		pla

		clc
		rts


;-----------------------------------------------------------
__hdosReadSect:
;-----------------------------------------------------------
		lda	#0x00
		sta	zp:ptrhdosBufOff

		lda	zp:ptrhdosBufHi
		sta	zp:ptrhdosBufOff + 1
		sta	adrhdosDest + 1

		lda	#0x1A
		sta	0xD640
		clv

;halt$:
;		inc	0xd020
;		jmp	halt$


		stx	sizhdosBuf
		sty	sizhdosBuf + 1

		lda	sizhdosBuf
		ora	sizhdosBuf + 1

		beq	exit$

		lda	#0x80
		tsb	0xd689

		lda	#0x00
		sta	0xd702
		sta	0xd704
;		lda	#>lsthdosDMA
    lda #.byte1 lsthdosDMA
		sta	0xd701
;		lda	#<lsthdosDMA
    lda #.byte0 lsthdosDMA
		sta	0xd705

exit$:
		rts


;-----------------------------------------------------------
__hdosSetFN:
;-----------------------------------------------------------
		stx	adrhdosFN
		sty	adrhdosFN + 1

		sec

		lda	#0x2e				  ; dos_setname Hypervisor trap
		sta	0xd640				; Do hypervisor trap
		clv
		bcs	ok$

		lda	#0x01
		sta	flghdosErr

		sec
		rts

ok$:
		lda	#0x00
		sta	flghdosErr

		clc
		rts


;-----------------------------------------------------------
__hdosCloseAll:
;-----------------------------------------------------------
		lda	#0x22
		sta	0xd640
		clv

		lda	#0x00
		sta	flghdosErr

		rts