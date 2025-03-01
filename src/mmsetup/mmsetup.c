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


#include "karljr.h"
#include "jude.h"
#include "jude_widgets.h"
#include "mmsetup.h"
#include "mmsetup_ui.h"
#include "mmsetup_core.h"


judeControl_t ctl_mmsetup_config_0_5 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		3,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		11,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_5),
		0,
		0x0,
	  KEY_OF_MODKEY(KEY_M65_SYS_4)};

karlFarPtr_t pnl_mmsetup_welc_1_controls[] = {
		NEARTOFARPTRREC(&ctl_mmsetup_welc_1_0),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_1_1),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_1_2),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_1_3)};

karlModule_t mod_mmsetup_app = {
		sizeof(karlModule_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(karlDefModPrepare),
		NEARTOEVENTPTR(karlDefModInit),
		NEARTOEVENTPTR(karlDefModChange),
		NEARTOEVENTPTR(karlDefModRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		"                ",
		NEARTOFARPTRREC(mod_mmsetup_app_units),
		1};

judeControl_t ctl_mmsetup_config_0_6 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		4,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		13,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_6),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_5)};

char str_mmsetup_config_7[] = "6  D64 disk #2...";

char str_mmsetup_config_10[] = "Destination:";

char str_mmsetup_config_11[] = "Sources:";

judeLayer_t lay_mmsetup_bkg = {
		sizeof(judeLayer_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefLyrPrepare),
		NEARTOEVENTPTR(judeDefLyrInit),
		NEARTOEVENTPTR(judeDefLyrChange),
		NEARTOEVENTPTR(judeDefLyrRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		80,
		0,
		0,
		0};

judePanel_t pnl_mmsetup_welc_0 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_welcome),
		CLR_INSET,
		0,
		0,
		80,
		23,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_welc_0_controls),
		7};

char str_mmsetup_config_13[] = "<Please Select>";

judeControl_t ctl_mmsetup_config_0_15 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		9,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_config_0_8 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0,
		0x0000,
		0x0000,
		6,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		17,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_8),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_7)};

char str_mmsetup_config_2[] = "1  D81 disk #1...";

char str_mmsetup_buttons_1[] = "[Cancel  ]";

judeControl_t ctl_mmsetup_config_0_19 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		17,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_14),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_welc_0_3 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0020,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_FACE,
		20,
		7,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_3),
		0,
		0x2,
		KEY_OF_MODKEY(KEY_M65_SYS_E)};

judeControl_t ctl_mmsetup_config_0_12 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_PAPER,
		0,
		17,
		18,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_12),
		0,
		0xff,
		0};

judePage_t pge_mmsetup_start = {
		sizeof(judePage_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPgePrepare),
		NEARTOEVENTPTR(judeDefPgeInit),
		NEARTOEVENTPTR(judeDefPgeChange),
		NEARTOEVENTPTR(judeDefPgeRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPgePresent),
		NEARTOEVENTPTR(mmsetupWelcPgeKeypress),
		NEARTOFARPTRREC(&vew_mmsetup_main),
		CLR_INSET,
		0,
		0,
		80,
		25,
		FARPTRNULLREC,
		FARPTRNULLREC,
		FARPTRNULLREC,
		0,
		NEARTOFARPTRREC(pge_mmsetup_start_panels),
		2};

karlFarPtr_t pge_mmsetup_start_panels[] = {
		NEARTOFARPTRREC(&pnl_mmsetup_start_1),
		NEARTOFARPTRREC(&pnl_mmsetup_start_0)};

judePanel_t pnl_mmsetup_start_0 = {
  sizeof(judePanel_t),
  NEARTOFARPTRREC(&uni_mmsetup_ui),
  NEARTOEVENTPTR(judeDefPnlPrepare),
  NEARTOEVENTPTR(judeDefPnlInit),
  NEARTOEVENTPTR(judeDefPnlChange),
  NEARTOEVENTPTR(judeDefPnlRelease),
  0x0003,
  0x0000,
  0x0000,
  0x0a,
  NEARTOEVENTPTR(judeDefPnlPresent),
  EVENTPTRNULLREC,
  NEARTOFARPTRREC(&pge_mmsetup_start),
  CLR_INSET,
  0,
  0,
  80,
  23,
  NEARTOFARPTRREC(&lay_mmsetup_bkg),
  NEARTOFARPTRREC(pnl_mmsetup_start_0_controls),
  9};

