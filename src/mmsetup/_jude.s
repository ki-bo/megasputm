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

	#include	"_karljr_types.inc"
	#include	"_jude_types.inc"

  //#define	DEBUG_RASTERTIME 1

 .section code

	.public		judeInit
	.public		_judeViewInit
	.public		judeMain

	.public		judeDeActivatePage
	.public		judeActivatePage
	.public		judeUnDownCtrl
	.public		judeDownCtrl
	.public		judeDeActivateCtrl
	.public		judeActivateCtrl
	.public		judeSetPointer
  .public   _judeSetTheme

	.public		_judeEraseLine
	.public		judeEraseBkg
	.public		_judeDrawText
	.public		_judeDrawTextDirect
	.public		judeDrawAccel
	.public		_judeLogClrToSys
	.public		_judeLogClrIsReverse
	.public		judeEnqueueKey
	.public		judeDequeueKey
	.public		_judeMoveActiveControl

	.public		judeDefUIPrepare
	.public		judeDefUIInit
	.public		judeDefUIChange
	.public		judeDefUIRelease
	.public		judeDefViewPrepare
	.public		judeDefViewInit
	.public		judeDefViewChange
	.public		judeDefViewRelease
	.public		judeDefLyrPrepare
	.public		judeDefLyrInit
	.public		judeDefLyrChange
	.public		judeDefLyrRelease
	.public		judeDefPgePrepare
	.public		judeDefPgeInit
	.public		judeDefPgeChange
	.public		judeDefPgeRelease
	.public		judeDefPgePresent
	.public		judeDefPnlPrepare
	.public		judeDefPnlInit
	.public		judeDefPnlChange
	.public		judeDefPnlRelease
	.public		judeDefPnlPresent

	.public		_judeProcessAccelerators

	.public		jude_cellsize
	.public		jude_coloury0
	.public		jude_screeny0
	.public		jude_actvpg
	.public		judeDownElem

  .public   theme0

	.public		mouseXCol
	.public		mouseYRow

  .public   themeCnt;
  .public   actvtheme;

  .public   jude_onidle
  .public   jude_initflags

  .public   _judeBackupKernalZP
  .public   _judeBackupOwnZP
  .public   _judeRestoreKernalZP
  .public   _judeRestoreOwnZP

  .public   _judeUserIRQ

  .public   mouseXPos
  .public   mouseYPos

  .extern   input_update

  .extern		karlASCIIToScreen
	.extern		karlGetLastError
	.extern		karlPanic
	.extern		karlIOFast
	.extern		_karlModAttach
	.extern		karlObjExcludeState
	.extern		karlObjIncludeState

	.extern		_karlProcObjLst
	.extern		karl_dirty
	.extern		karl_changed
	.extern		karl_lock
	.extern		_karlCallObjLstMethod
	.extern		karl_errorno

	.extern		karlDefModPrepare
	.extern		karlDefModInit
	.extern		karlDefModChange
	.extern		karlDefModRelease


;-----------------------------------------------------------
judeInit:
;-----------------------------------------------------------
		MvDWImmW zp:zreg0, mod_jude_core
		jsr	_karlModAttach

    lda #0x00
    sta actvtheme

		ldx	#sizeof_NAME
loop0$:
		lda	theme0, x
		sta	jude_theme - sizeof_NAME, x

		inx
		cpx	#sizeof_NAME + 0x0F
		bne	loop0$

;***FIXME Check return
		rts


;-----------------------------------------------------------
_judeViewInit:
;	reg0		IN		VIEW
;-----------------------------------------------------------
    MvDWMem	zp:zptrself, zp:zreg0
		MvDWMem	zp:zreg8, zp:zptrself

		lda	#ERROR_NONE
		sta	karl_errorno

;	Check prepared
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z

		and	#STATE_PREPARED
		lbeq	exit$

	#ifdef	DEBUG_RASTERTIME
		lda	#0x07
		sta	VIC_BRDRCLR
	#endif

;	Prepare owned objects (bars, pages only here)
		lda	#VIEW__barscnt
		sta	zp:zreg5b0

		lda	#VIEW__bars_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	karl_errorno
		lbne	exit$

		lda	#VIEW__pagescnt
		sta	zp:zreg5b0

		lda	#VIEW__pages_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	karl_errorno
		lbne	exit$

;	Init
		lda	#ERROR_NONE
		sta	karl_errorno

		ldz	#OBJECT__initialise
		;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr
		inz
		;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr + 1

		beq	exit$

		ldz	#0x00
		jsr	_judeProxy

		lda	#0x00
		sta	jude_avhrsx

		ldz	#VIEW__width
		;nop
		lda	[zp:zptrself], z
		cmp	#0x29
		bcc cont0$

		lda	#0x01
		sta	jude_avhrsx

cont0$:
		lda	#0x00
		sta	jude_avhrsy

		ldz	#VIEW__height
		;nop
		lda	[zp:zptrself], z
		cmp	#0x1A
		bcc cont1$

		lda	#0x01
		sta	jude_avhrsy

cont1$:
		MvDWMem	jude_actvvw, zp:zptrself
		MvDWObjImm	zp:zreg4, zp:zptrself, VIEW__actvpage_p
		MvDWMem	zp:zptrself, zp:zreg4

		jsr	judeActivatePage

		lda	#ERROR_NONE
		sta	karl_errorno

exit$:
		rts


;-----------------------------------------------------------
judeMain:
;-----------------------------------------------------------
main$:
		cli

; Check no need to panic
    lda CPU_IRQ
    cmp #.byte0 _judeUserIRQ
    bne panic$
    lda CPU_IRQ + 1
    cmp #.byte1 _judeUserIRQ
    beq cont0$

panic$:
    jmp karlPanic

cont0$:
;	Check not locked
;	Suspend IRQ
		sei
		lda	karl_lock
		bne	main$


;	Set processed this frame
		lda	#0x01
		sta	jude_proc

		cli

		lda	jude_actvpg
		ora	jude_actvpg + 1
		ora	jude_actvpg + 2
		ora	jude_actvpg + 3
		beq	done$

;	Check Changed
		lda	karl_changed
		beq	dirty$

		jsr	_judeUpdateChanged

		lda	#0x00
		sta	karl_changed
		bra	done$

;	Check Keys
keys$:
		jsr	_judeSendKeys
    bra finish$

;	Check Dirty
dirty$:
		lda	karl_dirty
		beq	keys$

		jsr	_judePresentDirty

		lda	#0x00
		sta	karl_dirty

    bra done$

finish$:
    lda jude_onidle
    ora jude_onidle + 1
    beq done$

    jsr _judeOnIdleProxy

done$:
		sei
		lda	#0x00
		sta	jude_proc

		bra	main$


;-----------------------------------------------------------
judeDeActivatePage:
;-----------------------------------------------------------
		lda	#STATE_ACTIVE
		jsr	karlObjExcludeState
		
		lda	#STATE_DIRTY
		jsr	karlObjExcludeState

		MvDWZ	jude_actvpg

;**FIXME	Update view?

		jsr	_mouseUnPickElement
		jsr	judeDeActivateCtrl

		rts


;-----------------------------------------------------------
judeActivatePage:
;-----------------------------------------------------------
		lda	jude_actvpg
		ora	jude_actvpg + 1
		ora	jude_actvpg + 2
		ora	jude_actvpg + 3
		beq	activate$

		LDQMem	zp:zptrself
		STQMem	zp:zreg4

		LDQMem	jude_actvpg
		STQMem	zp:zptrself

		jsr	judeDeActivatePage

		LDQMem	zp:zreg4
		STQMem	zp:zptrself

activate$:
		MvDWMem	jude_actvpg, zp:zptrself

		lda	#STATE_DIRTY | STATE_ACTIVE
		jsr	karlObjIncludeState

		lda	#KEY_C64_CDOWN
		sta	zp:zvalkey
		lda	#0x00
		sta	zp:zvalkey + 1

		jsr	_judeMoveActiveControl

;**FIXME	Update view?

		rts


;-----------------------------------------------------------
judeUnDownCtrl:
;-----------------------------------------------------------
		lda	judeDownElem
		ora	judeDownElem + 1
		ora	judeDownElem + 2
		ora	judeDownElem + 3
		beq	exit$

		MvDWMem	zptrself, judeDownElem

		lda	#STATE_DOWN
		jsr	karlObjExcludeState

		MvDWZ	judeDownElem

exit$:
		rts


;-----------------------------------------------------------
judeDownCtrl:
;-----------------------------------------------------------
		MvDWMem	zp:zreg4, zp:zptrself

		jsr	judeUnDownCtrl

		ldz	#OBJECT__options
		;nop
		lda	[zp:zreg4], z
		and	#OPT_NONAVIGATE
		bne	nodeact$

		jsr	judeDeActivateCtrl

nodeact$:
		MvDWMem	zp:zptrself, zp:zreg4

;		lda	#0x00
;		sta	zp:zreg2b3
		lda	#STATE_DOWN
		jsr	karlObjIncludeState

		MvDWMem	judeDownElem, zptrself

		ldz	#OBJECT__options
		;nop
		lda	[zptrself], z
		and	#OPT_NONAVIGATE
		bne	noact$

		jsr	judeActivateCtrl

noact$:
		rts


;-----------------------------------------------------------
judeDeActivateCtrl:
;-----------------------------------------------------------
		lda	judeActvElem
		ora	judeActvElem + 1
		ora	judeActvElem + 2
		ora	judeActvElem + 3
		beq	exit$

		MvDWMem	zp:zptrself, judeActvElem

		lda	#STATE_ACTIVE
		jsr	karlObjExcludeState

		MvDWZ	judeActvElem

exit$:
		rts


;-----------------------------------------------------------
judeActivateCtrl:
;-----------------------------------------------------------
		MvDWMem	zp:zreg4, zptrself

		jsr	judeDeActivateCtrl
		
		MvDWMem	zp:zptrself, zp:zreg4

;		lda	#0x00
;		sta	zp:zreg2b3
		lda	#STATE_ACTIVE
		jsr	karlObjIncludeState

		MvDWMem	judeActvElem, zptrself

		rts


;-----------------------------------------------------------
judeSetPointer:
;-----------------------------------------------------------
		pha

		LDQMem	jude_mouseptr
		STQMem	zp:zregA

		lda	jude_mousevic
		sta	zp:zregBwl
		lda	jude_mousevic + 1
		sta	zp:(zregBwl + 1)

		pla

		cmp	#MPTR_NORMAL
		beq	move$

		cmp	#MPTR_WAIT
		bne	exit$

		inw	zp:zregBwl
		inw	zp:zregBwl
		inw	zp:zregBwl

		pha

		jsr	_mouseUnPickElement

		pla

move$:
		sta	jude_mptrstate

;	Sprite pointer
		ldz	#0x00
		lda	zp:zregBwl
		;nop
		sta	[zp:zregA], z

		inz
		lda	zp:zregBwl + 1
		;nop
		sta	[zp:zregA], z
		
exit$:
		rts


;-----------------------------------------------------------
_judeSetTheme:
;-----------------------------------------------------------
    cmp themeCnt
    bcs exit$

    sta actvtheme

    tax
    lda #.byte0 theme0
    sta zp:zreg4wl
    lda #.byte1 theme0
    sta zp:zreg4wl + 1

loop$:
    cpx #0x00
    beq update$

    clc
    lda #32
    adc zp:zreg4wl
    sta zp:zreg4wl
    lda #0
    adc zp:zreg4wl + 1
    sta zp:zreg4wl + 1

    dex
    bra loop$

update$:
		ldy	#sizeof_NAME
loop0$:
		lda	(zp:zreg4wl), y
		sta	jude_theme - sizeof_NAME, y

		iny
		cpy	#sizeof_NAME + 0x0F
		bne	loop0$

		lda	#.byte0 CLR_EMPTY
		ldx	#.byte1 CLR_EMPTY
		jsr	_judeLogClrToSys
		sta	0xD020

		lda	#.byte0 CLR_BACK
		ldx	#.byte1 CLR_BACK
		jsr	_judeLogClrToSys
		sta	0xD021

    jsr _judeThemeSetMouse

    MvDWMem zp:zptrself, jude_actvpg

		lda	#STATE_DIRTY
		jsr	karlObjIncludeState

exit$:
    rts


;-----------------------------------------------------------
_judeEraseLine:
;	IN	zregAwl		colour
;	IN	zregAb3		Max width
;	IN	zregBb1		x pos
;	IN	zregBb2		y pos
;-----------------------------------------------------------
		lda	zp:zregAwl
		sta	zp:zreg9b0			;colour
		ldx	zp:(zregAwl + 1)
		stx	zp:zreg9b1

		lda	zp:zregAb3
		sta	zp:zregBb3

		lda	jude_cellsize
		cmp	#0x02
		bne	single$

		lda	zp:zregBb1
		asl	a
		sta	zp:zregBb1

