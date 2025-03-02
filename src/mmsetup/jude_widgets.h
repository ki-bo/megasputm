//=============================================================================
//Jude Widgets
//=============================================================================
//
// Simple text-based GUI controls and widget library.
// 
// Copyright (c) 2022, 2025 Daniel England.
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

#include	"jude.h"

typedef	struct	TABBAR {
		judeBar_t		_bar;
	} judeTabBar_t;

typedef	struct	BUTTONBAR {
		judeBar_t		_bar;
	} judeButtonBar_t;

typedef	struct	BROWSEBAR {
		judeBar_t		_bar;
	} judeBrowseBar_t;

typedef	struct	BUTTONCTRL {
		judeControl_t	_control;
	} judeButtonCtrl_t;

typedef	struct	LABELCTRL {
		judeControl_t	_control;
		karlFarPtr_t	actvctrl_p;
	} judeLabelCtrl_t;

typedef	struct	PAGEBTNCTRL {
		judeControl_t	_control;
		karlFarPtr_t	actvpage_p;
	} judePageBtnCtrl_t;

typedef	struct	EDITCTRL {
		judeControl_t	_control;
		uint8_t			textsz;
		uint8_t			textmaxsz;
	} judeEditCtrl_t;

typedef	struct	RADIOGRPCTRL {
		judeControl_t	_control;

		karlFarPtr_t	controls_p;
		uint8_t			controlscnt;

		karlFarPtr_t	labelctrl;
	} judeRadioGrpCtrl_t;

typedef	struct	RADIOBTNCTRL {
		judeControl_t	_control;

		karlFarPtr_t	groupctrl_p;
	} judeRadioBtnCtrl_t;

void	judeDefCtlPrepare(void);
void	judeDefCtlInit(void);
void	judeDefCtlChange(void);
void	judeDefCtlRelease(void);
void	judeDefCtlPresent(void);
void	judeDefCtlKeypress(void);
void	judeDefEdtPresent(void);
void	judeDefEdtKeypress(void);
void	judeDefLblChange(void);
void	judeDefPBtChange(void);
void	judeDefPBtPresent(void);
void	judeDefRGpChange(void);
void	judeDefRBtChange(void);
void	judeRGroupReset(uint8_t index);


//PROGRESSBAR

typedef struct PROGRESSBAR {
  judeControl_t	_control;

  uint8_t  alloc;
  uint8_t  last;
  uint32_t max;
  uint32_t value;
  uint32_t next;
  uint32_t step;
} judeProgressBar_t;

void progressResetMax(uint32_t max);
void progressIncValue(uint32_t delta);

void judePrgBarPresent(void);


//LISTBOX

typedef	struct	LISTBOX {
  judeControl_t	_control;

  EVENTPTR		select;

  karlFarPtr_t	lines_p;
  uint8_t			linescnt;
  uint8_t			linewidth;
  uint8_t			currline;
  uint8_t			linesoff;
  uint8_t			hotline;
  uint8_t			selline;
} judeListBox_t;

void judeLBxPresent(void);
void judeLBxPressed(void);
void judeLBxChange(void);
