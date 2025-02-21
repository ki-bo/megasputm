//===========================================================
//jude
//===========================================================
//
// Project: Tilda
//
// Simple text-based GUI system and widget library.
//
// (c) Daniel England 2022, All Rights Reserved.
//
// I intend to release this under LGPL 3?
//
//-----------------------------------------------------------
//
// jude implements a simple, text-based GUI system and some
// widget primitives.  A type of inheritance and simple
// polymorphism is used to implement behaviours.
//
// jude expands on the object system of Karl Jr to define the
// UI elements.  The heirarchy is as so:
//
//				UINTERFACE
//				|
//				+VIEW
//				  |
//				  +LAYER
//				  |
//				  +BAR
//				  | |
//				  | >LAYER
//				  | |
//				  | +CONTROL
//				  |
//				  +PAGE
//				    |
//				    +PANEL
//				      |
//				      >LAYER
//				      |
//				      +CONTROL
//
// A HAL has yet to be implemented.  Some features of this
// module should be moved into another (or that) module as
// functional units.  Namely these features are IRQ, Mouse
// and Keyboard/Text and possibly the Cursor/Pointer.  Some
// abstraction for the Screen is desireable but I'm uncertain
// how it could be easily achieved.
//
// I have purposefully not used messages or a mechanism of
// creating run-time indexes of changes/updates and controls
// because doing so would introduce hard limits on many
// things and given there is no memory manager, would make C
// integration more difficult.
//
// See Karl Jr for further information.
//
//===========================================================

#pragma once

#include	"karljr.h"


#define	CLR_BACK		0x00FD		//Background on C64
#define	CLR_EMPTY		0x00FE		//Border on C64
#define	CLR_CURSOR		0x00FF		
#define	CLR_TEXT		0x0000
#define	CLR_FOCUS		0x0001
#define	CLR_INSET		0x0002
#define	CLR_FACE		0x0003
#define CLR_SHADOW		0x0004
#define CLR_PAPER		0x0005
#define CLR_MONEY		0x0006
#define	CLR_ITEM		0x0007
#define CLR_INFORM		0x0008
#define CLR_ACCEPT		0x0009
#define CLR_APPLY		0x000A
#define CLR_ABORT		0x000B

#define CLR_INTN_THME	0x0000		//Theme colour
#define CLR_SYSS_TEXT	0x1000		//Specific system text colour
#define CLR_SYSS_CTRL	0x2000		//Specific system control colour 


#define MPTR_NONE		0xFF
#define	MPTR_NORMAL		0x00
#define	MPTR_WAIT		0x01


#define INIT_PRESERVEKERNAL 0x01


typedef	struct	THEME {
		karlName_t		_name;
		uint8_t			_data[15];
	} judeTheme_t;


typedef	struct	UINTERFACE {
		karlUnit_t		_unit;

		karlFarPtr_t	mouseloc;
		karlFarPtr_t	mptrloc;
		uint8_t			mousepal;

		karlFarPtr_t	views_p;
		uint8_t			viewscnt;
	} judeUInterface_t;


typedef	struct	VIEW {
		karlObject_t		_object;

		uint8_t			width;
		uint8_t			height;

		karlFarPtr_t	location;
		uint8_t			cellsize;

		karlFarPtr_t	layers_p;
		uint8_t			layerscnt;

		karlFarPtr_t	bars_p;
		uint8_t			barscnt;

		karlFarPtr_t	actvpage;
		karlFarPtr_t	pages_p;
		uint8_t			pagescnt;

		uint16_t			linelen;
	} judeView_t;

typedef	struct	LAYER {
		karlObject_t	_object;

		uint8_t			width;
		uint16_t			offset;
		uint8_t			transparent;
		uint8_t			background;
	} judeLayer_t;


typedef	struct	ELEMENT {
		karlObject_t	_object;

//	Function calls
		EVENTPTR		present;
		EVENTPTR		keypress;

//	Data items
		karlFarPtr_t	owner;
		uint16_t			colour;
		uint8_t			posx;
		uint8_t			posy;
		uint8_t			width;
		uint8_t			height;
	} judeElement_t;

typedef	struct	PANEL {
		judeElement_t	_element;

		karlFarPtr_t	layer_p;

		karlFarPtr_t	controls_p;
		uint8_t			controlscnt;
	} judePanel_t;


typedef	struct	BAR {
		judePanel_t		_panel;
		uint8_t			position;
	} judeBar_t;


typedef	struct	PAGE {
		judeElement_t	_element;

		karlFarPtr_t	pagenxt;
		karlFarPtr_t	pagebak;

		karlFarPtr_t	text_p;
		uint8_t			textoffx;

		karlFarPtr_t	panels_p;
		uint8_t			panelscnt;
	} judePage_t;


typedef	struct	CONTROL {
		judeElement_t	_element;
		karlFarPtr_t	text_p;
		uint8_t			textoffx;
		uint8_t			textaccel;
		uint8_t			accelchar;
	} judeControl_t;

	


void	judeInit(void);
void	judeMain(void);


void	judeDeActivatePage(void);
void	judeActivatePage(void);
void	judeUnDownCtrl(void);
void	judeDownCtrl(void);
void	judeDeActivateCtrl(void);
void	judeActivateCtrl(void);

void judeSetPointer(uint8_t pstate);


void	judeEraseBkg(uint16_t colour);
void	judeDrawAccel(void);

void	judeEnqueueKey(uint16_t key);
void	judeDequeueKey(void);

void	judeDefUIPrepare(void);
void	judeDefUIInit(void);
void	judeDefUIChange(void);
void	judeDefUIRelease(void);
void	judeDefViewPrepare(void);
void	judeDefViewInit(void);
void	judeDefViewChange(void);
void	judeDefViewRelease(void);
void	judeDefLyrPrepare(void);
void	judeDefLyrInit(void);
void	judeDefLyrChange(void);
void	judeDefLyrRelease(void);
void	judeDefPgePrepare(void);
void	judeDefPgeInit(void);
void	judeDefPgeChange(void);
void	judeDefPgeRelease(void);
void	judeDefPgePresent(void);
void	judeDefPnlPrepare(void);
void	judeDefPnlInit(void);
void	judeDefPnlChange(void);
void	judeDefPnlRelease(void);
void	judeDefPnlPresent(void);

uint8_t	judeLogClrToSys(uint16_t colour);

void	judeViewInit(karlFarPtr_t view);
void	judeEraseLine(uint8_t w, uint8_t x, uint8_t y, uint16_t colour);
void  judeDrawText(uint16_t colour, uint8_t indent, uint8_t mwidth, uint8_t docont);
void  judeDrawTextDirect(uint16_t colour, uint8_t indent, uint8_t mwidth, uint8_t docont,
			uint8_t x, uint8_t y, uint8_t offs, unsigned long text);

uint8_t	judeLogClrIsReverse(uint16_t colour);

void judeSetTheme(uint8_t theme);


void  _judeLogClrToSys(void);
void	_judeLogClrIsReverse(void);
void	_judeViewInit(void);
void	_judeEraseLine(void);
void	_judeDrawText(void);
void	_judeDrawTextDirect(void);

void _judeProcessAccelerators(void);
void _judeMoveActiveControl(void);
void _judeBackupKernalZP(void);


void *judeInstallIdle(void *routine);

char *judeGetThemeDesc(void);

extern uint8_t mouseXCol;
extern uint8_t mouseYRow;
extern uint8_t themeCnt;
extern uint8_t actvtheme;
extern judeTheme_t *theme0;


extern uint16_t jude_onidle;
extern int8_t jude_initflags;