single$:
		lda	zp:zreg9b0
		ldx	zp:zreg9b1
		
		jsr	_judeLogClrIsReverse
		bcc	text$
		
		lda	#KEY_ASC_SPACE
		jsr	karlASCIIToScreen
		ora	#0x80

		bra	cont$

text$:
		lda	#KEY_ASC_SPACE
		jsr	karlASCIIToScreen
		
cont$:
		sta	zp:zreg9b2		;background char

		lda	zp:zreg9b0
		ldx	zp:zreg9b1
		jsr	_judeLogClrToSys

		sta	zp:zreg9b0		;system colour

		lda	zp:zregBb2
		asl a
		asl a
		tax

		lda	jude_screeny0, x
		sta	zp:zptrscreen		;screen ptr
		lda	jude_screeny0 + 1, x
		sta	zp:zptrscreen + 1
		lda	jude_screeny0 + 2, x
		sta	zp:zptrscreen + 2
		lda	jude_screeny0 + 3, x
		sta	zp:zptrscreen + 3

		lda	jude_coloury0, x
		sta	zp:zptrcolour		;colour ptr
		lda	jude_coloury0 + 1, x
		sta	zp:zptrcolour + 1
		lda	jude_coloury0 + 2, x
		sta	zp:zptrcolour + 2
		lda	jude_coloury0 + 3, x
		sta	zp:zptrcolour + 3

    lda zp:zregBb1
    taz
		ldx	zp:zregBb3
		dex


;***FIXME!!!
;	Replace this loopw with a dma job.

loopw$:
		lda	zp:zreg9b2				;char to screen ram
		;nop
		sta	[zp:zptrscreen], z
		
		lda	jude_cellsize
		cmp	#0x02
		bne	clrsingle$

		inz

		lda	#00				;hi char to screen ram
		;nop
		sta	[zp:zptrscreen], z

clrsingle$:
		lda	zp:zreg9b0				;colour to colour ram
		;nop
		sta	[zp:zptrcolour], z

		inz

cellcont$:
		dex
		bpl	loopw$

		rts



;-----------------------------------------------------------
judeEraseBkg:
;-----------------------------------------------------------
		sta	zp:zreg9b0			;colour
		stx	zp:zreg9b1

		ldz	#ELEMENT__posx

		LDQIndDWZ	zp:zptrself
		STQMem	zp:zreg8

		lda	jude_cellsize
		cmp	#0x02
		bne	single$

		lda	zp:zreg8
		asl a
		sta	zp:zreg8

single$:
		lda	zp:zreg9b0
		ldx	zp:zreg9b1
		
		jsr	_judeLogClrIsReverse
		bcc	text$
		
		lda	#KEY_ASC_SPACE
		jsr	karlASCIIToScreen
		ora	#0x80

		bra	cont$

text$:
		lda	#KEY_ASC_SPACE
		jsr	karlASCIIToScreen
		
cont$:
		sta	zp:zreg9b2		;background char

		lda	zp:zreg9b0
		ldx	zp:zreg9b1
		jsr	_judeLogClrToSys

		sta	zp:zreg9b0		;system colour

looph$:
		lda	zp:zreg8b1
		asl a
		asl a
		tax

		lda	jude_screeny0, x
		sta	zp:zptrscreen		;screen ptr
		lda	jude_screeny0 + 1, x
		sta	zp:zptrscreen + 1
		lda	jude_screeny0 + 2, x
		sta	zp:zptrscreen + 2
		lda	jude_screeny0 + 3, x
		sta	zp:zptrscreen + 3

		lda	jude_coloury0, x
		sta	zp:zptrcolour		;colour ptr
		lda	jude_coloury0 + 1, x
		sta	zp:zptrcolour + 1
		lda	jude_coloury0 + 2, x
		sta	zp:zptrcolour + 2
		lda	jude_coloury0 + 3, x
		sta	zp:zptrcolour + 3

		lda zp:zreg8b0
    taz

		ldx	zp:zreg8b2
		dex

;***FIXME!!!
;
;	This routine is now being called to clear the whole page!
;	Replace this loopw with a dma job.


loopw$:
		lda	zp:zreg9b2				;char to screen ram
		;nop
		sta	[zp:zptrscreen], z
		
		lda	jude_cellsize
		cmp	#0x02
		bne	clrsingle$

		inz

		lda	#00				;hi char to screen ram
		;nop
		sta	[zp:zptrscreen], z

clrsingle$:
		lda	zp:zreg9b0				;colour to colour ram
		;nop
		sta	[zp:zptrcolour], z

		inz

cellcont$:
		dex
		bpl	loopw$
		
		inc	zp:zreg8b1
		dec	zp:zreg8b3
		lda	zp:zreg8b3
		bne	looph$
		
		rts


;-----------------------------------------------------------
_judeDrawText:
;	IN	zregAwl		Colour
;	IN	zregAb2		Indent
;	IN	zregAb3		Max width
;	IN	zregBb0		Do cont char if opt
;-----------------------------------------------------------
		ldz	#ELEMENT__posx
		;nop
		lda	[zp:zptrself], z
		sta	zp:zregBb1		;x
		inz
		;nop
		lda	[zp:zptrself], z
		sta	zp:zregBb2		;y
;		iny
		
		MvDWObjImm	zp:zregD, zp:zptrself, CONTROL__text_p
		lda	zp:zregDb0
		ora	zp:zregDb1
		ora	zp:zregDb2
		ora	zp:zregDb3
		
		bne	cont0$
		
		rts
		
cont0$:
		ldz	#CONTROL__textoffx
		;nop
		lda	[zp:zptrself], z		;
		sta	zp:zregCb0		;text off x

;-----------------------------------------------------------
_judeDrawTextDirect:
;	IN	zregAwl		Colour
;	IN	zregAb2		Indent
;	IN	zregAb3		Max width
;	IN	zregBb0		Do cont char if opt
;	IN	zregBb1		x pos
;	IN	zregBb2		y pos
;	IN	zregCb0		text off x
;	IN	zregD		text pointer
;-----------------------------------------------------------

		clc
    lda zp:zregBb1
    sta 0x0800

		adc	zp:zregAb2

		ldx	jude_cellsize
		cpx	#0x02
		bne	xcont$

		asl a

xcont$:
		sta	zp:zregBb1		;x

		lda	zp:zregAb0
		ldx	zp:zregAb1
		jsr	_judeLogClrIsReverse
		bcc	text$
		
		lda	#0x80
		bra	cont1$
		
text$:
		lda	#0x00
		
cont1$:
		sta	zp:zregCb2		;char or for norm/rev

    sec
    lda zregAb3
    sbc zregAb2
    sta zregAb3


		lda	zp:zregBb2
		asl a
		asl a
		tax

		lda	jude_screeny0, x
		sta	zp:zptrscreen		;screen ptr
		lda	jude_screeny0 + 1, x
		sta	zp:zptrscreen + 1
		lda	jude_screeny0 + 2, x
		sta	zp:zptrscreen + 2
		lda	jude_screeny0 + 3, x
		sta	zp:zptrscreen + 3

		lda	zp:zregBb0
		beq	cont2$
	
		ldz	#OBJECT__options
		;nop
		lda	[zptrself], z
		and	#OPT_TEXTCONTMRK
		beq	cont2$

		dec	zp:zregAb3

cont2$:
		lda zp:zregCb0
    sta	zp:zregCb1

		ldx	#0x00
	
loopw$:
    lda zp:zregCb1
    taz

		;nop
		lda	[zp:zregD], z		;char 
		
		beq	exit$
		
		jsr	karlASCIIToScreen
		ora	zp:zregCb2
		
    pha
    lda zp:zregBb1
    taz
    pla

		;nop
		sta	[zp:zptrscreen], z
		
		inc	zp:zregBb1

		lda	jude_cellsize
		cmp	#0x02
		bne	cellcont$

		inc	zp:zregBb1

cellcont$:
		inc	zp:zregCb1

		inx
		cpx	zp:zregAb3
		bcs	contchk$

		bra	loopw$

contchk$:
		lda	zp:zregBb0
		beq	exit$

		ldz	#OBJECT__options
		;nop
		lda	[zp:zptrself], z
		and	#OPT_TEXTCONTMRK
		beq	exit$

;	FIXME:  This is for the text continuation mark
		lda	#0x68			; this is already screen code, not ascii
;		jsr	KarlASCIIToScreen
		ora	zp:zregCb2
		
    pha
    lda zp:zregBb1
    taz
    pla

		;nop
		sta	[zp:zptrscreen], z

exit$:
		rts


;-----------------------------------------------------------
judeDrawAccel:
;-----------------------------------------------------------
		ldz	#CONTROL__textaccel
		;nop
		lda	[zp:zptrself], z
		cmp	#0xFF
		beq	exit$

		sta	zp:zregBb3

		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z
		and	#STATE_ENABLED
		beq	exit$

		ldz	#ELEMENT__posx
		;nop
		lda	[zp:zptrself], z
		
		clc
		adc	zp:zregBb3

		ldx	jude_cellsize
		cpx	#0x02
		bne	storex$

		asl a
		tax
		inx
		txa

storex$:
		sta	zp:zregBb1		;x + textaccel
		
		inz
		;nop
		lda	[zp:zptrself], z
		sta	zp:zregBb2		;y
		
		lda	#.byte0 CLR_FOCUS
		ldx	#.byte1 CLR_FOCUS
		jsr	_judeLogClrToSys
		sta	zp:zregCb1		;system colour

		lda	zp:zregBb2
		asl a
		asl a
		tax

		lda	jude_coloury0, x
		sta	zp:zptrcolour		;screen ptr
		lda	jude_coloury0 + 1, x
		sta	zp:zptrcolour + 1
		lda	jude_coloury0 + 2, x
		sta	zp:zptrcolour + 2
		lda	jude_coloury0 + 3, x
		sta	zp:zptrcolour + 3

		ldz	#OBJECT__options
		;nop
		lda	[zp:zptrself], z
		and	#OPT_TEXTACCEL2X
		sta	zp:zregCb0

		;ldz	zp:zregBb1
    lda zp:zregBb1
    taz

		lda	zp:zregCb1
		;nop
		sta	[zp:zptrcolour], z
		
		ldx	zp:zregCb0
		beq	exit$
		
		ldx	jude_cellsize
		cpx	#0x02
		bne	dblx$

		inz

dblx$:
		inz
		;nop
		sta	[zp:zptrcolour], z

exit$:
		rts


;-----------------------------------------------------------
_judeLogClrToSys:
;-----------------------------------------------------------
		pha
		txa

		and	#0x10
		beq	ctrl$

		pla
		rts

ctrl$:
		ply
		iny
		iny
		iny
		lda	jude_theme, y
		rts


;-----------------------------------------------------------
_judeLogClrIsReverse:
;-----------------------------------------------------------
		pha
		txa

		and	#0x30
		beq	int$

    and #0x10
    bne systxt$

		pla

ctrl$:
		sec
		rts

systxt$:
    pla
    bra text$

int$:
		pla
		cmp	#CLR_EMPTY
		beq	ctrl$

		cmp	#CLR_FOCUS
		bpl	ctrl$

text$:
		clc
		rts


;-----------------------------------------------------------
judeEnqueueKey:
;-----------------------------------------------------------
		sei

		ldy	keysIdx
		cpy	#0x10
		bcc	enqueue$

finish$:
		lda	karl_lock
		bne	exit$

	#ifndef DEBUG_NOYIELDIRQ
		cli
	#endif
exit$:
		rts

enqueue$:
		sta	keysBuffer0, y
		iny
		txa
		sta	keysBuffer0, y

		inc	keysIdx
		inc	keysIdx

		bra	finish$


;-----------------------------------------------------------
judeDequeueKey:
;-----------------------------------------------------------
		sei

		lda	#0x00
		sta	zp:zvalkey
		sta	zp:zvalkey + 1

		lda	keysIdx
		bne	dequeue$

finish$:
		lda	karl_lock
		bne	exit$

	#ifndef DEBUG_NOYIELDIRQ
		cli
	#endif
exit$:
		rts

dequeue$:
		lda	keysBuffer0
		sta	zp:zvalkey
		lda	keysBuffer0 + 1
		sta	zp:zvalkey + 1

		ldx	#0x00
loop$:
		lda	keysBuffer0 + 2, x
		sta	keysBuffer0, x

		inx
		cpx	#0x0E
		bne	loop$

		dec	keysIdx
		dec	keysIdx

		bra	finish$