karlFarPtr_t pnl_mmsetup_start_0_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_0),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_1),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_2),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_3),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_4),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_5),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_6),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_7),
  NEARTOFARPTRREC(&ctl_mmsetup_start_0_8)
};
    
judeControl_t ctl_mmsetup_start_0_0 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_FOCUS,
  0,
  0,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_0),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_1 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_PAPER,
  0,
  1,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_1),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_2 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  5,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_2),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_3 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  6,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_3),
  0,
  0xff,
  0};
  
judeControl_t ctl_mmsetup_start_0_4 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  7,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_4),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_5 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  8,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_5),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_6 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  10,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_6),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_7 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED | STATE_VISIBLE,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_TEXT,
  0,
  11,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_7),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_start_0_8 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0,
  0x0000,
  0x0000,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_0),
  CLR_FACE,
  20,
  14,
  20,
  1,
  NEARTOFARPTRREC(str_mmsetup_start_8),
  0,
  0x2,
  KEY_OF_MODKEY(KEY_M65_SYS_U)};




judePanel_t pnl_mmsetup_start_1 = {
  sizeof(judePanel_t),
  NEARTOFARPTRREC(&uni_mmsetup_ui),
  NEARTOEVENTPTR(judeDefPnlPrepare),
  NEARTOEVENTPTR(judeDefPnlInit),
  NEARTOEVENTPTR(judeDefPnlChange),
  NEARTOEVENTPTR(judeDefPnlRelease),
  0x0003,
  0x0000,
  0x0000,
  0,
  NEARTOEVENTPTR(judeDefPnlPresent),
  EVENTPTRNULLREC,
  NEARTOFARPTRREC(&pge_mmsetup_start),
  CLR_INSET,
  0,
  23,
  80,
  2,
  NEARTOFARPTRREC(&lay_mmsetup_bkg),
  NEARTOFARPTRREC(pnl_mmsetup_start_1_controls),
  2};


karlFarPtr_t pnl_mmsetup_start_1_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_start_1_0),
  NEARTOFARPTRREC(&ctl_mmsetup_start_1_1)
};

judeControl_t ctl_mmsetup_start_1_0 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(mmsetupStartStrtChg),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0000,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_1),
  CLR_ACCEPT,
  70,
  24,
  10,
  1,
  NEARTOFARPTRREC(str_mmsetup_buttons_7),
  0,
  0x1,
  KEY_OF_MODKEY(KEY_M65_SYS_S)};


judeControl_t ctl_mmsetup_start_1_1 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(mmsetupStartExitChg),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0000,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_start_1),
  CLR_ABORT,
  0,
  24,
  10,
  1,
  NEARTOFARPTRREC(str_mmsetup_buttons_6),
  0,
  0xff,
  KEY_OF_MODKEY(KEY_M65_SYS_ESC)
};



judePage_t pge_mmsetup_welcome = {
		sizeof(judePage_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPgePrepare),
		NEARTOEVENTPTR(judeDefPgeInit),
		NEARTOEVENTPTR(judeDefPgeChange),
		NEARTOEVENTPTR(judeDefPgeRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPgePresent),
		NEARTOEVENTPTR(mmsetupWelcPgeKeypress),
		NEARTOFARPTRREC(&vew_mmsetup_main),
		CLR_INSET,
		0,
		0,
		80,
		25,
		FARPTRNULLREC,
		FARPTRNULLREC,
		FARPTRNULLREC,
		0,
		NEARTOFARPTRREC(pge_mmsetup_welcome_panels),
		2};

karlFarPtr_t vew_mmsetup_main_pages[] = {
		NEARTOFARPTRREC(&pge_mmsetup_welcome),
		NEARTOFARPTRREC(&pge_mmsetup_configure),
    NEARTOFARPTRREC(&pge_mmsetup_select),
    NEARTOFARPTRREC(&pge_mmsetup_process),
    NEARTOFARPTRREC(&pge_mmsetup_start)};

