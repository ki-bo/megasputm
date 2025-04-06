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

#include "karljr.h"
#include "jude.h"
#include "jude_widgets.h"

void mmsetupVewPrepare(void);

void mmsetupStartExitChg(void);
void mmsetupStartStrtChg(void);

void mmsetupWelcConfigChg(void);

void mmsetupWelcNextChg(void);
void mmsetupConfigCancelChg(void);

void mmsetupConfigItemChg(void);

void mmsetupSelectCancelChg(void);

void mmsetupRealDiskChg(void);

void mmsetupSelNoneChg(void);

void mmsetupListSelect(void);

void mmsetupSelAcceptChg(void);

void mmsetupWelcThemeChg(void);

void mmsetupConfigNextChg(void);

void mmsetupProcCancelChg(void);

void mmsetupProcDoneChg(void);

void mmsetupProcContChg(void);

void mmsetupWelcThemeLblPrep(void);

void mmsetupWelcExitChg(void);

void mmsetupWelcPgeKeypress(void);