;-----------------------------------------------------------
judeDefUIPrepare:
;-----------------------------------------------------------
		lda	#UINTERFACE__viewscnt
		sta	zp:zreg5b0

		lda	#UINTERFACE__views_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

    lda #0x02
    sta 0xd020

		lda	karl_errorno
		bne	exit$

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

exit$:
		rts


;-----------------------------------------------------------
judeDefUIInit:
;-----------------------------------------------------------
		MvDWObjImm	jude_mouseram, zp:zptrself, UINTERFACE__mouseloc
		MvDWObjImm	zp:zreg4, zptrself, UINTERFACE__mouseloc
		MvDWObjImm	jude_mouseptr, zp:zptrself, UINTERFACE__mptrloc

		ldz	#UINTERFACE__mousepal
		;nop
		lda	[zp:zptrself], z
		sta	jude_mousepal

		lda	zp:zreg4
		ora	zp:zreg4 + 1
		ora	zp:zreg4 + 2
		ora	zp:zreg4 + 3

		beq	cont6$

		ldz	#0x00
		ldy	#0x00
loop1$:
		lda	pointer0, y
		;nop
		sta	[zp:zreg4], z

		iny
		inz

		cpy	#0xA8
		bne	loop1$

		lda	#0xC0
		sta	zp:zregAb0
		lda	#0x00
		sta	zp:zregAb1
		sta	zp:zregAb2
		sta	zp:zregAb3

		LDQMem	zp:zreg4
		clc
		ADQMem	zp:zregA
		STQMem	zp:zreg4

		ldz	#0x00
		ldy	#0x00
loop2$:
		lda	pointer1, y
		;nop
		sta	[zp:zreg4], z

		iny
		inz

		cpy	#0xA8
		bne	loop2$


cont6$:
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefUIChange:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefUIRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
_judeViewPrepProcLayers:
;-----------------------------------------------------------
		lda	zp:zreg1wh
		beq	done$

		inw	zp:zreg1wl
		inw	zp:zreg1wl

;***FIXME  This is not using 16bit math

		ldz	#LAYER__width
		;nop
		lda	[zp:zreg0], z

		pha

		lda	zp:zreg1wh + 1
		cmp	#0x02
		bne	cont0$

		pla
		asl a
		bra	cont1$

cont0$:
		pla

cont1$:
		clc
		adc	zp:zreg1wl
		sta	zp:zreg1wl
		lda	#0x00
		adc	zp:zreg1wl + 1
		sta	zp:zreg1wl + 1

done$:
		inc	zp:zreg1wh

		rts


;-----------------------------------------------------------
judeDefViewPrepare:
;-----------------------------------------------------------
	#ifdef	DEBUG_RASTERTIME
		lda	#0x03
		sta	VIC_BRDRCLR
	#endif 

		lda	#VIEW__layerscnt
		sta	zp:zreg5b0

		lda	#VIEW__layers_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	karl_errorno
		beq	prep$

		rts

prep$:
		ldz	#VIEW__width
		;nop
		lda	[zp:zptrself], z

		sta	zp:zreg1wl

		lda	#0x00
		sta	zp:zreg1wl + 1
		sta	zp:zreg1wh

		ldz	#VIEW__cellsize
		;nop
		lda	[zp:zptrself], z

		sta	zp:zreg1wh + 1

		cmp	#0x02
		bne	cont0$

		ASLWMem	zp:zreg1wl

cont0$:
		lda	#VIEW__layerscnt
		sta	zp:zreg5b0

		lda	#VIEW__layers_p
		sta	zp:zreg5b2

		lda	#0x00
		sta	zp:zreg5b3

		lda	#.byte0 _judeViewPrepProcLayers
		sta	zp:zreg6wl
		lda	#.byte1 _judeViewPrepProcLayers
		sta	zp:(zreg6wl + 1)

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlProcObjLst

		ldz	#VIEW__layerscnt
		;nop
		lda	[zp:zptrself], z

		beq	done$

		cmp	#0x01
		beq	done$

		inw	zp:zreg1wl
		inw	zp:zreg1wl

done$:
		ldz	#VIEW__linelen
		lda	zp:zreg1wl
		;nop
		sta	[zp:zptrself], z
		inz
		lda	zp:(zreg1wl + 1)
		;nop
		sta	[zp:zptrself], z

		lda	#0x00
		sta	karl_errorno

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

	#ifdef	DEBUG_RASTERTIME
		lda	#0x04
		sta	VIC_BRDRCLR
	#endif

exit$:
		rts


;-----------------------------------------------------------
judeDefViewInit:
;-----------------------------------------------------------
		MvDWObjImm	jude_screenram, zp:zptrself, VIEW__location

		ldz	#VIEW__width
		;nop
		lda	[zp:zptrself], z

		cmp	#0x29
		bcs	dblw$

		lda	#0x28
		sta	jude_screenw
		bra	cont0$

dblw$:
		lda	#0x50
		sta	jude_screenw

cont0$:
		ldz	#VIEW__height
		;nop
		lda	[zp:zptrself], z

		cmp	#0x1A
		bcs	dblh$

		lda	#0x19
		sta	jude_screenh
		bra	cont1$

dblh$:
		lda	#0x32
		sta	jude_screenh

cont1$:
		ldz	#VIEW__cellsize
		;nop
		lda	[zp:zptrself], z

		beq	cell1$

		cmp	#0x01
		beq	cell1$

		lda	#0x02
		sta	jude_cellsize
		jmp	cont2$

cell1$:
		lda	#0x01
		sta	jude_cellsize

cont2$:
		ldz	#VIEW__linelen
		;nop
		lda	[zp:zptrself], z
		sta	jude_linesize
		inz
		;nop
		lda	[zp:zptrself], z
		sta	jude_linesize + 1

		MvDWMem	zp:zreg0, jude_screenram
		MvDWImm	zp:zreg2, VIC_CLRRAMH
		MvDWZ jude_screensize

		lda	#.byte0 jude_screeny0
		sta	zp:zreg1wl
		lda	#.byte1 jude_screeny0
		sta	zp:(zreg1wl + 1)

		lda	#.byte0 jude_coloury0
		sta	zp:zreg1wh
		lda	#.byte1 jude_coloury0
		sta	zp:(zreg1wh + 1)

		ldz	#0x00
		ldx	#0x00
loop0$:
		phx

		LDQMem	zp:zreg0
		STQIndW	zp:zreg1wl

		LDQMem	zp:zreg2
		STQIndW	zp:zreg1wh

		inw	zp:zreg1wl
		inw	zp:zreg1wl
		inw	zp:zreg1wl
		inw	zp:zreg1wl

		inw	zp:zreg1wh
		inw	zp:zreg1wh
		inw	zp:zreg1wh
		inw	zp:zreg1wh

		LDQMem	zp:zreg0
		clc
		ADQMem	jude_linesize
		STQMem	zp:zreg0

		LDQMem	zp:zreg2
		clc
		ADQMem	jude_linesize
		STQMem	zp:zreg2

		plx

		cpx	jude_screenh
		bcs	next0$

		phx

		LDQMem	jude_screensize
		clc
		ADQMem	jude_linesize
		STQMem	jude_screensize

		plx

next0$:
		inx
		cpx	#0x32
		bne	loop0$

;	Prevent VIC-II compatibility changes
		lda	#0x80
		trb	0xD05D

;	lowercase
		lda	0xD018
		and	#0xF1
    ora #0x06
		sta	0xD018

; xscrl
    lda 0xD016
    and #0xF8
    sta 0xD016

; texpos
		lda	#0x50
		sta	0xD04C

		lda	0xD04D
		and	#0xF0
		sta	0xD04D

;	Theme
		lda	#.byte0 CLR_EMPTY
		ldx	#.byte1 CLR_EMPTY
		jsr	_judeLogClrToSys
		sta	0xD020

		lda	#.byte0 CLR_BACK
		ldx	#.byte1 CLR_BACK
		jsr	_judeLogClrToSys
		sta	0xD021

;	Set the location of screen RAM
		lda	jude_screenram
		sta	0xD060
		lda	jude_screenram + 1
		sta	0xD061
		lda	jude_screenram + 2
		sta	0xD062

;	Set VIC to use 80/40 columns
		lda	jude_screenw
		cmp	#0x28
		bne	col80$

		lda	#0x80
		trb	0xD031

		bra	cont3$

col80$:
		lda	#0x80
		tsb	0xD031

;	Also, 80 columns is FCM/16 bit chars
		lda	#0x05
		tsb	0xD054


cont3$:
;	Set VIC to use 25/50 rows
		lda	jude_screenh
		cmp	#0x19
		bne	row50$

		lda	#0x08
		trb	0xD031
		bra	cont4$

row50$:
		lda	#0x08
		tsb	0xD031

cont4$:
;	Set VIC line size
		lda jude_linesize
		sta 0xD058
		lda jude_linesize + 1
		sta 0xD059

;	Set VIC window w
		lda	jude_screenw
		sta	0xD05E

;	CHRYSCL
		lda	#0x01
		sta	0xD05B

;	Set VIC window h
		lda	jude_screenh
		sta	0xD07B

;	Set VIC alpha compositing
		lda	#0x80
		tsb	0xD054

;	Use PALETTE RAM entries for colours 0 - 15
		lda	#0x04
		tsb	0xD030

		lda	jude_cellsize
		cmp	#0x02
		beq	cell16$

		lda	#0x01
		trb	0xD054
		bra	cont5$

cell16$:
		lda	#0x01
		tsb	0xD054

cont5$:
		lda	jude_screensize
		sta	zp:zreg4wl
		lda	jude_screensize + 1
		sta	zp:zreg4wl + 1

		dew	zp:zreg4wl
		dew	zp:zreg4wl

;	Size
		lda	zp:zreg4wl
		sta	dma_vw_siz
		lda	zp:(zreg4wl + 1)
		sta	dma_vw_siz + 1

;	Destination addr
		lda	#.byte0 (VIC_CLRRAMH + 2)
		sta	dma_vw_dadr
		lda	#.byte1 (VIC_CLRRAMH + 2)
		sta	dma_vw_dadr + 1

		lda	#.byte2 (VIC_CLRRAMH + 2)
		and	#0x0F
		sta	dma_vw_dbnk

		lda	#.byte2 (VIC_CLRRAMH + 2)
		lsr a
		lsr a
		lsr a
		lsr a
		sta	zp:zreg0b0

		lda	#.byte3 (VIC_CLRRAMH + 2)
		asl a
		asl a
		asl a
		asl a

		ora	zp:zreg0b0
		sta	dma_vw_dmb + 1

;	Source addr
		lda	#.byte0 (VIC_CLRRAMH)
		sta	dma_vw_sadr
		lda	#.byte1 (VIC_CLRRAMH)
		sta	dma_vw_sadr + 1

		lda	#.byte2 (VIC_CLRRAMH)
		and	#0x0F
		sta	dma_vw_sbnk

		lda	#.byte2 (VIC_CLRRAMH)
		lsr a
		lsr a
		lsr a
		lsr a
		sta	zp:zreg0b0

		lda	#.byte3 (VIC_CLRRAMH)
		asl a
		asl a
		asl a
		asl a

		ora	zp:zreg0b0
		sta	dma_vw_smb + 1

;	Source data
		MvDWImm	zp:zreg4, VIC_CLRRAMH

		lda	#.byte0 CLR_EMPTY
		ldx	#.byte1 CLR_EMPTY
		jsr	_judeLogClrToSys

		tax

		ldz	#0x00
		;nop
		sta	[zp:zreg4], z

		lda	jude_cellsize
		cmp	#0x02
		bne	clrsingle$

		lda	#0x00
		;nop
		sta	[zp:zreg4], z

		txa
		bra	clrplot2$

clrsingle$:
		txa

clrplot2$:
		inz
		;nop
		sta	[zp:zreg4], z

		lda	#0x00
		sta	0xD704
		sta	0xD702
		lda	#.byte1 dma_view_clear
		sta	0xD701
		lda	#.byte0 dma_view_clear
		sta	0xD705

		MvDWMem	zp:zreg4, jude_screenram

		inw	zp:zreg4wl
		bne	nohi0$

		inw	zp:zreg4wh

nohi0$:
		inw	zp:zreg4wl
		bne	nohi1$

		inw	zp:zreg4wh

nohi1$:
		lda	zp:zreg4
		sta	dma_vw_dadr
		lda	zp:(zreg4 + 1)
		sta	dma_vw_dadr + 1

		lda	zp:(zreg4 + 2)
		and	#0x0F
		sta	dma_vw_dbnk

		lda	zp:zreg4 + 2
		lsr a
		lsr a
		lsr a
		lsr a
		sta	zp:zreg0b0

		lda	zp:zreg4 + 3
		asl a
		asl a
		asl a
		asl a

		ora	zp:zreg0b0
		sta	dma_vw_dmb + 1

