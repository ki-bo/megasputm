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


#pragma once

#include "jude_widgets.h"
#include "karljr.h"
#include "jude.h"


extern karlModule_t mod_mmsetup_app;

extern karlFarPtr_t mod_mmsetup_app_units[];

extern judeUInterface_t uni_mmsetup_ui;

extern karlFarPtr_t uni_mmsetup_ui_views[];

extern judeLayer_t lay_mmsetup_bkg;

extern judeView_t vew_mmsetup_main;

extern karlFarPtr_t vew_mmsetup_main_layers[];

extern karlFarPtr_t vew_mmsetup_main_bars[];

extern karlFarPtr_t vew_mmsetup_main_pages[];

extern judePage_t pge_mmsetup_start;
extern karlFarPtr_t pge_mmsetup_start_panels[];

extern judePanel_t pnl_mmsetup_start_0;
extern karlFarPtr_t pnl_mmsetup_start_0_controls[];

extern judeControl_t ctl_mmsetup_start_0_0;
extern judeControl_t ctl_mmsetup_start_0_1;
extern judeControl_t ctl_mmsetup_start_0_2;
extern judeControl_t ctl_mmsetup_start_0_3;
extern judeControl_t ctl_mmsetup_start_0_4;
extern judeControl_t ctl_mmsetup_start_0_5;
extern judeControl_t ctl_mmsetup_start_0_6;
extern judeControl_t ctl_mmsetup_start_0_7;
extern judeControl_t ctl_mmsetup_start_0_8;

extern judePanel_t pnl_mmsetup_start_1;
extern karlFarPtr_t pnl_mmsetup_start_1_controls[];

extern judeControl_t ctl_mmsetup_start_1_0;
extern judeControl_t ctl_mmsetup_start_1_1;

extern judePage_t pge_mmsetup_welcome;
extern karlFarPtr_t pge_mmsetup_welcome_panels[];

extern judePanel_t pnl_mmsetup_welc_0;
extern karlFarPtr_t pnl_mmsetup_welc_0_controls[];

extern judeControl_t ctl_mmsetup_welc_0_0;

extern char str_mmsetup_welc_0[];

extern judePanel_t pnl_mmsetup_welc_1;

extern karlFarPtr_t pnl_mmsetup_welc_1_controls[];

extern judeControl_t ctl_mmsetup_welc_1_0;
extern judeControl_t ctl_mmsetup_welc_1_1;
extern judeControl_t ctl_mmsetup_welc_1_2;
extern judeControl_t ctl_mmsetup_welc_1_3;

extern char str_mmsetup_buttons_0[];
extern char str_mmsetup_buttons_3[];

extern judeControl_t ctl_mmsetup_welc_0_1;

extern char str_mmsetup_welc_1[];

extern judeControl_t ctl_mmsetup_welc_0_2;

extern char str_mmsetup_welc_2[];

extern char str_mmsetup_welc_3[];

extern judeControl_t ctl_mmsetup_welc_0_3;

extern judeControl_t ctl_mmsetup_welc_0_4;

extern char str_mmsetup_welc_4[];

extern judeControl_t ctl_mmsetup_welc_0_5;

extern char str_mmsetup_welc_5[];

extern judePage_t pge_mmsetup_configure;

extern karlFarPtr_t pge_mmsetup_configure_panels[];

extern judePanel_t pnl_mmsetup_config_0;

extern karlFarPtr_t pnl_mmsetup_config_0_controls[];

extern judePanel_t pnl_mmsetup_config_1;

extern karlFarPtr_t pnl_mmsetup_config_1_controls[];

extern judeControl_t ctl_mmsetup_config_1_0;

extern judeControl_t ctl_mmsetup_config_0_0;

extern char str_mmsetup_config_0[];

extern judeControl_t ctl_mmsetup_config_0_1;

extern char str_mmsetup_config_1[];

extern judeControl_t ctl_mmsetup_config_0_2;

extern char str_mmsetup_config_2[];

extern judeControl_t ctl_mmsetup_config_0_3;

extern char str_mmsetup_config_3[];

extern judeControl_t ctl_mmsetup_config_0_4;

extern char str_mmsetup_config_4[];

extern judeControl_t ctl_mmsetup_config_0_5;

extern char str_mmsetup_config_5[];

extern judeControl_t ctl_mmsetup_config_0_6;

extern judeControl_t ctl_mmsetup_config_0_7;

extern char str_mmsetup_config_6[];

extern char str_mmsetup_config_7[];

extern judeControl_t ctl_mmsetup_config_0_8;

extern char str_mmsetup_config_8[];

extern judeControl_t ctl_mmsetup_config_0_9;

extern char str_mmsetup_config_9[];

extern char str_mmsetup_config_10[];

extern judeControl_t ctl_mmsetup_config_0_10;

extern char str_mmsetup_config_11[];

extern judeControl_t ctl_mmsetup_config_0_11;

extern judeControl_t ctl_mmsetup_config_0_12;

