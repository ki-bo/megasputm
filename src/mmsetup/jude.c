#include	"jude.h"
#include <stdint.h>


uint8_t	judeLogClrToSys(uint16_t colour){
  uint8_t result;
  
  __asm(
    " .extern _judeLogClrToSys \n"
    "   jsr _judeLogClrToSys \n"
    : "=Ka" (result)
    : "Ka" ((uint8_t)(colour & 0xff)), "Kx" ((uint8_t)(colour >> 8))
    );

  return result;
};


void	judeViewInit(karlFarPtr_t view) {
	//zreg0wl = view->louint16_t;
	//zreg0wh = view->hiuint16_t;
  zreg0 = (uint32_t)view;

	_judeViewInit();
}

void	judeEraseLine(uint8_t w, uint8_t x, uint8_t y, uint16_t colour) {
//	IN	.A,.X		colour
//	IN	zregAb3		Max width
//	IN	zregBb1		x pos
//	IN	zregBb2		y pos

	zregAwl = colour;
	zregBb1 = x;
	zregBb2 = y;
	zregAb3 = w;

	_judeEraseLine();
}


void 	judeDrawText(uint16_t colour, uint8_t indent, uint8_t mwidth, uint8_t docont) {
//	IN	zregAwl		Colour
//	IN	zregAb2		Indent
//	IN	zregAb3		Max width
//	IN	zregBb0		Do cont char if opt

	zregAwl = colour;
	zregAb2 = indent;
	zregAb3 = mwidth;
	zregBb0 = docont;

	_judeDrawText();
}

void  judeDrawTextDirect(uint16_t colour, uint8_t indent, uint8_t mwidth, uint8_t docont,
  uint8_t x, uint8_t y, uint8_t offs, uint32_t text) {
  kzp.reg[0xa].wl = colour;
  kzp.reg[0xa].b[2] = indent;
  kzp.reg[0xa].b[3] = mwidth;
  kzp.reg[0xb].b[0] = docont;
  kzp.reg[0xb].b[1] = x;
  kzp.reg[0xb].b[2] = y;
  kzp.reg[0xc].b[0] = offs;
  
  kzp.reg[0xd].q = text;

	_judeDrawTextDirect();
}



uint8_t	judeLogClrIsReverse(uint16_t colour) {
  uint8_t result;

	 _judeLogClrIsReverse();

   __asm(
      " lda #0x00 \n"
      " rol a \n"
    :"=Ka" (result)
    :
    :
    );

  return result;   
}


void judeSetTheme(uint8_t theme) {
  __asm(
      " .extern _judeSetTheme \n"
      "   jsr _judeSetTheme \n"
      : 
      : "Ka" (theme)
      : "a"
  );
}


void *judeInstallIdle(void *routine) {
  void *result = (void *)jude_onidle;

  __asm(
    "   cli \n"
  );

  jude_onidle = (uint16_t)(routine);

  __asm(
    "   sei \n"
  );


  return result;
}

char *judeGetThemeDesc(void) {
  char *result = theme0[actvtheme]._name;

  return result;
}