;	Source addr
		MvDWMem	zp:zreg4, jude_screenram

		lda	zp:zreg4
		sta	dma_vw_sadr
		lda	zp:zreg4 + 1
		sta	dma_vw_sadr + 1

		lda	zp:zreg4 + 2
		and	#0x0F
		sta	dma_vw_sbnk

		lda	zp:zreg4 + 2
		lsr a
		lsr a
		lsr a
		lsr a
		sta	zp:zreg0b0

		lda	zp:zreg4 + 3
		asl a
		asl a
		asl a
		asl a

		ora	zp:zreg0b0
		sta	dma_vw_smb + 1

;	Source data


		lda	#KEY_ASC_SPACE
		jsr	karlASCIIToScreen

		pha

		lda	#.byte0 CLR_EMPTY
		ldx	#.byte1 CLR_EMPTY
		jsr	_judeLogClrIsReverse

		bcc	text$

		pla
		ora	#0x80
		bra	clrscr$

text$:
		pla

clrscr$:
		sta	zp:zreg0b0

		ldz	#0x00
		;nop
		sta	[zp:zreg4], z

		lda	jude_cellsize
		cmp	#0x02
		bne	scrsingle$

		lda	#0x00
		bra	scrplot$

scrsingle$:
		lda	zp:zreg0b0

scrplot$:
		inz
		;nop
		sta	[zp:zreg4], z

		lda	#0x00
    sta 0xd704
		sta	0xD702
		lda	#.byte1 dma_view_clear
		sta	0xD701
		lda	#.byte0 dma_view_clear
		sta	0xD705

mouse$:
		lda	#MPTR_NONE
		sta	jude_mptrstate

;	Sprite disable
		lda	#0x00
		sta	0xD015

		MvDWMem	zp:zreg4, jude_mouseram

		lda	zp:zreg4
		ora	zp:zreg4 + 1
		ora	zp:zreg4 + 2
		ora	zp:zreg4 + 3

		beq	cont6$

		ldx	#0x05
loop1$:
		LSQMem	zp:zreg4
		dex
		bpl	loop1$

		jsr	_judeThemeSetMouse

		lda	#MPTR_NORMAL
		sta	jude_mptrstate

;	Sprite pointer location & 16bit
		lda	jude_mouseptr
		sta	0xD06C
		sta	zp:zreg3
		lda	jude_mouseptr + 1
		sta	0xD06D
		sta	zp:zreg3 + 1
		lda	jude_mouseptr + 2
		sta	zp:zreg3 + 2
		ora	#0x80
		sta	0xD06E
		lda	#0x00
		sta	zp:zreg3 + 3

;	Sprite pointer
		ldz	#0x00
		lda	zp:zreg4
		;nop
		sta	[zp:zreg3], z
		sta	jude_mousevic

		inz
		lda	zp:zreg4 + 1
		;nop
		sta	[zp:zreg3], z
		sta	jude_mousevic + 1

;	Sprite transparency colour
		lda	#0x00
		sta	0xD027

;	Sprite 16 colour
		lda #0x01
		tsb 0xD06B

;	Sprite 8 bytes wide
		tsb	0xD057

;***FIXME What was this, is it fixed?
;	Workaround BUG #339
		lda	#0x08
		trb	0xD07A
;---

;	Sprite enable
		lda	#0x01
		sta	0xD015

;	Sprite position
		jsr	_mouseMoveSprX
		jsr	_mouseMoveSprY

cont6$:
		lda	#VIEW__layerscnt
		sta	zp:zreg5b0

		lda	#VIEW__layers_p
		sta	zp:zreg5b2

		lda	#OBJECT__initialise
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	karl_errorno
		bne	exit$

		lda	#VIEW__barscnt
		sta	zp:zreg5b0

		lda	#VIEW__bars_p
		sta	zp:zreg5b2

		lda	#OBJECT__initialise
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	karl_errorno
		bne	exit$

		lda	#VIEW__pagescnt
		sta	zp:zreg5b0

		lda	#VIEW__pages_p
		sta	zp:zreg5b2

		lda	#OBJECT__initialise
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

exit$:
		rts


;-----------------------------------------------------------
judeDefViewChange:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefViewRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefLyrPrepare:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

		rts


;-----------------------------------------------------------
judeDefLyrInit:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefLyrChange:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefLyrRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPgePrepare:
;-----------------------------------------------------------
		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPgeInit:
;-----------------------------------------------------------
		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#OBJECT__initialise
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		rts


;-----------------------------------------------------------
judeDefPgeChange:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPgeRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPgePresent:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z
		and	#STATE_ACTIVE
		bne	present$

		rts

present$:
		ldz	#ELEMENT__colour + 1
		;nop
		lda	[zp:zptrself], z
		tax
		dez
		;nop
		lda	[zp:zptrself], z

		jsr	judeEraseBkg

		lda	#VIEW__barscnt
		sta	zp:zreg5b0

		lda	#VIEW__bars_p
		sta	zp:zreg5b2

		lda	#ELEMENT__present
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, jude_actvvw

		jsr	_karlCallObjLstMethod


		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#ELEMENT__present
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	#STATE_DIRTY
		jsr	karlObjExcludeState

		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPnlPrepare:
;-----------------------------------------------------------
		lda	#PANEL__controlscnt
		sta	zp:zreg5b0

		lda	#PANEL__controls_p
		sta	zp:zreg5b2

		lda	#OBJECT__prepare
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPnlInit:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPnlChange:
;-----------------------------------------------------------
		lda	#STATE_CHANGED
		jsr	karlObjExcludeState

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_DIRTY
		jsr	karlObjIncludeState

		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPnlRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefPnlPresent:
;-----------------------------------------------------------
		ldz	#ELEMENT__colour + 1
		;nop
		lda	[zp:zptrself], z
		tax
		dez
		;nop
		lda	[zp:zptrself], z

present$:
		jsr	judeEraseBkg

		lda	#PANEL__controlscnt
		sta	zp:zreg5b0

		lda	#PANEL__controls_p
		sta	zp:zreg5b2

		lda	#ELEMENT__present
		sta	zp:zreg5b1

		lda	#0x00
		sta	zp:zreg5b3

		MvDWMem	zp:zreg4, zp:zptrself

		jsr	_karlCallObjLstMethod

		lda	#STATE_DIRTY
		jsr	karlObjExcludeState

		lda	#0x00
		sta	karl_errorno

		rts




;-----------------------------------------------------------
_judeMvActvNextPanel:
;-----------------------------------------------------------
		ldz	#0x00
		LDQIndDWZ	zp:zregF
		STQMem	zp:zptrowner

		lda	#0x00
		sta	zp:zregDb1

		MvDWObjImm	zp:zregC, zp:zptrowner, PANEL__controls_p
		MvDWMem	zp:zregE, zp:zregC

		ldz	#PANEL__controlscnt
		;nop
		lda	[zp:zptrowner], z
		sta	zp:zregDb0

		beq	exit$

		lda	zp:zvalkey + 1
		and	#KEY_MOD_SHIFT
		beq	exit$

		ldx	zp:zregDb0
		dex
		stx	zp:zreg5b0
		lda	#0x00
		sta	zp:zreg5b1
		sta	zp:zreg5b2
		sta	zp:zreg5b3

		ASLWMem	zp:zreg5wl
		ASLWMem	zp:zreg5wl

		LDQMem	zp:zregC
		clc
		ADQMem	zp:zreg5
		STQMem	zp:zregE

		ldx	zp:zregDb0
		dex
		stx	zp:zregDb1

exit$:
		rts


;-----------------------------------------------------------
_judeMoveActiveControl:
;-----------------------------------------------------------
		lda	jude_actvpg
		ora	jude_actvpg + 1
		ora	jude_actvpg + 2
		ora	jude_actvpg + 3
		bne	begin$

		rts

begin$:
;***FIXME	Check if control is actually on a bar and if so,
; restart (set active control to zero?)

;	Get page's panels list
		MvDWMem	zp:zreg4, jude_actvpg
		MvDWObjImm	zp:zregA, zp:zreg4, PAGE__panels_p

		ldz	#PAGE__panelscnt
		;nop
		lda	[zp:zreg4], z
		sta	zp:zregBb0

;					zp:zregA	:	pointer to page's panels
;					zp:zregBb0	:	page panel count

		lda	judeActvElem
		ora	judeActvElem + 1
		ora	judeActvElem + 2
		ora	judeActvElem + 3
		lbeq	restart$

;	Get active element's panel
		MvDWMem	zp:zreg4, judeActvElem
		MvDWObjImm zp:zptrowner, zp:zreg4, ELEMENT__owner

;					zp:zreg4	:	active elem
;					zptrowner:	active panel

		lda	zp:zptrowner
		ora	zp:zptrowner + 1
		ora	zp:zptrowner + 2
		ora	zp:zptrowner + 3
		lbeq	restart$

;	Get active panel's controls
		MvDWObjImm zp:zregC, zp:zptrowner, PANEL__controls_p
		
		ldz	#PANEL__controlscnt
		;nop
		lda	[zp:zptrowner], z
		sta	zp:zregDb0

;					zp:zregC	:	pointer to panel's controls
;					zp:zregDb0	:	panel control count

;	Find active control index
		lda	#0x00
		sta	zp:zregDb1

		MvDWMem	zp:zregE, zp:zregC

loopfac$:
		ldz	#0x00
		LDQIndDWZ	zp:zregE
		CPQMem	zp:zreg4
		beq	foundfac$

		LDQMem	zp:zregE
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregE

		inc	zp:zregDb1
		lda	zp:zregDb1
		cmp	zp:zregDb0
		bcc	loopfac$

		bra	restart$

foundfac$:

;					zp:zregE	:	pointer to control in list
;					zp:zregDb1	:	control list index

;	Find active panel index
		lda	#0x00
		sta	zp:zregDb2

		MvDWMem	zp:zregF, zp:zregA

loopfap$:
		ldz	#0x00
		LDQIndDWZ	zp:zregF
		CPQMem	zp:zptrowner
		beq	foundfap$

		LDQMem	zp:zregF
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregF

		inc	zp:zregDb2
		lda	zp:zregDb2
		cmp	zp:zregBb0
		bcc	loopfap$

		bra	restart$

foundfap$:

;					zp:zregF	:	pointer to panel in list
;					zp:zregDb2	:	panel list index
		lda	zp:zregDb2
		sta	zp:zregBb1

		lda	zp:zregDb1
		sta	zp:zregBb2

		lbra	nextelem$

restart$:
		lda	#0x00
		sta	zp:zregBb1
		sta	zp:zregBb2

;					zp:zregBb1:	start panel index
;					zp:zregBb2:	start control index

;					zp:zregA	:	pointer to page's panels
;					zp:zregBb0	:	page panel count
		MvDWMem	zp:zregF, zp:zregA
		lda	#0x00
		sta	zp:zregDb2

		lda	zp:zvalkey + 1
		and	#KEY_MOD_SHIFT
		beq	rescont0$

		ldx	zp:zregBb0
		dex
		stx	zp:zreg5b0
		lda	#0x00
		sta	zp:zreg5b1
		sta	zp:zreg5b2
		sta	zp:zreg5b3

		ASLWMem	zp:zreg5wl
		ASLWMem	zp:zreg5wl

		LDQMem	zp:zregF
		clc
		ADQMem	zp:zreg5
		STQMem	zp:zregF

		ldx	zp:zregBb0
		dex
		stx	zp:zregDb2

;					zp:zregF	:	pointer to panel in list
;					zp:zregDb2	:	panel list index

rescont0$:
		jsr	_judeMvActvNextPanel

;					zptrowner:	active panel
;					zp:zregC	:	pointer to panel's controls
;					zp:zregDb0	:	panel control count
;					zp:zregE	:	pointer to control in list
;					zp:zregDb1	:	control list index
		lda	zp:zregDb0
		bne	rescont1$

		lbra	nextpanel$

rescont1$:
		ldz	#0x00
		LDQIndDWZ	zp:zregE
		STQMem	zp:zreg4

;					zp:zreg4	:	active elem

start$:
		lda	zp:zregDb2
		sta	zp:zregBb1

		lda	zp:zregDb1
		sta	zp:zregBb2

;					zp:zregBb1:	start panel index
;					zp:zregBb2:	start control index

testelem$:
;	Must be state PREPARED | VISIBLE | ENABLED
		ldz	#OBJECT__state
		;nop
		lda	[zp:zreg4], z
		and	#(STATE_VISIBLE | STATE_ENABLED)
		cmp	#(STATE_VISIBLE | STATE_ENABLED)
		bne	nextelem$

