//=============================================================================
//MANIAC MANSION Setup Utility
//=============================================================================
//
// Setup utility appliction for Maniac Mansion on the MEGA65.
// 
// Copyright (c) 2025 Daniel England, Robert Steffens.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//=============================================================================


#include "hdos.h"
#include "jude.h"
#include "jude_widgets.h"
#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "karljr.h"

#include "mmsetup_core.h"
#include "hdos.h"
#include <stdint.h>


struct BYTE4 {
  uint8_t b1, b2, b3, b4;
};

void mmsetupVewPrepare(void) {
  struct BYTE4 flags = *(struct BYTE4 *)&bootflags;
  
  if (flags.b1 == 'K' && flags.b2 == 'M' && flags.b3 > 0 && flags.b4 == 0xFF) {
    vew_mmsetup_main.actvpage = (karlFarPtr_t)(&pge_mmsetup_start);
  }

  judeDefViewPrepare();
}

void mmsetupStartExitChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    hdos_detachD81();

    //dengland FIXME Hack the Kernal do it properly check the version
    *(uint8_t *)(0x11b1) = 0;

    //clear the font, otherwise looks terrible
    *(uint8_t *)(0xd07a) = *(uint8_t *)(0xd07a) & (!0x10);

    hdos_restart();
  }
}


void mmsetupStartStrtChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_welcome);
    judeActivatePage();
  }
}

void mmsetupWelcNextChg(void) {
    uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
    judeDefCtlChange();

		if (state & STATE_DOWN) {
      configProcFlags = 0;
      
      if (ctl_mmsetup_welc_0_2._element._object.tag) 
        configProcFlags |= PROCFL_CONFIGURE;
      if (ctl_mmsetup_welc_0_3._element._object.tag) 
        configProcFlags |= PROCFL_EXTRACT;
      if (ctl_mmsetup_welc_0_4._element._object.tag) 
        configProcFlags |= PROCFL_BUILD;
      if (ctl_mmsetup_welc_0_5._element._object.tag) 
        configProcFlags |= PROCFL_VALIDATE;

      if (!(configProcFlags & ~PROCFL_CONFIGURE)) {
        ctl_mmsetup_welc_0_6.text_p = (karlFarPtr_t)str_mmsetup_welc_7;

        zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
        karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);

      } else if (configurationInvalid() && !ctl_mmsetup_welc_0_2._element._object.tag) {
        ctl_mmsetup_welc_0_6.text_p = (karlFarPtr_t)str_mmsetup_welc_6;

        zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
        karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);

      } else {
        zptrself = (uint32_t)&ctl_mmsetup_config_0_21;
        karlObjExcludeState(STATE_VISIBLE);

        zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
        karlObjExcludeState(STATE_VISIBLE);

        if (ctl_mmsetup_welc_0_2._element._object.tag) {
          zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_configure);
        } else {
          zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_process);
        }
        judeActivatePage();

        if (!ctl_mmsetup_welc_0_2._element._object.tag) {
          initiateProcess();
        }
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
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
  
  judeDefCtlChange();

  if ((state & STATE_DOWN) && configurationInvalid()) {
    ((karlObject_t __huge *)zptrself)->tag = 1;
    karlObjIncludeState(STATE_CHANGED);

    ctl_mmsetup_welc_0_6.text_p = (karlFarPtr_t)str_mmsetup_welc_6;

    zptrself = (uint32_t)&ctl_mmsetup_welc_0_6;
    karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);
  }
}

void mmsetupConfigItemChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
  
  judeDefCtlChange();

  if (state & STATE_DOWN) {

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

      lbx_mmsetup_select_0_5.currline = 0;
      lbx_mmsetup_select_0_5.linesoff = 0;
      lbx_mmsetup_select_0_5.hotline = 0;

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
      
      char __huge *lineptr = (char __huge *)(LISTBOXLINESMEM + (66 * lbx_mmsetup_select_0_5.selline));
      for (uint8_t i = 0; i < 65; i++) {
        dest_details[config_select].fileName[i] = lineptr[i];
      }
      dest_details[config_select].namelen = lineptr[65];
      
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

uint8_t bonusTheme = 2;

void mmsetupWelcThemeChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    uint8_t theme = actvtheme + 1;
    if (theme >= (themeCnt - ((bonusTheme) ? 3 : 0)))  {
      theme = 0;
      bonusTheme -= (bonusTheme) ? 1 : 0;
    }

    judeSetTheme(theme);
    ctl_mmsetup_welc_1_2.text_p = (karlFarPtr_t)judeGetThemeDesc();
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_welc_1_2);
    karlObjIncludeState(STATE_CHANGED);
  }
}

void mmsetupConfigNextChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    uint8_t invalid = configurationInvalid();
    if (invalid == 1) {
      ctl_mmsetup_config_0_21.text_p = (karlFarPtr_t)str_mmsetup_config_17;

      zptrself = (uint32_t)&ctl_mmsetup_config_0_21;
      karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);
    } else if (invalid == 2) {
      ctl_mmsetup_config_0_21.text_p = (karlFarPtr_t)str_mmsetup_config_18;

      zptrself = (uint32_t)&ctl_mmsetup_config_0_21;
      karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);
    } else if (invalid == 3) {
      ctl_mmsetup_config_0_21.text_p = (karlFarPtr_t)str_mmsetup_config_19;

      zptrself = (uint32_t)&ctl_mmsetup_config_0_21;
      karlObjIncludeState(STATE_VISIBLE | STATE_CHANGED);
    } else if (invalid == 0) {
      zptrself = (uint32_t)&ctl_mmsetup_config_0_21;
      karlObjExcludeState(STATE_VISIBLE);

      zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_process);
      judeActivatePage();

      initiateProcess();
    }
  }
}

void mmsetupProcCancelChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    if (((karlObject_t __huge *)zptrself)->tag) {
      cancelProcess();
    } else {
      zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_welcome);
      judeActivatePage();
    }
  }
}

void mmsetupProcDoneChg(void) {
  uint8_t state = ((__attribute__ ((huge))karlObject_t *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    zptrself = (uint32_t)((karlObject_t __huge *)&pge_mmsetup_welcome);
    judeActivatePage();
  }
}

void mmsetupProcContChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    continueProcess();
  }
}

void mmsetupWelcThemeLblPrep(void) {
  judeDefCtlPrepare();

  ctl_mmsetup_welc_1_2.text_p = (karlFarPtr_t)judeGetThemeDesc();
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_welc_1_2);
  karlObjIncludeState(STATE_CHANGED);
}


void mmsetupWelcExitChg(void) {
  uint8_t state = ((karlObject_t __huge *)zptrself)->state;
		    
  judeDefCtlChange();

  if (state & STATE_DOWN) {
    *(uint8_t *)(0xd07a) = *(uint8_t *)(0xd07a) & (!0x10);

    hdos_restart();
  }
}

void mmsetupWelcPgeKeypress(void) {
  uint32_t keypress = _zkarljr2.valkey;

  uint8_t mod = (keypress & 0xff00) >> 8;
  uint8_t key = (keypress & 0x00ff);

  if ((mod == 5) && (key == 75)) {
    kernal_get_status();

    while(1) {
      __asm(" inc 0xd020 ");
    }
  }
  else if ((mod == 5) && (key == 87)) {
    judeSetTheme(7);

    ctl_mmsetup_welc_1_2.text_p = (karlFarPtr_t)judeGetThemeDesc();
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_welc_1_2);
    karlObjIncludeState(STATE_CHANGED);
  }
}
