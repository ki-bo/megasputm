#include "jude.h"
#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "karljr.h"

#include "mmsetup_core.h"

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
}

const char blah[] = "BLAH BLAH BLAH     0123456789012345678901234567890123456789123"; 

void mmsetupConfigItemChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
  
  judeDefCtlChange();

  if (state & STATE_DOWN) {
      /*uint8_t __huge *out = (uint8_t __huge *)(0x016000);

      for (uint8_t i = 0; i < 10; ++i) {
        for (uint8_t j = 0; j < 64; ++j) {
          *out = blah[j];
          ++out;
        }
      }*/

      judeSetPointer(MPTR_WAIT);

      lbx_mmsetup_select_0_5.linescnt = readDirectoryFiles("D81");
		  zptrself = (uint32_t)((void __huge *)&lbx_mmsetup_select_0_5);
      karlObjIncludeState(STATE_CHANGED);

      judeSetPointer(MPTR_NORMAL);

		  zptrself = (uint32_t)((__attribute__ ((huge))karlObject_t *)&pge_mmsetup_select);
		  judeActivatePage();
  }
}

void mmsetupSelectCancelChg(void) {
    uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
    judeDefCtlChange();

		if (state & STATE_DOWN) {
		  zptrself = (uint32_t)((__attribute__ ((huge))karlObject_t *)&pge_mmsetup_configure);
		  judeActivatePage();
    }
}