;	and otherwise navagable
		ldz	#OBJECT__options
		;nop
		lda	[zp:zreg4], z

		and	#OPT_NONAVIGATE
		bne	nextelem$

activelem$:
		MvDWMem	zp:zptrself, zp:zreg4
		jsr	judeActivateCtrl

		rts

nextelem$:
		lda	zp:zvalkey + 1
		and	#KEY_MOD_SHIFT
		bne	nxtelemup$

		inc	zp:zregDb1
		lda	zp:zregDb1
		cmp	zp:zregDb0
		beq	nextpanel$

		LDQMem	zp:zregE
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregE

		bra	nxtelemfetch$

nxtelemup$:
		dec	zp:zregDb1
		lda	zp:zregDb1
		cmp	#0xFF
		beq	nextpanel$

		LDQMem	zp:zregE
		sec
		SBQMem	zvaltemp0
		STQMem	zp:zregE


nxtelemfetch$:
		ldz	#0x00
		LDQIndDWZ	zp:zregE
		STQMem	zp:zreg4

;					zp:zregBb1:	start panel index
;					zp:zregBb2:	start control index
;					zp:zregDb2	:	panel list index
;					zp:zregDb1	:	control list index
		lda	zp:zregBb1
		cmp	zp:zregDb2
		bne	testelem$

		lda	zp:zregBb2
		cmp	zp:zregDb1
		bne	testelem$

		rts					;ABORT

nextpanel$:
		lda	zp:zvalkey + 1
		and	#KEY_MOD_SHIFT
		bne	nxtpnlup$

		inc	zp:zregDb2
		lda	zp:zregDb2
		cmp	zp:zregBb0
		beq	nxtpnldnwrap$

		LDQMem	zp:zregF
		clc
		ADQMem	zp:zvaltemp0
		STQMem	zp:zregF

		bra	nxtpnlfetch$

nxtpnldnwrap$:
		MvDWMem	zp:zregF, zp:zregA

		lda	#0x00
		sta	zp:zregDb2

		bra	nxtpnlfetch$

nxtpnlup$:
		dec	zp:zregDb2
		lda	zp:zregDb2
		cmp	#0xFF
		beq	nxtpnlupwrap$

		LDQMem	zp:zregF
		sec
		SBQMem	zvaltemp0
		STQMem	zp:zregF

		bra	nxtpnlfetch$

nxtpnlupwrap$:
		ldx	zp:zregBb0
		dex
		stx	zp:zreg5b0
		lda	#0x00
		sta	zp:zreg5b1
		sta	zp:zreg5b2
		sta	zp:zreg5b3

		ASLWMem	zp:zreg5wl
		ASLWMem	zp:zreg5wl

		LDQMem	zp:zregA
		clc
		ADQMem	zp:zreg5
		STQMem	zp:zregF

nxtpnlfetch$:
		jsr	_judeMvActvNextPanel

;					zp:zregBb1:	start panel index
;					zp:zregBb2:	start control index
;					zp:zregDb2	:	panel list index
;					zp:zregDb1	:	control list index

		lda	zp:zregDb0
		bne	nxtpnlcont0$

		lda	zp:zregDb2
		cmp	zp:zregBb1
		LBNE	nextpanel$

		rts				;Abort

nxtpnlcont0$:
		ldz	#0x00
		LDQIndDWZ	zp:zregE
		STQMem	zp:zreg4

		lbra	testelem$


;					zp:zregA	:	pointer to page's panels
;					zp:zregBb0	:	page panel count
;					zp:zregF	:	pointer to panel in list
;					zp:zregDb2	:	panel list index
;					zptrowner:	active panel
;					zp:zregC	:	pointer to panel's controls
;					zp:zregDb0	:	panel control count
;					zp:zregE	:	pointer to control in list
;					zp:zregDb1	:	control list index


;-----------------------------------------------------------
_judeSendKeys:
;-----------------------------------------------------------
		jsr	judeDequeueKey
		lda	zp:zvalkey
		bne	input$

exit$:
		rts

input$:
		;MvDWMem	zp:zreg4, jude_screeny0
		;lda	zp:zvalkey
		;ldz	#0x00
		;sta	[zp:zreg4], z
		;inz
		;inz
		;lda	zp:zvalkey + 1
		;sta	[zp:zreg4], z

		lda	zp:zvalkey + 1
		and	#KEY_MOD_SYS
		beq	tstfkeys$

		jmp	findaccel$

tstfkeys$:
		lda	zp:zvalkey
		cmp	#KEY_M65_F1
		bcs	fkey0$
	
		cmp	#KEY_M65_ESC
		lbeq findaccel$

		bra	isdownctrl$

fkey0$:
		cmp	#(KEY_M65_F14 + 1)	
		lbcs	isdownctrl$

		lda	zp:zvalkey + 1
		and	#KEY_MOD_SHIFT
		beq	fkeycont$

		inc	zp:zvalkey

fkeycont$:
		jmp	findaccel$

isdownctrl$:
		lda	judeDownElem
		ora	judeDownElem + 1
		ora	judeDownElem + 2
		ora	judeDownElem + 3
		bne	downctrl$

		lda	judeActvElem
		ora	judeActvElem + 1
		ora	judeActvElem + 2
		ora	judeActvElem + 3
		bne	actvctrl$

		bra	page0$

actvctrl$:
		MvDWMem	zp:zptrself, judeActvElem

		ldz	#OBJECT__options + 1
		;nop
		lda	[zp:zptrself], z
		and	#.byte1 OPT_AUTOTRACK
		beq	chkmv$

		bra	send$

chkmv$:
		lda	zp:zvalkey
		cmp	#KEY_C64_CDOWN
		beq	moveactv$

		cmp	#KEY_C64_CDOWN | 0x80
		beq	moveactv$

		cmp	#KEY_C64_CRIGHT
		beq	moveactv$

		cmp	#KEY_C64_CRIGHT | 0x80
		beq	moveactv$

		cmp	#KEY_M65_TAB
		beq	moveactv$

    cmp #KEY_M65_SHTAB
    beq mungetab$

		lda	zp:zvalkey
		cmp	#KEY_ASC_CR
		bne	send$

		jsr	judeDownCtrl
		jmp	_judeSendKeys

mungetab$:
    lda #KEY_M65_TAB
    sta zp:zvalkey

moveactv$:
		jsr	_judeMoveActiveControl
		jmp	_judeSendKeys


downctrl$:
		MvDWMem	zp:zptrself, judeDownElem

send$:
		ldz	#ELEMENT__keypress
		;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr
		inz
    ;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr + 1

		beq	page0$

		ldz	#0x00
		jsr	_judeProxy

;***FIXME!!! Check return from keypress and send keys to parent
;loop if not initial control not down and return not abort instead
;of this hack
page0$:
		lda	karl_errorno
		cmp	#ERROR_ABORT
		beq	def$

		lda	jude_actvpg
		ora	jude_actvpg + 1
		ora	jude_actvpg + 2
		ora	jude_actvpg + 3
		beq	def$

		MvDWMem	zp:zptrself, jude_actvpg

		ldz	#ELEMENT__keypress
		;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr
		inz
    ;nop
		lda	[zp:zptrself], z
		sta	jude_proxyptr + 1

		beq	def$

		ldz	#0x00
		jsr	_judeProxy

discard0$:
		jmp	_judeSendKeys

def$:
		jmp	_judeSendKeys


findaccel$:
		jsr	_judeProcessAccelerators

		rts


;-----------------------------------------------------------
_judeProcVwElemsAccel:
;-----------------------------------------------------------
		lda	#ERROR_NONE
		sta	karl_errorno

    lda zp:zptrtemp2 + 1
    beq elem$

		ldz	#OBJECT__state
		lda	[zp:zreg0], z

		and	zp:zptrtemp2 + 1
    cmp zp:zptrtemp2 + 1
		bne	exit$

elem$:
		ldz	#CONTROL__accelchar
		lda	[zp:zreg0], z

    beq exit$

;cont$:
		cmp	zp:zvalkey
		bne	exit$

		MvDWMem	zp:zptrself, zp:zreg0
		jsr	judeDownCtrl 

		lda	#ERROR_ABORT
		sta	karl_errorno

exit$:
		rts


;-----------------------------------------------------------
_judeProcessAccelerators:
;-----------------------------------------------------------
    ;lda zvalkey
    ;and #0x7f
    ;sta zvalkey

		lda	#.byte0 _judeProcVwElemsAccel
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsAccel
		sta	zp:zreg6wl + 1

		lda	#STATE_VISIBLE | STATE_ENABLED
		sta	zp:zptrtemp2 + 1

		jsr	_judeProcViewElements

		rts


;-----------------------------------------------------------
_judeUpdateChanged:
;-----------------------------------------------------------
		lda	#.byte0 _judeProcVwElemsUpdate
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsUpdate
		sta	zp:zreg6wl + 1

		lda	#OBJECT__change
		sta	zp:zptrtemp2

		lda	#STATE_CHANGED
		sta	zp:zptrtemp2 + 1

		jsr	_judeProcViewElements

		rts


;-----------------------------------------------------------
_judeProcVwElemsPanels:
;-----------------------------------------------------------
    lda zp:zptrtemp2 + 1
    beq panel$
    
    ldz	#OBJECT__state
		;nop
		lda	[zp:zreg0], z

		and	zp:zptrtemp2 + 1
    cmp zp:zptrtemp2 + 1
		bne	controls$

panel$:
		lda	zp:zreg3wh
		sta	jude_proxyptr
		lda	zp:zreg3wh + 1
		sta	jude_proxyptr + 1

		ldz	#0x00
		jsr	_judeProxy

controls$:
		MvDWMem	zp:zreg4, zp:zreg0

		lda	#PANEL__controlscnt
		sta	zp:zreg5b0

		lda	#PANEL__controls_p
		sta	zp:zreg5b2

		lda	#0x00
		sta	zp:zreg5b3

		lda	zp:zreg3wh
		sta	zp:zreg6wl
		lda	zp:zreg3wh + 1
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

exit$:
		rts


;-----------------------------------------------------------
_judeProcVwElemsPages:
;-----------------------------------------------------------
		lda zp:zptrtemp2 + 1
    beq page$
    
    ldz	#OBJECT__state
		;nop
		lda	[zp:zreg0], z

		and	zp:zptrtemp2 + 1
    cmp zp:zptrtemp2 + 1
		bne	panels$

page$:
		lda	zp:zreg3wh
		sta	jude_proxyptr
		lda	zp:zreg3wh + 1
		sta	jude_proxyptr + 1

		ldz	#0x00
		jsr	_judeProxy

panels$:
		MvDWMem	zp:zreg4, zp:zreg0

		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#0x00
		sta	zp:zreg5b3

		lda	#.byte0 _judeProcVwElemsPanels
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsPanels
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

exit$:
		rts


;-----------------------------------------------------------
_judeProcViewElements:
;-----------------------------------------------------------
;	zp:zreg6wl		IN		callback routine
;-----------------------------------------------------------
		lda	jude_actvvw
		ora	jude_actvvw + 1
		ora	jude_actvvw + 2
		ora	jude_actvvw + 2
		bne	begin$

		rts

begin$:
		lda	zp:zreg6wl
		sta	zp:zreg3wh
		lda	zp:zreg6wl + 1
		sta	zp:zreg3wh + 1

		MvDWMem	zp:zreg4, jude_actvvw

		lda	#VIEW__barscnt
		sta	zp:zreg5b0

		beq  present$

		lda	#VIEW__bars_p
		sta	zp:zreg5b2

		lda	#0x00
		sta	zp:zreg5b3

		lda	#.byte0 _judeProcVwElemsPanels
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsPanels
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

;	Process the active page only
present$:
		lda	jude_actvpg
		ora	jude_actvpg + 1
		ora	jude_actvpg + 2
		ora	jude_actvpg + 3
		bne	cont0$

		rts

cont0$:
		MvDWMem	zp:zreg0, jude_actvpg

    lda zp:zptrtemp2 + 1
    beq page$

		ldz	#OBJECT__state
		;nop
		lda	[zp:zreg0], z

		and	zp:zptrtemp2 + 1
    cmp zp:zptrtemp2 + 1
		bne	panels$

page$:
		lda	zp:zreg3wh
		sta	jude_proxyptr
		lda	zp:zreg3wh + 1
		sta	jude_proxyptr + 1

		ldz	#0x00
		jsr	_judeProxy

panels$:
		MvDWMem	zp:zreg4, jude_actvpg

		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#0x00
		sta	zp:zreg5b3

		lda	#.byte0 _judeProcVwElemsPanels
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsPanels
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

exit$:
		rts