judeUInterface_t uni_mmsetup_ui = {
		sizeof(judeUInterface_t),
		NEARTOFARPTRREC(&mod_mmsetup_app),
		NEARTOEVENTPTR(judeDefUIPrepare),
		NEARTOEVENTPTR(judeDefUIInit),
		NEARTOEVENTPTR(judeDefUIChange),
		NEARTOEVENTPTR(judeDefUIRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		"                ",
		DWRDTOFARPTRREC(0x00013000),
		DWRDTOFARPTRREC(0x00013200),
		1,
		NEARTOFARPTRREC(uni_mmsetup_ui_views),
		1};

judeView_t vew_mmsetup_main = {
		sizeof(judeView_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(mmsetupVewPrepare),
		NEARTOEVENTPTR(judeDefViewInit),
		NEARTOEVENTPTR(judeDefViewChange),
		NEARTOEVENTPTR(judeDefViewRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		80,
		25,
		DWRDTOFARPTRREC(0x00012000),
		2,
		NEARTOFARPTRREC(vew_mmsetup_main_layers),
		1,
		NEARTOFARPTRREC(vew_mmsetup_main_bars),
		0,
		NEARTOFARPTRREC(&pge_mmsetup_welcome),
		NEARTOFARPTRREC(vew_mmsetup_main_pages),
		5,
		0};

char str_mmsetup_welc_1[] = "Select the operations you wish to perform.";

judeControl_t ctl_mmsetup_config_0_16 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		11,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};

judePanel_t pnl_mmsetup_config_1 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_configure),
		CLR_INSET,
		0,
		23,
		80,
		2,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_config_1_controls),
		2};

judeControl_t ctl_mmsetup_config_1_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigCancelChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_1),
		CLR_ABORT,
		0,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_1),
		0,
		0xff,
		KEY_OF_MODKEY(KEY_M65_SYS_ESC)};

judeControl_t ctl_mmsetup_welc_0_5 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE | STATE_ENABLED,
		0x0000,
		0x0020,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_FACE,
		20,
		11,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_5),
		0,
		0x2,
		KEY_OF_MODKEY(KEY_M65_SYS_V)};

judeControl_t ctl_mmsetup_welc_0_6 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_ENABLED,
		0x0000,
		OPT_NONAVIGATE,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_SYSS_TEXT | 0x0A,
		0,
		3,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_6),
		0,
		0xff,
		0};


judeControl_t ctl_mmsetup_config_0_10 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_PAPER,
		0,
		5,
		18,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_10),
		0,
		0xff,
		0};

karlFarPtr_t pnl_mmsetup_config_0_controls[] = {
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_0),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_1),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_2),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_3),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_4),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_5),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_6),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_7),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_8),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_9),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_10),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_11),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_12),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_13),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_14),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_15),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_16),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_17),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_18),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_19),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_20),
		NEARTOFARPTRREC(&ctl_mmsetup_config_0_21)};

judeControl_t ctl_mmsetup_config_0_21 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
  CLR_SYSS_TEXT | 0x0A,
  0,
  3,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_welc_6),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_config_0_2 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		5,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_2),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_1)};

char str_mmsetup_config_14[] = "<All>";

karlFarPtr_t pnl_mmsetup_config_1_controls[] = {
		NEARTOFARPTRREC(&ctl_mmsetup_config_1_0),
		NEARTOFARPTRREC(&ctl_mmsetup_config_1_1)};

judeControl_t ctl_mmsetup_welc_0_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_PAPER,
		0,
		1,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_1),
		0,
		0xff,
		0};

karlFarPtr_t pge_mmsetup_configure_panels[] = {
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		NEARTOFARPTRREC(&pnl_mmsetup_config_1)};

char str_mmsetup_welc_5[] = "  Validate disks";

char str_mmsetup_welc_4[] = "  Build SID sounds";

char str_mmsetup_welc_2[] = "  Configure";

char str_mmsetup_buttons_0[] = "[Next >  ]";

char str_mmsetup_welc_6[] = "You must perform Configuration at this time.";
char str_mmsetup_welc_7[] = "You must perform at least one Data Operation.";

