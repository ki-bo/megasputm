/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

enum {
  UI_STR_PREP_IN,
  UI_STR_PREP_WITH,
  UI_STR_PREP_ON,
  UI_STR_PREP_TO,
  UI_STR_PAUSED,
  UI_STR_SWITCH_DISK,
  UI_STR_RESTART,
  UI_STR_COUNT
};

extern char *ui_strings[];

// code_init functions
void init_strings_en(void);
void init_strings_de(void);