;-----------------------------------------------------------
_judeProcVwElemsUpdate:
;-----------------------------------------------------------
		lda zp:zptrtemp2 + 1
    beq elems$
    
    ldz	#OBJECT__state
		;nop
		lda	[zp:zreg0], z

		and	zp:zptrtemp2 + 1
    cmp zp:zptrtemp2 + 1
		bne	exit$

elems$:
		MvDWMem	zp:zptrself, zp:zreg0

		;ldz	zp:zptrtemp2
    lda zp:zptrtemp2
    taz

		;nop
		lda	[zp:zreg0], z
		sta	jude_proxyptr
		inz
		;nop
		lda	[zp:zreg0], z
		sta	jude_proxyptr + 1

		beq	exit$

		ldz	#0x00
		jsr	_judeProxy

exit$:
		rts

;-----------------------------------------------------------
_judePresentDirty:
;-----------------------------------------------------------
		lda	#.byte0 _judeProcVwElemsUpdate
		sta	zp:zreg6wl
		lda	#.byte1 _judeProcVwElemsUpdate
		sta	zp:zreg6wl + 1

		lda	#ELEMENT__present
		sta	zp:zptrtemp2

		lda	#STATE_DIRTY
		sta	zp:zptrtemp2 + 1

		jsr	_judeProcViewElements

		rts


;-----------------------------------------------------------
_judeThemeSetMouse:
;-----------------------------------------------------------
		lda	#0xC0
		tsb	0xD070

		lda	#.byte0 CLR_CURSOR
		ldx	#.byte1 CLR_CURSOR
		jsr	_judeLogClrToSys

		tax
		lda	0xD100, x
		sta	zp:zreg3b0
		lda	0xD200, x
		sta	zp:zreg3b1
		lda	0xD300, x
		sta	zp:zreg3b2

		lda	0xD070
		and	#0x3F
		sta	0xD070

		lda	jude_mousepal
		asl a
		asl a
		asl a
		asl a
		asl a
		asl a
		tsb	0xD070

		lda	#0x00
		sta	0xD100
		sta	0xD102
		sta	0xD200
		sta	0xD202
		sta	0xD300
		sta	0xD302

		lda	zp:zreg3b0
		sta	0xD101
		lda	zp:zreg3b1
		sta	0xD201
		lda	zp:zreg3b2
		sta	0xD301

		lda	#0x08
		sta	0xD103
		sta	0xD203
		sta	0xD303

		lda	#0xFF
		sta	0xD104
		sta	0xD204
		sta	0xD304

		lda	0xD070
		and	#0xF3
		sta	0xD070

		lda	jude_mousepal
		asl a
		asl a
		tsb	0xD070

;	Set the mapped palette to the one used by the chars so
;	that m65 will get the proper screen shot
		lda	#0xC0
		tsb	0xD070

		rts


;-----------------------------------------------------------
_mouseProcessClick:
;-----------------------------------------------------------
		lda	mouseBtnLClick
		beq	exit$

		lda	#0x00
		sta	mouseBtnLClick

		lda	mouseCapture
		beq	norm$

		jmp	(mouseCapClick)

norm$:
		lda	judePickElem
		ora	judePickElem + 1
		ora	judePickElem + 2
		ora	judePickElem + 3
		bne	down$

		rts

down$:
		MvDWMem	zp:zptrself, judePickElem

		jsr	judeDownCtrl

exit$:
		rts


;-----------------------------------------------------------
_mouseUnPickElement:
;-----------------------------------------------------------
		lda	judePickElem
		ora	judePickElem + 1
		ora	judePickElem + 2
		ora	judePickElem + 3
		beq	exit$

		MvDWMem	zp:zptrself, judePickElem

		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z

		and	#STATE_PICKED
		beq	clear$

		lda	#STATE_PICKED
		jsr	karlObjExcludeState

clear$:
		MvDWZ	judePickElem

exit$:
		rts


;-----------------------------------------------------------
_mousePickElement:
;-----------------------------------------------------------
		lda	judePickElem
		cmp judeMsePElem
		bne	update$

		lda	judePickElem + 1
		cmp judeMsePElem + 1
		bne	update$

		lda	judePickElem + 2
		cmp judeMsePElem + 2
		bne	update$

		lda	judePickElem + 3 
		cmp judeMsePElem + 3
		bne	update$

		rts

update$:
		jsr	_mouseUnPickElement

		MvDWMem	zp:zptrself, judeMsePElem
		MvDWMem	judePickElem, zp:zptrself

;		lda	#0x00
;		sta	zp:zreg2b3
		lda	#STATE_PICKED
		jsr	karlObjIncludeState

exit$:
		rts


;-----------------------------------------------------------
_mouseInElement:
;-----------------------------------------------------------
		ldz	#ELEMENT__posy
		;nop
		lda	[zp:zptrself], z
		sta	zp:zptrtemp0

		lda	mouseYRow
		cmp	zp:zptrtemp0
		bcs	testh$

		bra	nomatch$

testh$:
		ldz	#ELEMENT__height
		;nop
		lda	[zp:zptrself], z

		clc
		adc	zp:zptrtemp0
		sta	zp:zptrtemp0

		lda	mouseYRow
		cmp	zp:zptrtemp0
		bcs	nomatch$

		ldz	#ELEMENT__posx
		;nop
		lda	[zp:zptrself], z
		sta	zp:zptrtemp0

		lda	mouseXCol
		cmp	zp:zptrtemp0
		bcs	testw$

nomatch$:
		clc
		rts

testw$:
		ldz	#ELEMENT__width
		;nop
		lda	[zp:zptrself], z

		clc
		adc	zp:zptrtemp0
		sta	zp:zptrtemp0

		lda	mouseXCol
		cmp	zp:zptrtemp0
		bcs	nomatch$

		sec
		rts


;-----------------------------------------------------------
_mousePickBlink:
;-----------------------------------------------------------
		ldy	judePBlinkDelay
		beq	blink$

		dey
		sty	judePBlinkDelay
		
		rts

blink$:
		ldy	#0x14
		sty	judePBlinkDelay

		MvDWMem	zp:zptrself, judePickElem

		lda	judePBlinkState
		eor	#0x01
		sta	judePBlinkState

		beq	exclude$

;		lda	#0x00
;		sta	zp:zreg2b3
		lda	#STATE_PICKED
		jsr	karlObjIncludeState
		rts

exclude$:
		lda	#STATE_PICKED
		jsr	karlObjExcludeState

		rts


;-----------------------------------------------------------
_mouseProcessPickControls:
;-----------------------------------------------------------
		MvDWMem	zp:zptrself, zp:zreg0

		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z
		bit	#STATE_VISIBLE
		bne	cont0$

		rts

cont0$:
		bit	#STATE_ENABLED
		bne	cont1$

		rts

cont1$:
		ldz	#OBJECT__options
		;nop
		lda	[zp:zptrself], z
		and	#OPT_NONAVIGATE
		beq	cont2$

		rts

cont2$:
		jsr	_mouseInElement
		bcs	cont3$

		rts

cont3$:
		MvDWMem	judeMsePElem, zp:zptrself

		lda	#ERROR_ABORT
		sta	karl_errorno

		rts

;-----------------------------------------------------------
_mouseProcessPickPanels:
;-----------------------------------------------------------
		MvDWMem	zp:zptrself, zp:zreg0

		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], z
		bit	#STATE_VISIBLE
		bne	cont0$

		rts

cont0$:
		bit	#STATE_ENABLED
		bne	cont1$

		rts

cont1$:
		ldz	#OBJECT__options
		;nop
		lda	[zp:zptrself], z
		and	#OPT_NONAVIGATE
		beq	cont2$

		rts

cont2$:
		jsr	_mouseInElement
		bcs	cont3$

		rts

cont3$:
		MvDWMem	zp:zreg4, zp:zreg0

		lda	#PANEL__controlscnt
		sta	zp:zreg5b0

		lda	#PANEL__controls_p
		sta	zp:zreg5b2

		lda	#0x01
		sta	zp:zreg5b3

		lda	#.byte0 _mouseProcessPickControls
		sta	zp:zreg6wl
		lda	#.byte1 _mouseProcessPickControls
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

		rts


;;-----------------------------------------------------------
; __MouseProcessPickPages:
;;-----------------------------------------------------------
;		MvDWMem	zp:zptrself, zp:zreg0
;
;		ldz	#OBJECT__state
;		nop
;		lda	(zp:zptrself), z
;		and	#STATE_VISIBLE
;		bne	cont0$
;
;		rts
;
; cont0$:
;		nop
;		lda	(zp:zptrself), z
;		and	#STATE_ENABLED
;		bne	cont1$
;
;		rts
;
; cont1$:
;		ldz	#OBJECT__options
;		nop
;		lda	(zp:zptrself), z
;		and	#OPT_NONAVIGATE
;		beq	cont2$
;
;		rts
;
; cont2$:
;		jsr	_mouseInElement
;		bcs	cont3$
;
;		rts
;
; cont3$:
;		MvDWMem	zp:zreg4, zp:zreg0
;
;		lda	#PAGE__panelscnt
;		sta	zp:zreg5b0
;
;		lda	#PAGE__panels_p
;		sta	zp:zreg5b2
;
;		lda	#0x01
;		sta	zp:zreg5b3
;
;		lda	#<_MouseProcessPickPanels
;		sta	zp:zreg6wl
;		lda	#>_MouseProcessPickPanels
;		sta	zp:zreg6wl + 1
;
;		jsr	_karlProcObjLst
;
;		rts


;-----------------------------------------------------------
_mouseProcessMouse:
;-----------------------------------------------------------
		MvDWMem	zp:zreg4, jude_actvvw
		lda	zp:zreg4
		ora	zp:zreg4 + 1
		ora	zp:zreg4 + 2
		ora	zp:zreg4 + 3
		bne	begin$

exit$:
		rts

begin$:
		inc	mouseCheck

		lda	jude_mptrstate
		cmp	#MPTR_NORMAL
		bne	exit$

		lda	mouseBtnLClick
		lbne	proc$

		lda	mouseCheck
		cmp	#0x10
		bcs	proc$

		lda	mouseCapture
		beq	tstpick$

		rts

tstpick$:
		lda	judePickElem
		ora	judePickElem + 1
		ora	judePickElem + 2
		ora	judePickElem + 3

		bne	tstblink$
		rts

tstblink$:
		lda	judePickElem
		cmp judeDownElem
		bne	blink$

		lda	judePickElem + 1
		cmp judeDownElem + 1
		bne	blink$

		lda	judePickElem + 2
		cmp judeDownElem + 2
		bne	blink$

		lda	judePickElem + 3 
		cmp judeDownElem + 3
		bne	blink$

		rts

blink$:
		jsr	_mousePickBlink
		rts

proc$:
		lda	#0x00
		sta	mouseCheck

		lda	mouseXPos
		sta	mouseTemp0
		lda	mouseXPos + 1
		sta	mouseTemp0 + 1
		
		LSRWMem	mouseTemp0
		LSRWMem	mouseTemp0
		
		lda	jude_avhrsx
		bne	cont0$

		LSRWMem	mouseTemp0

cont0$:
		lda	mouseTemp0
		sta	mouseXCol
		
		lda	mouseYPos
		sta	mouseTemp0
		lda	mouseYPos + 1
		sta	mouseTemp0 + 1

		LSRWMem	mouseTemp0
		LSRWMem	mouseTemp0
		
		lda	jude_avhrsy
		bne	cont1$

		LSRWMem	mouseTemp0
		
cont1$:
		lda	mouseTemp0
		sta	mouseYRow

		lda	mouseCapture
		beq	findctrl$
		
		jmp	(mouseCapMove)

findctrl$:
		MvDWZ	judeMsePElem

		MvDWMem	zp:zreg4, jude_actvpg

		lda	#PAGE__panelscnt
		sta	zp:zreg5b0

		lda	#PAGE__panels_p
		sta	zp:zreg5b2

		lda	#0x01
		sta	zp:zreg5b3

		lda	#.byte0 _mouseProcessPickPanels
		sta	zp:zreg6wl
		lda	#.byte1 _mouseProcessPickPanels
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

		lda	judeMsePElem
		ora	judeMsePElem + 1
		ora	judeMsePElem + 2
		ora	judeMsePElem + 3
		bne	dopick$

		MvDWMem	zp:zreg4, jude_actvvw

		lda	#VIEW__barscnt
		sta	zp:zreg5b0

		lda	#VIEW__bars_p
		sta	zp:zreg5b2

		lda	#0x01
		sta	zp:zreg5b3

		lda	#.byte0 _mouseProcessPickPanels
		sta	zp:zreg6wl
		lda	#.byte1 _mouseProcessPickPanels
		sta	zp:zreg6wl + 1

		jsr	_karlProcObjLst

		lda	judeMsePElem
		ora	judeMsePElem + 1
		ora	judeMsePElem + 2
		ora	judeMsePElem + 3
		beq	unpick$