extern char str_mmsetup_config_12[];

extern judeControl_t ctl_mmsetup_config_0_13;

extern char str_mmsetup_config_13[];

extern judeControl_t ctl_mmsetup_config_0_14;

extern judeControl_t ctl_mmsetup_config_0_15;

extern judeControl_t ctl_mmsetup_config_0_16;

extern judeControl_t ctl_mmsetup_config_0_17;

extern judeControl_t ctl_mmsetup_config_0_18;

extern judeControl_t ctl_mmsetup_config_0_19;

extern judeControl_t ctl_mmsetup_config_0_20;

extern judeControl_t ctl_mmsetup_config_0_21;

extern char str_mmsetup_config_14[];

extern judeControl_t ctl_mmsetup_config_1_1;

extern char str_mmsetup_buttons_1[];

extern judeControl_t ctl_mmsetup_welc_0_6;

extern char str_mmsetup_welc_6[];
extern char str_mmsetup_welc_7[];


extern judePage_t pge_mmsetup_select;
extern karlFarPtr_t pge_mmsetup_select_panels[];

extern judePanel_t pnl_mmsetup_select_0;
extern karlFarPtr_t pnl_mmsetup_select_0_controls[];

extern judeControl_t ctl_mmsetup_select_0_0;
extern judeControl_t ctl_mmsetup_select_0_1;
extern judeControl_t ctl_mmsetup_select_0_2;
extern judeControl_t ctl_mmsetup_select_0_3;
extern judeControl_t ctl_mmsetup_select_0_4;
extern judeListBox_t lbx_mmsetup_select_0_5;

extern judePanel_t pnl_mmsetup_select_1;
extern karlFarPtr_t pnl_mmsetup_select_1_controls[];
extern judeControl_t ctl_mmsetup_select_1_0;
extern judeControl_t ctl_mmsetup_select_1_1;

extern char str_mmsetup_select_0[];
extern char str_mmsetup_select_1[];
extern char str_mmsetup_select_2[];
extern char str_mmsetup_select_3[];
extern char str_mmsetup_select_4[];

extern char str_mmsetup_select_1[];
extern char str_mmsetup_buttons_2[];

extern char str_mmsetup_config_15[];
extern char str_mmsetup_config_16[];
extern char str_mmsetup_config_17[];
extern char str_mmsetup_config_18[];

extern karlFarPtr_t config_desc_controls[];


extern judePage_t pge_mmsetup_process;
extern judePanel_t pnl_mmsetup_proc_0;
extern judePanel_t pnl_mmsetup_proc_1;
extern judeControl_t ctl_mmsetup_proc_0_0;
extern judeControl_t ctl_mmsetup_proc_0_1;    //Process desc
extern judeControl_t ctl_mmsetup_proc_0_2;    //Prompt
extern judeControl_t ctl_mmsetup_proc_0_3;    
extern judeProgressBar_t pgb_mmsetup_proc_0_4;    //proc progress
extern judeControl_t ctl_mmsetup_proc_0_5;    
extern judeControl_t ctl_mmsetup_proc_0_6;    //read data
extern judeControl_t ctl_mmsetup_proc_0_7;    
extern judeProgressBar_t pgb_mmsetup_proc_0_8;    //write progress
extern judeControl_t ctl_mmsetup_proc_0_9;    
extern judeControl_t ctl_mmsetup_proc_0_10;   //error count
extern judeListBox_t lbx_mmsetup_proc_0_11;   //output info

extern judeControl_t ctl_mmsetup_proc_1_0;
extern judeControl_t ctl_mmsetup_proc_1_1;
extern judeControl_t ctl_mmsetup_proc_1_2;

extern karlFarPtr_t pge_mmsetup_proc_panels[];
extern karlFarPtr_t pnl_mmsetup_proc_0_controls[];
extern karlFarPtr_t pnl_mmsetup_proc_1_controls[];

extern char str_mmsetup_buttons_4[];
extern char str_mmsetup_buttons_5[];
extern char str_mmsetup_buttons_6[];
extern char str_mmsetup_buttons_7[];

extern char str_mmsetup_proc_0[];
extern char str_mmsetup_proc_1[];
extern char str_mmsetup_proc_2[];
extern char str_mmsetup_proc_3[];
extern char str_mmsetup_proc_4[];
extern char str_mmsetup_proc_5[];
extern char str_mmsetup_proc_6[];
extern char str_mmsetup_proc_7[];
extern char str_mmsetup_proc_8[];
extern char str_mmsetup_proc_9[];
extern char str_mmsetup_proc_10[];

extern char str_mmsetup_start_0[];
extern char str_mmsetup_start_1[];

extern char str_mmsetup_start_2[];
extern char str_mmsetup_start_3[];
extern char str_mmsetup_start_4[];
extern char str_mmsetup_start_5[];

extern char str_mmsetup_start_6[];
extern char str_mmsetup_start_7[];

extern char str_mmsetup_start_8[];
