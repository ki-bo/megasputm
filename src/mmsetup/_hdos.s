;=============================================================================
;HDOS simplified SD card DOS replacement for MEGA65
;=============================================================================
;
; Based on Bigglesworth 0.20A.
;
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
    .public ptrhdosBufHi
    .public ptrhdosXfrHi
;===========================================================

ptrhdosBufHi:   
    .space 1
ptrhdosXfrHi:
    .space 1

ptrhdosBufOff:
    .space 2
ptrhdosBufDir:
    .space 2
ptrhdosBufFNm:
    .space 2
sizhdosBuf:
    .space 2

;===========================================================



;===========================================================
    .section data, data
;===========================================================

flghdosErr:
		.byte	0x01
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
    .public _hdos_attachD810
;===========================================================

_hdos_attachD810:
    lda #0x40
    sta 0xD640
    clv

    bcc fail$

    clc
    rts

fail$:
    sec
    rts


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

		plx

		rts
	

;-----------------------------------------------------------
_hdosOpenDir:
;-----------------------------------------------------------
; Opendir takes no arguments and returns File descriptor in A
;-----------------------------------------------------------
		lda	#0x12
		sta	0xd640
		clv

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
	
;	Third, call the hypervisor trap
;	File descriptor gets passed in X.
;	Result gets written to transfer area we setup 
		plx
		ldy	zp:ptrhdosXfrHi
		lda	#0x14
		sta	0xd640
		clv

		bcs	readDirSuccess$

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
; the file length here is limited to 64 bytes in the read
; but for the open, you are limited to 63 so we make this 
; null

		ldy	#64
		lda	(zp:ptrhdosBufDir), y
		tay
		lda	#0x00
		sta	(zp:ptrhdosBufFNm), y


;	File type and attributes

		ldy	#64 + 1 + 12 + 4 + 4
		lda	(zp:ptrhdosBufDir), y
		sta	valhdosFType

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

;   Set file name must be called prior

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
		sta	zp:sizhdosBuf
		sta	zp:sizhdosBuf + 1

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
		lda	zp:sizhdosBuf
		ora	zp:sizhdosBuf + 1

		bne	cont0$

		jsr	__hdosReadSect

		lda	zp:sizhdosBuf
		ora	zp:sizhdosBuf + 1

		bne	cont0$

		lda	#0x01
		sta	flghdosErr

		sec
		rts

cont0$:
		ldy	#0x00
		lda	(zp:ptrhdosBufOff), y

		pha

		inw	zp:ptrhdosBufOff
		dew	zp:sizhdosBuf

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

		stx	zp:sizhdosBuf
		sty	zp:sizhdosBuf + 1

		lda	zp:sizhdosBuf
		ora	zp:sizhdosBuf + 1

		beq	exit$

		lda	#0x80
		tsb	0xd689

		lda	#0x00
		sta	0xd702
		sta	0xd704
		lda	#.byte1 lsthdosDMA
		sta	0xd701
		lda	#.byte0 lsthdosDMA
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