judePanel_t pnl_mmsetup_config_0 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_configure),
		CLR_INSET,
		0,
		0,
		80,
		23,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_config_0_controls),
		22};

char str_mmsetup_config_8[] = "7  Disk #1 rooms...";

char str_mmsetup_config_4[] = "3  ADF disk #1...";

judeControl_t ctl_mmsetup_welc_0_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_FOCUS,
		0,
		0,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_0),
		0,
		0xff,
		0};

char str_mmsetup_welc_3[] = "  Extract rooms";

char str_mmsetup_welc_0[] = "Maniac Mansion setup utility...";

char str_mmsetup_config_0[] = "Setup configuration";

judeControl_t ctl_mmsetup_config_1_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigNextChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_1),
		CLR_ACCEPT,
		70,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_0),
		0,
		0x1,
    KEY_OF_MODKEY(KEY_M65_SYS_N)};

judeControl_t ctl_mmsetup_welc_0_2 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupWelcConfigChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0020,
		1,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_FACE,
		20,
		5,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_2),
		0,
		0x2,
		KEY_OF_MODKEY(KEY_M65_SYS_C)};

char str_mmsetup_config_5[] = "4  ADF disk #2...";

judeControl_t ctl_mmsetup_config_0_7 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		5,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		15,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_7),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_6)};

judeControl_t ctl_mmsetup_config_0_20 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		19,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_14),
		0,
		0xff,
		0};

karlFarPtr_t pnl_mmsetup_welc_0_controls[] = {
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_0),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_1),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_2),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_3),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_4),
		NEARTOFARPTRREC(&ctl_mmsetup_welc_0_5),
    NEARTOFARPTRREC(&ctl_mmsetup_welc_0_6)};

judeControl_t ctl_mmsetup_config_0_3 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		1,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		7,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_3),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_2)};

char str_mmsetup_config_6[] = "5  D64 disk #1...";

judeControl_t ctl_mmsetup_config_0_11 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_PAPER,
		0,
		9,
		18,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_11),
		0,
		0xff,
		0};

karlFarPtr_t pge_mmsetup_welcome_panels[] = {
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_1)};

judeControl_t ctl_mmsetup_config_0_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FOCUS,
		0,
		0,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_0),
		0,
		0xff,
		0};

char str_mmsetup_config_3[] = "2  D81 disk #2...";

judeControl_t ctl_mmsetup_config_0_14 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		7,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_config_0_17 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		13,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_config_0_18 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		15,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_config_0_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_PAPER,
		0,
		1,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_1),
		0,
		0xff,
		0};

char str_mmsetup_config_1[] = "Select disks and files for processing.";

karlFarPtr_t vew_mmsetup_main_layers[] = {
		NEARTOFARPTRREC(&lay_mmsetup_bkg)};

judeControl_t ctl_mmsetup_welc_0_4 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0020,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_0),
		CLR_FACE,
		20,
		9,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_welc_4),
		0,
		0x2,
		KEY_OF_MODKEY(KEY_M65_SYS_B)};

char str_mmsetup_config_9[] = "8  Disk #2 rooms...";

karlFarPtr_t vew_mmsetup_main_bars[] = {
		0};

char str_mmsetup_config_12[] = "Filters:";

karlFarPtr_t uni_mmsetup_ui_views[] = {
		NEARTOFARPTRREC(&vew_mmsetup_main)};

judeControl_t ctl_mmsetup_welc_1_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupWelcNextChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_1),
		CLR_ACCEPT,
		70,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_0),
		0,
		0x1,
		KEY_OF_MODKEY(KEY_M65_SYS_N)};

judeControl_t ctl_mmsetup_welc_1_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupWelcThemeChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_1),
		CLR_INFORM,
		12,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_3),
		0,
		0x1,
		KEY_OF_MODKEY(KEY_M65_SYS_T)};

judeControl_t ctl_mmsetup_welc_1_3 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupWelcExitChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_1),
		CLR_ABORT,
		0,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_6),
		0,
		0xff,
		KEY_OF_MODKEY(KEY_M65_SYS_ESC)
};

