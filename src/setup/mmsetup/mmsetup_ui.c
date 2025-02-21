#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "karljr.h"

uint8_t configRequired = 1;



void mmsetupWelcNextChg(void) {
    uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
    judeDefCtlChange();

		if (state & STATE_DOWN) {
      zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
      karlObjExcludeState(STATE_VISIBLE);

		  zptrself = (uint32_t)((__attribute__ ((huge))karlObject_t *)&pge_mmsetup_configure);
		  judeActivatePage();
    }
}

void mmsetupConfigCancelChg(void) {
    uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
    judeDefCtlChange();

		if (state & STATE_DOWN) {
		  zptrself = (uint32_t)((__attribute__ ((huge))karlObject_t *)&pge_mmsetup_welcome);
		  judeActivatePage();
    }
}

void mmsetupWelcConfigChg(void){
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
  
  judeDefCtlChange();

  if (state & STATE_DOWN && configRequired) {
    ((__attribute__ ((huge))karlObject_t *)zptrself)->tag = 1;
    karlObjIncludeState(STATE_CHANGED);

    zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
    karlObjIncludeState(STATE_VISIBLE);
  }
};