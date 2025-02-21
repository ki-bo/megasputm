#include	"karljr.h"


void	karlModAttach(karlFarPtr_t module) {
	//zreg0wl = ((karlWFarPtr_t)module).data[0];
	//zreg0wh = ((karlWFarPtr_t)module).data[1];
  zreg0 = (uint32_t)(module);

	_karlModAttach();
}


void	karlObjExcStateEx(uint8_t changed, uint16_t state) {
	zreg0wl = state;
	zreg0b2 = changed;

	_karlObjExcStateEx();
}

void	karlObjIncStateEx(uint8_t changed, uint16_t state) {
	zreg0wl = state;
	zreg0b2 = changed;

	_karlObjIncStateEx();
}
