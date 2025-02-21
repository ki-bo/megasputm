#include "jude.h"
#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "karljr.h"

#include "mmsetup_core.h"


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

//const char blah[] = "BLAH BLAH BLAH     0123456789012345678901234567890123456789123"; 

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

      uint8_t tag = ((karlObject_t *)(zptrself))->tag;
      uint8_t tag1 = tag >> 1;

      //restore selection
      if (dest_details[tag].type == SETTINGT_DISK) {
        ctl_mmsetup_select_0_2._element._object.tag = 1;
        ctl_mmsetup_select_0_3._element._object.tag = 0;
      } else if (dest_details[tag].type == SETTINGT_NONE) {
        ctl_mmsetup_select_0_2._element._object.tag = 0;
        ctl_mmsetup_select_0_3._element._object.tag = 1;
      } else {
        ctl_mmsetup_select_0_2._element._object.tag = 0;
        ctl_mmsetup_select_0_3._element._object.tag = 0;
      }

      zptrself = (uint32_t)((void __huge *)&ctl_mmsetup_select_0_2);
      if (tag1 >= 1) {
        karlObjExcludeState(STATE_ENABLED);
      } else {
        karlObjIncludeState(STATE_ENABLED);
      }

      lbx_mmsetup_select_0_5.linescnt = readDirectoryFiles(file_extensions[tag1]);

      //restore selection - we're assuming this works!
      if (dest_details[tag].type == SETTINGT_IMGE) {
        lbx_mmsetup_select_0_5.selline = dest_details[tag].index;
      } else {
        lbx_mmsetup_select_0_5.selline = 0xff;
      }

      config_select = tag;

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

void mmsetupRealDiskChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    uint8_t tag = 1;
    ((karlObject_t *)(zptrself))->tag = tag;

    if (tag && lbx_mmsetup_select_0_5.linescnt > 0) {
      ctl_mmsetup_select_0_3._element._object.tag = 0;
		  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_select_0_3);
      karlObjIncludeState(STATE_CHANGED);

      lbx_mmsetup_select_0_5.selline = 0xff;
      //lbx_mmsetup_select_0_5.linesoff = 0x00;
		  zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_select_0_5);
      karlObjIncludeState(STATE_CHANGED);
    }
  }
}

void mmsetupSelNoneChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    uint8_t tag = 1;
    ((karlObject_t *)(zptrself))->tag = tag;

    if (tag && lbx_mmsetup_select_0_5.linescnt > 0) {
      ctl_mmsetup_select_0_2._element._object.tag = 0;
		  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_select_0_2);
      karlObjIncludeState(STATE_CHANGED);

      lbx_mmsetup_select_0_5.selline = 0xff;
      //lbx_mmsetup_select_0_5.linesoff = 0x00;
		  zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_select_0_5);
      karlObjIncludeState(STATE_CHANGED);
    }
  }
}

void mmsetupListSelect(void) {
  ctl_mmsetup_select_0_2._element._object.tag = 0;
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_select_0_2);
  karlObjIncludeState(STATE_CHANGED);

  ctl_mmsetup_select_0_3._element._object.tag = 0;
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_select_0_3);
  karlObjIncludeState(STATE_CHANGED);
}


void mmsetupSelAcceptChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    //judeControl_t __huge * ctrl = ((judeControl_t __huge *)&(config_desc_controls[config_select]));
    karlFarPtr_t text;

    if (ctl_mmsetup_select_0_3._element._object.tag) {
      dest_details[config_select].type = SETTINGT_NONE;
      text = (karlFarPtr_t)(str_mmsetup_config_16);
    } else if (ctl_mmsetup_select_0_2._element._object.tag) {
      dest_details[config_select].type = SETTINGT_DISK;
      text = (karlFarPtr_t)(str_mmsetup_config_15);
    } else {
      dest_details[config_select].type = SETTINGT_IMGE;
      dest_details[config_select].index = lbx_mmsetup_select_0_5.selline;
      
      char __huge *lineptr = (char __huge *)(LISTBOXLINESMEM + (65 * lbx_mmsetup_select_0_5.selline));
      for (uint8_t i = 0; i < 65; i++) {
        dest_details[config_select].fileName[i] = lineptr[i];
      }
      text = (karlFarPtr_t)(dest_details[config_select].fileName);
    }

    //doesn't work
    //ctrl->text_p = text;

    switch (config_select) {
      case 0:
        ctl_mmsetup_config_0_13.text_p = text;
        break;
      case 1:
        ctl_mmsetup_config_0_14.text_p = text;
        break;
      case 2:
        ctl_mmsetup_config_0_15.text_p = text;
        break;
      case 3:
        ctl_mmsetup_config_0_16.text_p = text;
        break;
      case 4:
        ctl_mmsetup_config_0_17.text_p = text;
        break;
      case 5:
        ctl_mmsetup_config_0_18.text_p = text;
        break;
      default: 
        ;
    }


    //zptrself = (uint32_t)((karlObject_t __huge *)ctrl);
    //karlObjIncludeState(STATE_CHANGED);

    zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_configure);
    judeActivatePage();
  }
}