dopick$:
		lda	judePickElem
		cmp	judeMsePElem
		bne	newpick$
		lda	judePickElem + 1
		cmp	judeMsePElem + 1
		bne	newpick$
		lda	judePickElem + 2
		cmp	judeMsePElem + 2
		bne	newpick$
		lda	judePickElem + 3
 		cmp	judeMsePElem + 3
		bne	newpick$

		jsr	_mousePickBlink
		rts

newpick$:
		lda	#0x14
		sta	judePBlinkDelay
		lda	#0x01
		sta	judePBlinkState

		jsr	_mousePickElement
		rts
		
unpick$:
		jsr	_mouseUnPickElement

		rts


;-----------------------------------------------------------
_mouseInputMouse:
;-----------------------------------------------------------
begin$:
		ldy	#0b00000000		    ;Set ports A and B to input
		sty	CIA1_DDRB
		sty	CIA1_DDRA			;Keyboard won't look like mouse
		lda	CIA1_PRB			 ;Read Control-Port 1
		dec	CIA1_DDRA			;Set port A back to output
		eor	#0b11111111		    ;Bit goes up when button goes down
		sta	mouseButtons
		beq	L0$				 ;(bze)
		dec	CIA1_DDRB			;Mouse won't look like keyboard
		sty	CIA1_PRB			 ;Set "all keys pushed"

L0$:    
		jsr	_mouseButtonCheck

    rts



		lda	SID_ADCONV1		   ;Get mouse X movement

		ldy	mouseOldPotX
		jsr	moveCheck			;Calculate movement vector
		sty	mouseOldPotX

; Skip processing if nothing has changed

		bcc	SkipX$

; Calculate the new X coordinate (--> a/y)
		asl a
		pha
		txa
		rol a
		tax
		pla

		clc
		adc	mouseXPosNew

		tay					    ;Remember low byte
		txa
		adc	mouseXPosNew+1
		tax

; Limit the X coordinate to the bounding box

		cpy	mouseXMin
		sbc	mouseXMin+1
		bpl	L1$
		ldy	mouseXMin
		ldx	mouseXMin+1
		bra	L2$
L1$:    	
		txa

		cpy	mouseXMax
		sbc	mouseXMax+1
		bmi	L2$
		ldy	mouseXMax
		ldx	mouseXMax+1
L2$:    
		sty	mouseXPosNew
    sty mouseXPos
	  stx	mouseXPosNew+1
    stx mouseXPos + 1

; Move the mouse pointer to the new X pos

		tya
		jsr	_mouseMoveSprX
		
; Calculate the Y movement vector

SkipX$: 
		lda	SID_ADCONV2		   ;Get mouse Y movement
		ldy	mouseOldPotY
		jsr	moveCheck			;Calculate movement
		sty	mouseOldPotY

; Skip processing if nothing has changed

		bcc	SkipY$

; Calculate the new Y coordinate (--> a/y)

		asl a
		pha
		txa
		rol a
		tax
		pla

		sta	mouseOldValue
		lda	mouseYPosNew
		sec
		sbc	mouseOldValue

		tay
		stx	mouseOldValue
		lda	mouseYPosNew+1
		sbc	mouseOldValue
		tax

; Limit the Y coordinate to the bounding box

		cpy	mouseYMin
		sbc	mouseYMin+1
		bpl	L3$
		ldy	mouseYMin
		ldx	mouseYMin+1
		bra	L4$
L3$:    
		txa

		cpy	mouseYMax
		sbc	mouseYMax+1
		bmi	L4$
		ldy	mouseYMax
		ldx	mouseYMax+1
L4$:    	
		sty	mouseYPosNew
		sty mouseYPos
    stx	mouseYPosNew+1
    stx mouseYPos + 1

; Move the mouse pointer to the new Y pos

		tya
		jsr	_mouseMoveSprY

;		lda	mouseCheck
;		bne	SkipY$
;		
;		lda	#0x01
;		sta	mouseCheck

; Done

SkipY$: 
		rts

;-----------------------------------------------------------
moveCheck:
; Move check routine, called for both coordinates.
;
; Entry:        y = old value of pot register
;               a = current value of pot register
; Exit:         y = value to use for old value
;               x/a = delta value for position
;-----------------------------------------------------------
		sty     mouseOldValue
		sta     mouseNewValue
		ldx     #0x00

		sec				; a = mod64 (new - old)
		sbc	mouseOldValue

	cmp #0x3f
	bcs notPositiveMovement$
	ldy mouseNewValue
	ldx #0
		sec
	rts
notPositiveMovement$:
	cmp #0xc0
	bcc notNegativeMovement$
		ldy     mouseNewValue
	ldx #0xff
		sec
		rts

notNegativeMovement$:
	ldy mouseNewValue
		txa                             ; A = 0x00
		clc
		rts


;-----------------------------------------------------------
_mouseButtonCheck:
;-----------------------------------------------------------

		lda	mouseButtons			;mouseButtons still the same as last
		cmp	mouseButtonsOld		;time?
		beq	done$			;Yes - don't do anything here
		
		and	#MOUSE_LBTN		;No - Is left button down?
		bne	testRight$		;Yes - test right
		
		lda	mouseButtonsOld		;No, but was it last time?
		and	#MOUSE_LBTN
		beq	testRight$		;No - test right
		
		lda	#0x01			;Yes - flag have left click
		sta	mouseBtnLClick
		
testRight$:
		bit	#MOUSE_RBTN		;Is right button down?
		bne	done$			;Yes - don't do anything here
		
		lda	mouseButtonsOld		;No, but was it last time?
		bit	#MOUSE_RBTN
		beq	done$			;No - don't do anything here
		
		lda	#0x01			;Yes - flag have right click
		sta	mouseBtnRClick

done$:
		lda	mouseButtons			;Store the current state
		sta	mouseButtonsOld
		rts


;-----------------------------------------------------------
_mouseMoveSprX:
;-----------------------------------------------------------
		lda	mouseXPos + 1
		lsr a
		sta	mouseTemp0
		lda	mouseXPos
		ror a
		sta	mouseXCell
		lda	mouseTemp0
		lsr a
		sta	mouseTemp0
		lda	mouseXCell
		ror a
		sta	mouseXCell

		clc
		lda	mouseXPos
		adc	#SPRITE_OFFX
		sta	mouseTemp1
		lda	mouseXPos + 1
		adc	#0x00
		sta	mouseTemp1 + 1

		lda	mouseTemp1
		sta	VIC_XPOS
		lda	mouseTemp1 + 1
		cmp	#0x00
		beq	unset$

		lda	#0x01
		tsb	VIC_XPOSMSB
		rts
	
unset$:
		lda	#0x01
		TRB	VIC_XPOSMSB

		rts

;-----------------------------------------------------------
_mouseMoveSprY:
;-----------------------------------------------------------
		lda	mouseYPos + 1
		lsr a
		sta	mouseTemp0
		lda	mouseYPos
		ror a
		sta	mouseYCell
		lda	mouseTemp0
		lsr a
		sta	mouseTemp0
		lda	mouseYCell
		ror a
		sta	mouseYCell

		clc
		lda	mouseYPos
		adc	#SPRITE_OFFY
		sta	mouseTemp1
		lda	mouseYPos + 1
		adc	#0x00
		sta	mouseTemp1 + 1

		lda	mouseTemp1
		sta	VIC_YPOS

		rts


;-----------------------------------------------------------
_keysInputKeys:
;-----------------------------------------------------------
loop$:
		ldx	0xD611
		lda	0xD610

		sta	0xD610

		bne	enqueue$

		rts

enqueue$:
		jsr	judeEnqueueKey
		bra	loop$


;-----------------------------------------------------------
_judeVolatileStore:
;-----------------------------------------------------------
		ldx	#0xff
loop$:
		lda	0x00, x
store$:
		sta	jude_volstr, x

    dex
		cpx #0x01
		bne	loop$

		rts


;-----------------------------------------------------------
_judeVolatileLoad:
;-----------------------------------------------------------
		ldx	#0xff
loop$:
		lda	jude_volstr, x
store$:
		sta	0x00, x

		dex
    cpx #0x01
		bne	loop$

		rts


;-----------------------------------------------------------
_judeUserIRQNOP:
;-----------------------------------------------------------
		rti


;-----------------------------------------------------------
_judeUserIRQ:
;-----------------------------------------------------------
		php				;save the initial state
		pha
		phx
		phy
		phz

		cld

start$:
		lda	#0x01
		sta	karl_lock

		jsr	_judeVolatileStore

	#ifdef	DEBUG_RASTERTIME
		lda	#0x00
		sta	VIC_BRDRCLR
    inc   0xd020
	#endif

;	Is the VIC-II needing service?
		lda	VIC_IRQFLGS
		and	#0x01
		bne	vicirq$

;	Some other interrupt source
    lda 0xd080
    and #0x80
    beq nextirq0$

		jsr	karlPanic

nextirq0$:
    lda 0xdc0d
    and #0x80
    beq nextirq1$

    jsr karlPanic

nextirq1$:
    bra proc$

vicirq$:
; Seem to need to reguardless
		asl VIC_IRQFLGS

proc$:
		MvDWMem	zp:zptrtemp1, zp:zptrself

		jsr	_judeUserIRQHandler

		MvDWMem	zp:zptrself, zp:zptrtemp1

	#ifdef	DEBUG_RASTERTIME
		lda	#.byte0 CLR_EMPTY
		ldx	#.byte1 CLR_EMPTY
		jsr	_judeLogClrToSys
		sta	VIC_BRDRCLR
	#endif

		jsr	_judeVolatileLoad

		lda	#0x00
		sta	karl_lock

		plz
		ply
		plx
		pla
		plp

		rti


;-----------------------------------------------------------
_judeUserIRQHandler:
;-----------------------------------------------------------
keys$:
		jsr	_keysInputKeys

mouse$:
    jsr input_update
    jsr	_mouseInputMouse
    ;jsr _mouseButtonCheck

    ; prepare CIA1 alredy for sampling mouse, as this takes some time
    lda #0x40
    sta 0xdc00

    jsr _mouseMoveSprX
    jsr _mouseMoveSprY

		jsr	_mouseProcessMouse
		jsr	_mouseProcessClick


exit$:

		rts


;-----------------------------------------------------------
_judeBackupKernalZP:
;-----------------------------------------------------------
    ldx #0
loop$:
    lda zp:00, X
    sta jude_kernal, X

    inx
    bne loop$

    rts


;-----------------------------------------------------------
_judeBackupOwnZP:
;-----------------------------------------------------------
    ldx #0
loop$:
    lda zp:00, X
    sta jude_runtime, X

    inx
    bne loop$

    rts

;-----------------------------------------------------------
_judeRestoreKernalZP:
;-----------------------------------------------------------
    ldx #2
loop$:
    lda jude_kernal, X
    sta zp:00, X

    inx
    bne loop$

    rts


;-----------------------------------------------------------
_judeRestoreOwnZP:
;-----------------------------------------------------------
    ldx #2
loop$:
    lda jude_runtime, X
    sta zp:00, X

    inx
    bne loop$

    rts


;-----------------------------------------------------------
_judeDefCorePrepare:
;-----------------------------------------------------------
		sei

		;lda	#0x7F			;disable standard CIA irqs
		;sta	CIA1_IRQCTL
    ;sta CIA2_IRQCTL
		;lda	CIA1_IRQCTL
		;lda	CIA2_IRQCTL

;	Bank out BASIC+Kernal, keep IO
;	First, make sure that the IO port line are set to output
		;lda	0x00
		;ora	#0x07
		;sta	0x00

;	Now, exclude BASIC+KERNAL from the memory map (keep IO)
		;lda	#0x1D
		;sta	0x01

    lda #0x18
    trb 0xd030

		;lda	#0x00
		;ldx	#0x0f
		;ldy	#0x00
		;ldz	#0x0f
		;map

;		lda	#0
;		tax
;		tay
;		taz
;		map
;		eom

    ;lda #0x00 ; MAPLO
    ;ldx #0x00
    ;ldy #0x00  ; MAPHI
    ;ldz #0xE3
    ;map
    ;eom

		;lda	#0x00
		;sta	0xd02f

		lda	#0x70
		sta	0xD640
		clv

    ;lda #0x57
    ;sta zptrtemp0
    ;lda #0xcb
    ;sta zptrtemp0 + 1
    ;lda #0x02
    ;sta zptrtemp0 + 2
    ;lda #0x00
    ;sta zptrtemp0 + 3

    ;lda #0xea
    ;ldz #0x00
    ;sta [zptrtemp0], z
    ;inz
    ;sta [zptrtemp0], z

 		lda	#0x00
		sta	karl_errorno

		rts

