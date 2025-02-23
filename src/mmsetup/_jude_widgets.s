;===========================================================
	#include	"_karljr_types.inc"
	#include	"_jude_types.inc"
	#include	"_jude_widgets.inc"
;===========================================================

 .section code

;===========================================================
	.public		judeDefCtlPrepare
	.public		judeDefCtlInit
	.public		judeDefCtlChange
	.public		judeDefCtlRelease
	.public		judeDefCtlPresent
	.public		judeDefCtlKeypress
	.public		judeDefEdtPresent
	.public		judeDefEdtKeypress

	.public		judeDefLblChange

	.public		judeDefPBtChange
	.public		judeDefPBtPresent

	.public		judeDefRGpChange
	.public		judeDefRBtChange

	.public		judeRGroupReset
;===========================================================


;===========================================================
	.extern		judeActivatePage

	.extern		judeUnDownCtrl
	.extern		judeDownCtrl
	.extern		judeDeActivateCtrl
	.extern		judeActivateCtrl
	.extern		judeEraseBkg
	.extern		_judeDrawText
	.extern		_judeDrawTextDirect
	.extern		judeDrawAccel
	.extern		_judeLogClrToSys
	.extern		_judeLogClrIsReverse
	.extern		judeEnqueueKey
	.extern		judeDequeueKey

	.extern		jude_actvpg
	.extern		jude_cellsize
	.extern		jude_coloury0
	.extern		jude_screeny0
	.extern		judeDownElem

	.extern		karl_errorno
	.extern		karlObjExcludeState
	.extern		karlObjIncludeState
	.extern		_karlObjIncStateEx
;===========================================================


;===========================================================
;	Routines
;===========================================================

;-----------------------------------------------------------
judeDefCtlPrepare:
;-----------------------------------------------------------
;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_PREPARED
		jsr	karlObjIncludeState

		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefCtlInit:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefCtlChange:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		lda	[zp:zptrself], z
		and	#STATE_CHANGED
		beq	exit$

		lda	#0x00
		sta	zp:zregAb0

		lda	[zp:zptrself], Z
		and	#STATE_DOWN
		beq	dirty$

		lda	#0x01
		sta	zp:zregAb0
		
		ldz	#OBJECT__options
		lda	[zp:zptrself], Z
		and	#OPT_DOWNCAPTURE
		bne	dirty$

		ldz	#OBJECT__state
		lda	[zp:zptrself], Z
		and	#(0xFF ^ STATE_DOWN)
		sta	[zp:zptrself], Z

		MvDWZ	judeDownElem

dirty$:
		lda	#STATE_CHANGED
		jsr	karlObjExcludeState

		ldz	#OBJECT__options
		
		lda	zp:zregAb0
		beq	cont1$

auto$:
		lda	[zp:zptrself], Z
		and	#OPT_AUTOCHECK
		beq	cont1$
		
		ldz	#OBJECT__tag
		lda	[zp:zptrself], Z
		beq	check$

		ldz	#OBJECT__options + 1
		lda	[zp:zptrself], Z

		dez

		and	#.byte1 OPT_NOAUTOCHKOF
		bne	cont1$

		ldz	#OBJECT__tag
		lda	#0x00
		bra	cont0$

check$:
		lda	#0x01
		
cont0$:
		sta	[zp:zptrself], Z
		
		ldz	#OBJECT__options
cont1$:
		lda	[zp:zptrself], Z
		and	#OPT_NOAUTOINVL
		bne	exit$

;		lda	#0x01
;		sta	zp:zreg2b3
		lda	#STATE_DIRTY
		jsr	karlObjIncludeState

exit$:
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefCtlRelease:
;-----------------------------------------------------------
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefCtlPresent:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		lda	[zp:zptrself], Z

		sta	zp:zregAb0

		and	#STATE_VISIBLE
		bne	present$
		
		lbra	exit$

present$:
		lda	zp:zregAb0
		and	#STATE_ENABLED
		bne	checkpick$
		
		lda	#.byte0 CLR_SHADOW
		ldx	#.byte1 CLR_SHADOW
		bra	draw$
		
checkpick$:
		lda	zp:zregAb0				;Check if picked
		bit	#STATE_PICKED
		beq	checkactv$
		bit	#STATE_ACTIVE
		bne	normal$


picked$:
		ldz	#ELEMENT__colour + 1;Check its not already FOCUS
		lda	[zp:zptrself], Z

		cmp	#.byte1 CLR_FOCUS
		bne	pickednrm$

		dez
		lda	[zp:zptrself], Z

		cmp	#.byte0 CLR_FOCUS
		bne	pickednrm$
		
		lda	#.byte0 CLR_FACE
		ldx	#.byte1 CLR_FACE
		bra	draw$

pickednrm$:
		lda	#.byte0 CLR_FOCUS
		ldx	#.byte1 CLR_FOCUS
		bra	draw$

