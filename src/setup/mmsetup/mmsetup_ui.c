#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "karljr.h"

uint8_t config_required = 1;



void mmsetupWelcNextChg(void) {
    uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
    judeDefCtlChange();

		if (state & STATE_DOWN) {
      if ((ctl_mmsetup_welc_0_3._element._object.tag == 0) &&
          (ctl_mmsetup_welc_0_4._element._object.tag == 0) &&
          (ctl_mmsetup_welc_0_5._element._object.tag == 0)) {

        ctl_mmsetup_welc_0_6.text_p = (karlFarPtr_t)str_mmsetup_welc_7;

        zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
        karlObjIncludeState(STATE_VISIBLE);
        karlObjIncludeState(STATE_CHANGED);

      } else {
        zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
        karlObjExcludeState(STATE_VISIBLE);

        zptrself = (uint32_t)((__attribute__ ((huge))karlObject_t *)&pge_mmsetup_configure);
        judeActivatePage();
      }
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

  if (state & STATE_DOWN && config_required) {
    ((__attribute__ ((huge))karlObject_t *)zptrself)->tag = 1;
    karlObjIncludeState(STATE_CHANGED);

    ctl_mmsetup_welc_0_6.text_p = (karlFarPtr_t)str_mmsetup_welc_6;

    zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
    karlObjIncludeState(STATE_VISIBLE);
    karlObjIncludeState(STATE_CHANGED);
  }
};