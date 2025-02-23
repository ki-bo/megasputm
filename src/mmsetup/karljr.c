#include	"karljr.h"


void	karlModAttach(karlFarPtr_t module) {
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