judeControl_t ctl_mmsetup_welc_1_2 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(mmsetupWelcThemeLblPrep),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE | STATE_ENABLED,
		0x0000,
		OPT_NONAVIGATE,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_welc_1),
		CLR_TEXT,
		24,
		24,
		16,
		1,
		FARPTRNULLREC,
		0,
		0xff,
		0x0};
    

judeControl_t ctl_mmsetup_config_0_9 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0,
		0x0000,
		0x0000,
		7,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		19,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_9),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_8)};

judeControl_t ctl_mmsetup_config_0_4 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupConfigItemChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		2,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_FACE,
		20,
		9,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_4),
		0,
		0x0,
		KEY_OF_MODKEY(KEY_M65_SYS_3)};

karlFarPtr_t mod_mmsetup_app_units[] = {
		NEARTOFARPTRREC(&uni_mmsetup_ui)};

judePage_t pge_mmsetup_configure = {
		sizeof(judePage_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPgePrepare),
		NEARTOEVENTPTR(judeDefPgeInit),
		NEARTOEVENTPTR(judeDefPgeChange),
		NEARTOEVENTPTR(judeDefPgeRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPgePresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&vew_mmsetup_main),
		CLR_EMPTY,
		0,
		0,
		80,
		25,
		FARPTRNULLREC,
		FARPTRNULLREC,
		FARPTRNULLREC,
		0,
		NEARTOFARPTRREC(pge_mmsetup_configure_panels),
		2};

judePanel_t pnl_mmsetup_welc_1 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_welcome),
		CLR_INSET,
		0,
		23,
		80,
		2,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_welc_1_controls),
		4};

judeControl_t ctl_mmsetup_config_0_13 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_config_0),
		CLR_TEXT,
		42,
		5,
		38,
		1,
		NEARTOFARPTRREC(str_mmsetup_config_13),
		0,
		0xff,
		0};


judePage_t pge_mmsetup_select = {
		sizeof(judePage_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPgePrepare),
		NEARTOEVENTPTR(judeDefPgeInit),
		NEARTOEVENTPTR(judeDefPgeChange),
		NEARTOEVENTPTR(judeDefPgeRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPgePresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&vew_mmsetup_main),
		CLR_EMPTY,
		0,
		0,
		80,
		25,
		FARPTRNULLREC,
		FARPTRNULLREC,
		FARPTRNULLREC,
		0,
		NEARTOFARPTRREC(pge_mmsetup_select_panels),
		2};

karlFarPtr_t pge_mmsetup_select_panels[] = {
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		NEARTOFARPTRREC(&pnl_mmsetup_select_1)};


judePanel_t pnl_mmsetup_select_0 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_select),
		CLR_INSET,
		0,
		0,
		80,
		23,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_select_0_controls),
		6};


karlFarPtr_t pnl_mmsetup_select_0_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_select_0_0),
  NEARTOFARPTRREC(&ctl_mmsetup_select_0_1),
  NEARTOFARPTRREC(&ctl_mmsetup_select_0_2),
  NEARTOFARPTRREC(&ctl_mmsetup_select_0_3),
  NEARTOFARPTRREC(&ctl_mmsetup_select_0_4),
  NEARTOFARPTRREC(&lbx_mmsetup_select_0_5)
};

judeControl_t ctl_mmsetup_select_0_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_FOCUS,
		0,
		0,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_select_0),
		0,
		0xff,
		0};


judeControl_t ctl_mmsetup_select_0_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_PAPER,
		0,
		1,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_select_1),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_select_0_2 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupRealDiskChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		OPT_AUTOCHECK,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_FACE,
		20,
		5,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_select_2),
		0,
		0x6,
		KEY_OF_MODKEY(KEY_M65_SYS_R)};

judeControl_t ctl_mmsetup_select_0_3 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupSelNoneChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE | STATE_ENABLED,
		0x0000,
		OPT_AUTOCHECK,
    0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_FACE,
		20,
		7,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_select_3),
		0,
		0x9,
		KEY_OF_MODKEY(KEY_M65_SYS_N)};

judeControl_t ctl_mmsetup_select_0_4 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_FACE,
		20,
		9,
		20,
		1,
		NEARTOFARPTRREC(str_mmsetup_select_4),
		0,
		0x9,
		KEY_OF_MODKEY(KEY_M65_SYS_A)};