;-----------------------------------------------------------
_judeDefCoreInit:
;-----------------------------------------------------------
		lda	#.byte0 _judeUserIRQ		;install our handler
		sta	CPU_IRQ
		lda	#.byte1 _judeUserIRQ
		sta	CPU_IRQ + 1

		lda	#.byte0 _judeUserIRQNOP		;install our handler
		sta	CPU_RESET
		lda	#.byte1 _judeUserIRQNOP
		sta	CPU_RESET + 1

		lda	#.byte0 _judeUserIRQNOP		;install our handler
		sta	CPU_NMI
		lda	#.byte1 _judeUserIRQNOP
		sta	CPU_NMI + 1

    ;asl VIC_IRQFLGS

		lda	#0b01111111		;We'll always want rasters
		and	VIC_CTRLREG		;    less than 0x0100
		sta	VIC_CTRLREG
		
		lda	#0x19
		sta	VIC_RSTRVAL
		
		lda	#0x01			;Enable raster irqs
		sta	VIC_IRQMASK

    ;asl VIC_IRQFLGS


		lda	#0x00
		sta	karl_errorno

		cli

		rts

;-----------------------------------------------------------
_judeDefCoreChange:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts

;-----------------------------------------------------------
_judeDefCoreRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
_judeProxy:
;-----------------------------------------------------------
		jmp	(jude_proxyptr)


;-----------------------------------------------------------
_judeOnIdleProxy:
;-----------------------------------------------------------
		jmp	(jude_onidle)



 .section rodata, rodata

themeCnt:
		.byte	0x0A


;	.define	CLR_BACK		0x00FD		;Background on C64
;	.define	CLR_EMPTY		0x00FE		;Border on C64
;	.define	CLR_CURSOR		0x00FF		

;	.define	CLR_TEXT		0x0000
;	.define	CLR_FOCUS		0x0001

;	.define	CLR_INSET		0x0002
;	.define	CLR_FACE		0x0003
;	.define CLR_SHADOW		0x0004

;	.define CLR_PAPER		0x0005
;	.define CLR_MONEY		0x0006
;	.define	CLR_ITEM		0x0007

;	CLR_INFORM: .equ		0x0008
;	CLR_ACCEPT: .equ		0x0009

;	CLR_APPLY: .equ		0x000A
;	CLR_ABORT: .equ		0x000B


theme0:
		.asciz		"CORPORATE       "
		.byte		0x00, 0x06, 0x04, 0x01, 0x01, 0x06, 0x0E, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x0E, 0x0D, 0x07, 0x0A

		.asciz		"SIMPLY DARKENED "
		.byte		0x01, 0x00, 0x04, 0x0C, 0x0E, 0x00, 0x0F, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x0E, 0x0D, 0x07, 0x0A

		.asciz		"FAMILIAR        "
		.byte		0x00, 0x0E, 0x06, 0x01, 0x01, 0x0E, 0x04, 0x0C
		.byte		0x0F, 0x03, 0x0F, 0x06, 0x0D, 0x07, 0x0A

		.asciz		"DARK PET'O DAYS "
		.byte		0x0D, 0x00, 0x05, 0x00, 0x01, 0x00, 0x04, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x03, 0x05, 0x07, 0x0A

		.asciz		"ASTRO SHADES    "
		.byte		0x00, 0x00, 0x0A, 0x02, 0x01, 0x00, 0x0A, 0x0B
		.byte		0x0F, 0x0A, 0x0C, 0x0F, 0x0D, 0x07, 0x0A

		.asciz		"FOSS FURS IS US "
		.byte		0x00, 0x05, 0x0D, 0x01, 0x01, 0x05, 0x0D, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x0D, 0x0D, 0x07, 0x0A

		.asciz		"CORPORATE NUEVO "
		.byte		0x00, 0x06, 0x04, 0x01, 0x01, 0x06, 0x04, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x0E, 0x0D, 0x07, 0x0A

		.asciz		"WESTERN POSTCARD"
		.byte		0x00, 0x09, 0x07,   0x07, 0x01,   0x09, 0x08, 0x0B
		.byte		0x0F, 0x03, 0x0C,   0x0E, 0x0D,   0x03, 0x0A

		.asciz		"PASSION-ATE     "
		.byte		0x00, 0x04, 0x0A, 0x0A, 0x01, 0x04, 0x03, 0x0B
		.byte		0x0F, 0x0E, 0x0C, 0x07, 0x0D, 0x0E, 0x0A

		.asciz		"MISSION: SUNRISE"
		.byte		0x00, 0x07, 0x08, 0x09, 0x01, 0x07, 0x08, 0x0B
		.byte		0x0F, 0x03, 0x0C, 0x0E, 0x0D, 0x03, 0x0A


pointer0:
		.byte		0x33, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00
		.byte		0x32, 0x22, 0x22, 0x22, 0x23, 0x00, 0x00, 0x00
		.byte		0x32, 0x44, 0x44, 0x42, 0x30, 0x00, 0x00, 0x00
		.byte		0x32, 0x31, 0x11, 0x23, 0x00, 0x00, 0x00, 0x00
		.byte		0x32, 0x31, 0x11, 0x42, 0x30, 0x00, 0x00, 0x00
		.byte		0x32, 0x31, 0x11, 0x14, 0x23, 0x00, 0x00, 0x00
		.byte		0x32, 0x32, 0x31, 0x11, 0x42, 0x30, 0x00, 0x00
		.byte		0x32, 0x23, 0x23, 0x11, 0x14, 0x23, 0x00, 0x00
		.byte		0x32, 0x30, 0x32, 0x31, 0x32, 0x30, 0x00, 0x00
		.byte		0x33, 0x00, 0x03, 0x23, 0x23, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x32, 0x30, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

pointer1:
		.byte		0x00, 0x03, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00
		.byte		0x00, 0x32, 0x22, 0x22, 0x22, 0x23, 0x30, 0x00
		.byte		0x03, 0x24, 0x44, 0x44, 0x44, 0x42, 0x23, 0x00
		.byte		0x32, 0x33, 0x11, 0x11, 0x11, 0x14, 0x23, 0x00
		.byte		0x32, 0x31, 0x11, 0x11, 0x11, 0x14, 0x23, 0x00
		.byte		0x32, 0x31, 0x11, 0x11, 0x11, 0x14, 0x23, 0x00
		.byte		0x32, 0x31, 0x11, 0x11, 0x11, 0x14, 0x23, 0x00
		.byte		0x32, 0x33, 0x11, 0x11, 0x11, 0x42, 0x33, 0x00
		.byte		0x03, 0x23, 0x32, 0x34, 0x44, 0x22, 0x30, 0x00
		.byte		0x00, 0x32, 0x23, 0x31, 0x11, 0x42, 0x30, 0x00
		.byte		0x03, 0x24, 0x43, 0x11, 0x14, 0x23, 0x00, 0x00
		.byte		0x32, 0x31, 0x14, 0x23, 0x32, 0x30, 0x00, 0x00
		.byte		0x32, 0x33, 0x32, 0x32, 0x23, 0x00, 0x00, 0x00
		.byte		0x03, 0x22, 0x23, 0x33, 0x30, 0x00, 0x00, 0x00
		.byte		0x00, 0x33, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		.byte		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00



    .section data, data
actvtheme:
    .byte 0x00


mod_jude_core:
;	Object
		.byte		sizeof_MODULE			;size
		.long 	0x00000000				;parent
		.word		karlDefModPrepare		;prepare
		.word		karlDefModInit			;initialise
		.word		karlDefModChange		;change
		.word		karlDefModRelease		;release
		.word		STATE_DEFAULT			;state
		.word		0x0000					;oldstate
		.word		0x0000					;options
		.word		0x0000					;tag
;	Named
		.asciz		"JUDE CORE       "		;name
;	Module
		.long		uns_jude_core			;units_p
		.byte		0x01						;unitscnt

uns_jude_core:
		.long		uni_jude_core

uni_jude_core:
;	Object
		.byte		sizeof_UNIT			;size
		.long		mod_jude_core			;parent
		.word		_judeDefCorePrepare	;prepare
		.word		_judeDefCoreInit		;initialise
		.word		_judeDefCoreChange		;change
		.word		_judeDefCoreRelease	;release
		.word		STATE_DEFAULT			;state
		.word		0x0000					;oldstate
		.word		0x0000					;options
		.word		0x0000					;tag
;	Named
		.asciz		"JUDE CORE       "		;name


dma_view_clear:
	.byte	0x0B					; Request format is F018B
dma_vw_smb:
	.byte	0x80,0x00				; Source MB 
dma_vw_dmb:
	.byte	0x81,0x00				; Destination MB 
	.byte	0x00					; No more options
	.byte	0x00					;Command LSB
dma_vw_siz:
	.word	0x0000				;Count LSB Count MSB
dma_vw_sadr:
	.word	0x0000				;Source Address LSB Source Address MSB
dma_vw_sbnk:
	.byte	0x00					;Source Address BANK and FLAGS
dma_vw_dadr:
	.word	0x0000				;Destination Address LSB Destination Address MSB
dma_vw_dbnk:
	.byte	0x00					;Destination Address BANK and FLAGS
	.byte	0x00					;Command MSB
	.word	0x0000				;Modulo LSB / Mode Modulo MSB / Mode



jude_proxyptr:
		.word	0x0000


jude_temp0:
		.byte	0x00


jude_actvvw:
		.long	0x00000000
jude_actvpg:
		.long	0x00000000
jude_avhrsx:
		.byte	0x00
jude_avhrsy:
		.byte	0x00

jude_proc:
		.byte	0x00


jude_screenram:
		.long	0x00000000
jude_mousevic:
		.word	0x0000
jude_mouseram:
		.long	0x00000000
jude_mouseptr:
		.long	0x00000000
jude_mousepal:
		.byte	0x00
jude_mptrstate:
		.byte	0x00

jude_screenw:
		.byte	0x00
jude_screenh:
		.byte	0x00
jude_cellsize:
		.byte	0x00
jude_linesize:
		.long	0x00000000
jude_screensize:
		.long	0x00000000

jude_screeny0:
		.space 50 * 4, 0

jude_coloury0:
		.space 50 * 4, 0

jude_theme:
		.space	15, 0


;-----------------------------------------------------------
;Mouse driver variables
;-----------------------------------------------------------
mouseOldPotX:        
	.byte    	0               	; Old hw counter values
mouseOldPotY:        
	.byte    	0

mouseDirTemp:	
	.byte 0
mouseXDir:	
	.byte 0
mouseYDir:	
	.byte 0
mouseXPosNew:           
	.word    	0               
mouseYPosNew:           
	.word    	0               
mouseXPosPending:           
	.word    	0               
mouseYPosPending:           
	.word    	0               
mouseXPos:
	.word    	0               	; Current mouse position, x
mouseYPos:
	.word    	0               	; Current mouse position, y
mouseXMin:           
	.word    	0               	; X1 value of bounding box
mouseYMin:           
	.word    	0               	; Y1 value of bounding box
mouseXMax:           
	.word    	319               	; X2 value of bounding box
mouseYMax:           
	.word    	199           		; Y2 value of bounding box
mouseButtons:        
	.byte    	0               	; button status bits
mouseButtonsOld:
	.byte		0
mouseBtnLClick:
	.byte		0
mouseBtnRClick:
	.byte		0
	
mouseOldValue:       
	.byte    	0               	; Temp for MoveCheck routine
mouseNewValue:       
	.byte    	0               	; Temp for MoveCheck routine

mouseCheck:
		.byte		0x00
mouseTemp0:
		.byte		0x00
mouseTemp1:
		.word		0

mouseCapture:
		.byte		0x00
mouseCapCtrl:
			.word	0x0000
mouseCapMove:
			.word	0x0000
mouseCapClick:
			.word	0x0000

mouseXCol:
	.byte		0x00
mouseYRow:
	.byte		0x00
mouseXCell:
	.byte		0x00
mouseYCell:
	.byte		0x00


keysBuffer0:
	.space	16, 0x00

keysIdx:
	.byte	0x00


judePickElem:
		.long 0x00000000
judeMsePElem:
		.long 0x00000000
judeDownElem:
		.long 0x00000000
judeActvElem:
		.long 0x00000000

judePBlinkDelay:
		.byte	0x00
judePBlinkState:
		.byte	0x00

jude_irqthread:
    .word   0x0000;
jude_onidle:
    .word   0x0000;


jude_initflags:
    .byte 0x00

jude_volstr:
		.space	0xff, 0

jude_kernal:
		.space	0xff, 0

jude_runtime:
		.space	0xff, 0