checkactv$:
		lda	zp:zregAb0
		and	#STATE_ACTIVE
		bne	picked$			;Make it the same as picked
		
normal$:
		ldz	#ELEMENT__colour + 1
		lda	[zp:zptrself], Z
		tax
		dez
		lda	[zp:zptrself], Z
		
draw$:
		sta	zp:zregAb0
		stx	zp:zregAb1

		jsr	judeEraseBkg

		lda	#0x00
		sta	zp:zregAb2

		ldz	#CONTROL__textoffx
		lda	[zp:zptrself], Z
		sta	zp:zregAb3

		ldz	#ELEMENT__width
		lda	[zp:zptrself], Z
		
		sec
		sbc	zp:zregAb3
		sta	zp:zregAb3

		lda	#0x01
		sta	zp:zregBb0

		jsr	_judeDrawText

		jsr	judeDrawAccel
		
		ldz	#OBJECT__options
		;nop
		lda	[zp:zptrself], Z
		and	#OPT_AUTOCHECK
		beq	exit$
		
		ldz	#OBJECT__tag
		;nop
		lda	[zp:zptrself], Z
		beq	exit$
		
		ldz	#ELEMENT__posx
		;nop
		lda	[zp:zptrself], Z

		ldx	jude_cellsize
		cpx	#0x02
		bne	xcont$

		asl a

xcont$:
;		sta	zp:zregBb1
		sta	zp:zregBb3
		


		inz
		;nop
		lda	[zp:zptrself], Z
		sta	zp:zregBb2

		lda	zp:zregBb2
		asl a
		asl a
		tax

		lda	jude_screeny0, X
		sta	zp:zptrscreen		;screen ptr
		lda	jude_screeny0 + 1, X
		sta	zp:zptrscreen + 1
		lda	jude_screeny0 + 2, X
		sta	zp:zptrscreen + 2
		lda	jude_screeny0 + 3, X
		sta	zp:zptrscreen + 3		

		lda	jude_coloury0, X
		sta	zp:zptrcolour		;screen ptr
		lda	jude_coloury0 + 1, X
		sta	zp:zptrcolour + 1
		lda	jude_coloury0 + 2, X
		sta	zp:zptrcolour + 2
		lda	jude_coloury0 + 3, X
		sta	zp:zptrcolour + 3
		
		lda	#.byte0 CLR_TEXT
		ldx	#.byte1 CLR_TEXT
		jsr	_judeLogClrToSys

		pha

		lda	zp:zregBb3
		taz

;	Use 0x51 for uppercase
		lda	#0x7A
		;nop
		sta	[zp:zptrscreen], Z

		ldx	jude_cellsize
		cpx	#0x02
		bne	clrcont$

		inz

clrcont$:
		pla

		;nop
		sta	[zp:zptrcolour], Z
		
exit$:
		lda	#STATE_DIRTY
		jsr	karlObjExcludeState

		lda	#0x00
		sta	karl_errorno

		rts

;-----------------------------------------------------------
judeDefCtlKeypress:
;-----------------------------------------------------------
		lda	#ERROR_NONE
		sta	karl_errorno

		rts


;-----------------------------------------------------------
judeDefLblChange:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z

		pha

		jsr	judeDefCtlChange

		pla
		and	#STATE_DOWN
		beq	exit$

		MvDWObjImm	zp:zreg4, zp:zptrself, LABELCTRL__actvctrl_p
		
		lda	zp:zreg4b0
		ora	zp:zreg4b1
		ora	zp:zreg4b2
		ora	zp:zreg4b3
		beq	exit$

		MvDWMem	zp:zptrself, zp:zreg4
		jsr	judeDownCtrl

exit$:
		rts
		

;-----------------------------------------------------------
judeDefPBtChange:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z

		pha

		jsr	judeDefCtlChange

		pla
		and	#STATE_DOWN
		beq	exit$

		MvDWObjImm	zp:zreg4, zp:zptrself, PAGEBTNCTRL__actvpage_p
		
		lda	zp:zreg4b0
		ora	zp:zreg4b1
		ora	zp:zreg4b2
		ora	zp:zreg4b3
		beq	exit$

		MvDWMem	zp:zptrself, zp:zreg4
		jsr	judeActivatePage

exit$:
		rts


;-----------------------------------------------------------
judeDefPBtPresent:
;-----------------------------------------------------------
		MvDWObjImm	zp:zreg4, zp:zptrself, PAGEBTNCTRL__actvpage_p
		
		LDQMem	zp:zreg4
		CPQMem	jude_actvpg

		bne	unset$

		ldz	#ELEMENT__colour
		lda	#.byte0 CLR_FOCUS
		;nop
		sta	[zp:zptrself], Z
		inz
		lda	#.byte1 CLR_FOCUS
		;nop
		sta	[zp:zptrself], Z

		bra	default$

