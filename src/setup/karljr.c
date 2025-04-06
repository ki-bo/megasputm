//=============================================================================
//Karl Jr
//=============================================================================
//
// Simple object life-time management.
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


#include	"karljr.h"


void	karlModAttach(karlFarPtr_t module) {
  zreg0 = (uint32_t)(module);

	_karlModAttach();
}


void	karlObjExcStateEx(uint8_t changed, uint16_t state) {
	zreg0wl = state;
	zreg0b2 = changed;

	_karlObjExcStateEx();
}

void	karlObjIncStateEx(uint8_t changed, uint16_t state) {
	zreg0wl = state;
	zreg0b2 = changed;

	_karlObjIncStateEx();
}