judeListBox_t lbx_mmsetup_select_0_5 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeLBxChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		OPT_AUTOTRACK,
		0,
		NEARTOEVENTPTR(judeLBxPresent),
		NEARTOEVENTPTR(judeLBxPressed),
		NEARTOFARPTRREC(&pnl_mmsetup_select_0),
		CLR_TEXT,
		42,
		5,
		38,
		15,
		FARPTRNULLREC,
		0,
		0x0,
		0,
		NEARTOEVENTPTR(mmsetupListSelect),
		DWRDTOFARPTRREC(LISTBOXLINESMEM),
		0x00,
		66,
		0x00,
		0x00,
		0x00,
		0xFF
  };


judePanel_t pnl_mmsetup_select_1 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_select),
		CLR_INSET,
		0,
		23,
		80,
		2,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_select_1_controls),
		2};


karlFarPtr_t pnl_mmsetup_select_1_controls[] = {
		NEARTOFARPTRREC(&ctl_mmsetup_select_1_0),
		NEARTOFARPTRREC(&ctl_mmsetup_select_1_1)};


judeControl_t ctl_mmsetup_select_1_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupSelAcceptChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_1),
		CLR_ACCEPT,
		70,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_2),
		0,
		0x0,
		0};


judeControl_t ctl_mmsetup_select_1_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupSelectCancelChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_select_1),
		CLR_ABORT,
		0,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_1),
		0,
		0xff,
		KEY_OF_MODKEY(KEY_M65_SYS_ESC)};


judePage_t pge_mmsetup_process = {
		sizeof(judePage_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPgePrepare),
		NEARTOEVENTPTR(judeDefPgeInit),
		NEARTOEVENTPTR(judeDefPgeChange),
		NEARTOEVENTPTR(judeDefPgeRelease),
		0x0003,
		0x0000,
		0x0000,
		0x0a,
		NEARTOEVENTPTR(judeDefPgePresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&vew_mmsetup_main),
		CLR_INSET,
		0,
		0,
		80,
		25,
		FARPTRNULLREC,
		FARPTRNULLREC,
		FARPTRNULLREC,
		0,
		NEARTOFARPTRREC(pge_mmsetup_proc_panels),
		2};

karlFarPtr_t pge_mmsetup_proc_panels[] = {
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_1)};


judePanel_t pnl_mmsetup_proc_0 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_process),
		CLR_INSET,
		0,
		0,
		80,
		23,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_proc_0_controls),
		12};

judeControl_t ctl_mmsetup_proc_0_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
		CLR_FOCUS,
		0,
		0,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_proc_0),
		0,
		0xff,
		0};


judeControl_t ctl_mmsetup_proc_0_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeDefCtlChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0002,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
		CLR_PAPER,
		0,
		1,
		80,
		1,
		NEARTOFARPTRREC(str_mmsetup_proc_1),
		0,
		0xff,
		0};

judeControl_t ctl_mmsetup_proc_0_2 = {    //Prompt
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  STATE_ENABLED,
  0x0000,
  OPT_NONAVIGATE,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_SYSS_TEXT | 0x0A,
  0,
  3,
  80,
  1,
  NEARTOFARPTRREC(str_mmsetup_proc_4),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_proc_0_3 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_PAPER,
  0,
  5,
  18,
  1,
  NEARTOFARPTRREC(str_mmsetup_proc_5),
  0,
  0xff,
  0};