unset$:
		ldz	#ELEMENT__colour
		lda	#.byte0 CLR_FACE
		;nop
		sta	[zp:zptrself], Z
		inz
		lda	#.byte1 CLR_FACE
		;nop
		sta	[zp:zptrself], Z

default$:
		jmp	judeDefCtlPresent



;-----------------------------------------------------------
judeDefEdtPresent:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z
		and	#STATE_DOWN
		beq	normal$

		lda	#.byte0 CLR_TEXT
		sta	zp:zregAb0
		ldx	#.byte1 CLR_TEXT
		stx	zp:zregAb1

		jsr	judeEraseBkg

		ldz	#CONTROL__textoffx
		;nop
		lda	[zp:zptrself], Z
		sta	zp:zregAb3

		ldz	#ELEMENT__width
		;nop
		lda	[zp:zptrself], Z
		
		sec
		sbc	zp:zregAb3
		sta	zp:zregAb3

		dec	zp:zregAb3

		ldz	#EDITCTRL__textsz
		;nop
		lda	[zp:zptrself], Z
		sta	zp:zregAb2

		lda	zp:zregAb3
		cmp	zp:zregAb2
		bcs	noindent$

		sec
		lda	zp:zregAb2
		sbc	zp:zregAb3
		sta	zp:zregAb2
	
		bra	text$

noindent$:
		lda	#0x00
		sta	zp:zregAb2

text$:
		lda	#0x00
		sta	zp:zregBb0

;	IN	zp:zregAwl		Colour
;	IN	zp:zregAb2		Indent
;	IN	zp:zregAb3		Max width
;	IN	zp:zregBb0		Do cont char if opt
		jsr	_judeDrawText
		
		rts

normal$:
		jmp	judeDefCtlPresent


;-----------------------------------------------------------
judeDefEdtKeypress:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z
		and	#STATE_DOWN
		bne	downkeys$

		rts

downkeys$:
		lda	zvalkey
		cmp	#KEY_C64_CDOWN
		beq	navkey$

		cmp	#KEY_ASC_CR
		bne	input$

		jsr	judeUnDownCtrl
		rts

navkey$:
		jsr	judeUnDownCtrl
		

input$:
		MvDWObjImm	zp:zregA, zp:zptrself, CONTROL__text_p

		ldz	#EDITCTRL__textsz
		;nop
		lda	[zp:zptrself], Z
		sta	zp:zregBb0

		lda	zvalkey
		cmp	#KEY_C64_DEL
		beq	delete$

		ldz	#EDITCTRL__textmaxsz
		;nop
		lda	[zp:zptrself], Z
		cmp	zp:zregBb0
		beq	exit$

		;ldz	zp:zregBb0
    lda zp:zregBb0
    taz

		lda	zvalkey
		;nop
		sta	[zp:zregA], Z
		
		inz
		lda	#0x00
		;nop
		sta	[zp:zregA], Z
		tza

invalidate$:
		ldz	#EDITCTRL__textsz
		;nop
		sta	[zp:zptrself], Z

;		lda	#0x00
;		sta	zp:zreg2b3
		lda	#(STATE_DIRTY | STATE_CHANGED)
		jsr	karlObjIncludeState

exit$:
		jmp	judeDefCtlKeypress

delete$:


		lda	zp:zregBb0
		taz

		beq	exit$

		dez

		lda	#0x00
		;nop
		sta	[zp:zregA], Z
		
		tza
		
		bra	invalidate$


;-----------------------------------------------------------
judeRGroupReset:
;-----------------------------------------------------------
		sta	zp:zregDb1

		lda	#0x00
		sta	zp:zregDb0
		sta	zp:zregDb3

		MvDWMem	zp:zregA, zp:zptrself

		MvDWObjImm	zp:zregC, zp:zregA, RADIOGRPCTRL__controls_p

		ldz	#RADIOGRPCTRL__controlscnt
		;nop
		lda	[zp:zregA], Z
		sta	zp:zregDb2

loop$:
		ldz	#0x00
		LDQIndDWZ zp:zregC
		STQMem	zp:zregE

		LDQMem	zp:zregC
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregC

		lda	zp:zregDb0
		cmp	zp:zregDb1
		beq	found$

		ldz	#OBJECT__tag
		lda	#0x00

update$:
		;nop
		sta	[zp:zregE], Z

		MvDWMem zp:zptrself, zp:zregE

		lda	#STATE_CHANGED | STATE_DIRTY
		jsr	karlObjIncludeState

next$:
		inc	zp:zregDb0

		lda	zp:zregDb0
		cmp	zp:zregDb2
		bne	loop$

		bra	label$

