#include "karljr.h"
#include "jude.h"
#include "mmsetup.h"


int main(void) {
  karlInit();
	judeInit();

  karlModAttach((karlFarPtr_t)&mod_mmsetup_app);

  judeViewInit((karlFarPtr_t)&vew_mmsetup_main);

  judeMain();

  return -1;
}