judeProgressBar_t pgb_mmsetup_proc_0_4 = {    //Process progress bar
  sizeof(judeProgressBar_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judePrgBarPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_SHADOW,
  20,
  5,
  20,
  1,
  FARPTRNULLREC,
  0,
  0xff,
  0,

  0,
  0,
  0,
  0,
  0,
  0
};


judeControl_t ctl_mmsetup_proc_0_5 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_PAPER,
  0,
  7,
  18,
  1,
  NEARTOFARPTRREC(str_mmsetup_proc_6),
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_proc_0_6 = {    //Read counter
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_TEXT,
  20,
  7,
  20,
  1,
  FARPTRNULLREC,
  0,
  0xff,
  0};

judeControl_t ctl_mmsetup_proc_0_7 = {
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_PAPER,
  0,
  9,
  18,
  1,
  NEARTOFARPTRREC(str_mmsetup_proc_7),
  0,
  0xff,
  0};

judeProgressBar_t pgb_mmsetup_proc_0_8 = {    //write progress
  sizeof(judeProgressBar_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judePrgBarPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_SHADOW,
  20,
  9,
  20,
  1,
  FARPTRNULLREC,
  0,
  0xff,
  0,

  0,
  0,
  0,
  0,
  0,
  0
};

judeControl_t ctl_mmsetup_proc_0_9 = {
    sizeof(judeControl_t),
    FARPTRNULLREC,
    NEARTOEVENTPTR(judeDefCtlPrepare),
    NEARTOEVENTPTR(judeDefCtlInit),
    NEARTOEVENTPTR(judeDefCtlChange),
    NEARTOEVENTPTR(judeDefCtlRelease),
    0x0003,
    0x0000,
    0x0002,
    0,
    NEARTOEVENTPTR(judeDefCtlPresent),
    NEARTOEVENTPTR(judeDefCtlKeypress),
    NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
    CLR_PAPER,
    0,
    11,
    18,
    1,
    NEARTOFARPTRREC(str_mmsetup_proc_8),
    0,
    0xff,
    0};
  
judeControl_t ctl_mmsetup_proc_0_10 = {    //error count
  sizeof(judeControl_t),
  FARPTRNULLREC,
  NEARTOEVENTPTR(judeDefCtlPrepare),
  NEARTOEVENTPTR(judeDefCtlInit),
  NEARTOEVENTPTR(judeDefCtlChange),
  NEARTOEVENTPTR(judeDefCtlRelease),
  0x0003,
  0x0000,
  0x0002,
  0,
  NEARTOEVENTPTR(judeDefCtlPresent),
  NEARTOEVENTPTR(judeDefCtlKeypress),
  NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
  CLR_SYSS_TEXT | 0x0A,
  20,
  11,
  20,
  1,
  FARPTRNULLREC,
  0,
  0xff,
  0};

judeListBox_t lbx_mmsetup_proc_0_11 = {   //output list
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(judeLBxChange),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		OPT_AUTOTRACK,
		0,
		NEARTOEVENTPTR(judeLBxPresent),
		NEARTOEVENTPTR(judeLBxPressed),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_0),
		CLR_TEXT,
		42,
		5,
		38,
		15,
		FARPTRNULLREC,
		0,
		0x0,
		0,
		EVENTPTRNULLREC,
		DWRDTOFARPTRREC(LISTBOXLINESMEM),
		0x00,
		40,
		0x00,
		0x00,
		0x00,
		0xFF
  };


karlFarPtr_t pnl_mmsetup_proc_0_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_0),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_1),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_2),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_3),
  NEARTOFARPTRREC(&pgb_mmsetup_proc_0_4),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_5),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_6),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_7),
  NEARTOFARPTRREC(&pgb_mmsetup_proc_0_8),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_9),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_0_10),
  NEARTOFARPTRREC(&lbx_mmsetup_proc_0_11)
};


judePanel_t pnl_mmsetup_proc_1 = {
		sizeof(judePanel_t),
		NEARTOFARPTRREC(&uni_mmsetup_ui),
		NEARTOEVENTPTR(judeDefPnlPrepare),
		NEARTOEVENTPTR(judeDefPnlInit),
		NEARTOEVENTPTR(judeDefPnlChange),
		NEARTOEVENTPTR(judeDefPnlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefPnlPresent),
		EVENTPTRNULLREC,
		NEARTOFARPTRREC(&pge_mmsetup_welcome),
		CLR_INSET,
		0,
		23,
		80,
		2,
		NEARTOFARPTRREC(&lay_mmsetup_bkg),
		NEARTOFARPTRREC(pnl_mmsetup_proc_1_controls),
		3};

judeControl_t ctl_mmsetup_proc_1_0 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupProcDoneChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_1),
		CLR_ACCEPT,
		70,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_4),
		0,
		0x1,
		KEY_OF_MODKEY(KEY_M65_SYS_D)};