found$:
		sta	zp:zregDb3

		LDQMem	zp:zregC
		STQMem	zp:zregF

		ldz	#OBJECT__tag
		lda	#0x01
		bra	update$

label$:
		lda	zp:zregDb3
		cmp	zp:zregDb1
		bne	exit$

		inc zp:zregDb3

		MvDWObjImm	zp:zregB, zp:zregA, RADIOGRPCTRL__labelctrl

		lda	zp:zregBb0
		ora	zp:zregBb1
		ora	zp:zregBb2
		ora	zp:zregBb3
		beq	exit$

		lda	zp:zregDb3
		cmp	zp:zregDb2
		bcc	this$

		MvDWObjImm	zp:zregF, zp:zregA, RADIOGRPCTRL__controls_p

this$:
		ldz	#0x00
		LDQIndDWZ zp:zregF
		STQMem	zp:zregE

set$:
		ldz	#LABELCTRL__actvctrl_p

		lda	zp:zregEb0
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb1
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb2
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb3
		;nop
		sta	[zp:zregB], Z


exit$:
		rts

;-----------------------------------------------------------
judeDefRGpChange:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z

		ldz	#OBJECT__oldstate + 1
		;nop
		sta	[zp:zptrself], Z

		jsr	judeDefCtlChange

		ldz	#OBJECT__tag
		;nop
		lda	[zp:zptrself], Z

		beq	exit$

		ldz	#OBJECT__oldstate + 1
		;nop
		lda	[zp:zptrself], Z

		and	#STATE_DOWN
		beq	exit$

		MvDWMem	zp:zregA, zp:zptrself
		jsr	_judeUpdateRadioGroup

exit$:
		lda	#0x00
		sta	karl_errorno

		rts


;-----------------------------------------------------------
_judeUpdateRadioGroup:
;-----------------------------------------------------------
		MvDWMem	zp:zregB, zp:zptrself

;	Unset all other than zp:zregB in group
;		Store index of zp:zregB
		MvDWObjImm	zp:zregC, zp:zregA, RADIOGRPCTRL__controls_p

		lda	#0x00
		sta	zp:zregDb0

		lda	#0xFF
		sta	zp:zregDb1

		ldz	#RADIOGRPCTRL__controlscnt
		;nop
		lda	[zp:zregA], Z
		sta	zp:zregDb2

loop$:
		ldz	#0x00
		LDQIndDWZ zp:zregC
		
		CPQMem	zp:zregB
		beq	found$

		STQMem	zp:zregE

		ldz	#OBJECT__tag

		lda	#0x00
		;nop
		sta	[zp:zregE], Z

		MvDWMem zp:zptrself, zp:zregE

		lda	#STATE_CHANGED | STATE_DIRTY
		jsr	karlObjIncludeState

		LDQMem	zp:zregC
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregC

next$:
		inc	zp:zregDb0

		lda	zp:zregDb0
		cmp	zp:zregDb2
		bne	loop$

		bra	cont$

found$:
		LDQMem	zp:zregC
		clc
		ADQMem	zvaltemp0
		STQMem	zp:zregC
		STQMem	zp:zregF

		lda	zp:zregDb0
		sta	zp:zregDb1

		bra	next$

cont$:
		inc	zp:zregDb1

		lda	zp:zregDb1
		cmp	zp:zregDb2
		bcc	set$

		MvDWObjImm	zp:zregF, zp:zregA, RADIOGRPCTRL__controls_p

set$:
		ldz	#0x00
		LDQIndDWZ zp:zregF
		STQMem	zp:zregE

		MvDWObjImm	zp:zregB, zp:zregA, RADIOGRPCTRL__labelctrl

		lda	zp:zregBb0
		ora	zp:zregBb1
		ora	zp:zregBb2
		ora	zp:zregBb3
		beq	exit$

		ldz	#LABELCTRL__actvctrl_p

		lda	zp:zregEb0
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb1
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb2
		;nop
		sta	[zp:zregB], Z
		inz
		lda	zp:zregEb3
		;nop
		sta	[zp:zregB], Z

exit$:
		rts


;-----------------------------------------------------------
judeDefRBtChange:
;-----------------------------------------------------------
		ldz	#OBJECT__state
		;nop
		lda	[zp:zptrself], Z

		ldz	#OBJECT__oldstate + 1
		;nop
		sta	[zp:zptrself], Z

		jsr	judeDefCtlChange

		ldz	#OBJECT__tag
		;nop
		lda	[zp:zptrself], Z

		beq	exit$

		ldz	#OBJECT__oldstate + 1
		;nop
		lda	[zp:zptrself], Z

		and	#STATE_DOWN
		beq	exit$

		MvDWObjImm	zp:zregA, zp:zptrself, RADIOBTNCTRL__groupctrl_p
		jsr	_judeUpdateRadioGroup

exit$:
		lda	#0x00
		sta	karl_errorno

		