judeControl_t ctl_mmsetup_proc_1_1 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupProcContChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		STATE_VISIBLE,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_1),
		CLR_INFORM,
		58,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_5),
		0,
		0x1,
		KEY_OF_MODKEY(KEY_M65_SYS_C)};

judeControl_t ctl_mmsetup_proc_1_2 = {
		sizeof(judeControl_t),
		FARPTRNULLREC,
		NEARTOEVENTPTR(judeDefCtlPrepare),
		NEARTOEVENTPTR(judeDefCtlInit),
		NEARTOEVENTPTR(mmsetupProcCancelChg),
		NEARTOEVENTPTR(judeDefCtlRelease),
		0x0003,
		0x0000,
		0x0000,
		0,
		NEARTOEVENTPTR(judeDefCtlPresent),
		NEARTOEVENTPTR(judeDefCtlKeypress),
		NEARTOFARPTRREC(&pnl_mmsetup_proc_1),
		CLR_ABORT,
		0,
		24,
		10,
		1,
		NEARTOFARPTRREC(str_mmsetup_buttons_1),
		0,
		0xff,
		KEY_OF_MODKEY(KEY_M65_SYS_ESC)};


karlFarPtr_t pnl_mmsetup_proc_1_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_proc_1_0),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_1_1),
  NEARTOFARPTRREC(&ctl_mmsetup_proc_1_2)};


char str_mmsetup_start_0[] = "MANIAC MANSION could NOT run";
char str_mmsetup_start_1[] = "You have entered the setup utility.";

char str_mmsetup_start_2[] = "The data files from the original games are required in order to play the game on";
char str_mmsetup_start_3[] = "your MEGA65.  You will need the image files from the Amiga disks and optionally";
char str_mmsetup_start_4[] = "the C64 disks in order to create the disks or images you need for your MEGA65.";
char str_mmsetup_start_5[] = "You may do this by using this utility.  To begin, click the Start button below.";

char str_mmsetup_start_6[] = "If you do not wish to continue, you may unmount any disk and exit the utility by";
char str_mmsetup_start_7[] = "clicking the Exit button or by pressing ESC.";

char str_mmsetup_start_8[] = "  Unmount";

char str_mmsetup_select_0[] = "Make selection";
char str_mmsetup_select_1[] = "Browse for files and make selections.";
char str_mmsetup_select_2[] = "  Use real disk";
char str_mmsetup_select_3[] = "  Select None";
char str_mmsetup_select_4[] = "  Select All";

char str_mmsetup_buttons_2[] = "[Accept  ]";
char str_mmsetup_buttons_3[] = "[Theme   ]";
char str_mmsetup_buttons_4[] = "[Done    ]";
char str_mmsetup_buttons_5[] = "[Continue]";
char str_mmsetup_buttons_6[] = "[Exit    ]";
char str_mmsetup_buttons_7[] = "[Start   ]";

char str_mmsetup_config_15[] = "<Real Disk>";
char str_mmsetup_config_16[] = "<No Selection>";

char str_mmsetup_config_17[] = "You must provide the D81/ADF Extraction details.";
char str_mmsetup_config_18[] = "You must provide the D64 Build details.";
char str_mmsetup_config_19[] = "You must provide the D81 Source details.";


char str_mmsetup_proc_0[] = "Performing Data Operations";
char str_mmsetup_proc_1[] = "Extracting/Building room files for destination disks...";
char str_mmsetup_proc_2[] = "Building SID sounds from D64s...";
char str_mmsetup_proc_3[] = "Validating D81 disk images...";
char str_mmsetup_proc_4[] = "Please insert the required disk and select Continue.";
char str_mmsetup_proc_5[] = "Progress:";
char str_mmsetup_proc_6[] = "Reading:";
char str_mmsetup_proc_7[] = "Writing:";
char str_mmsetup_proc_8[] = "Errors:";
char str_mmsetup_proc_9[] = "  -   F O U N D   - ";
char str_mmsetup_proc_10[] ="  -    N O N E    - ";


karlFarPtr_t config_desc_controls[] = {
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_13),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_14),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_15),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_16),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_17),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_18),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_19),
  NEARTOFARPTRREC(&ctl_mmsetup_config_0_20)};

