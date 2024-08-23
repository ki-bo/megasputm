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

#include "ui_strings.h"
#include "vm.h"
#include <stdlib.h>
#include <string.h>

#pragma clang section bss="zdata"
char *ui_strings[UI_STR_COUNT];

static char *add_string(const char *str);

#pragma clang section text="code_init" data="data_init" rodata="cdata_init" bss="bss_init"
void init_strings_en(void)
{
  ui_strings[UI_STR_PREP_IN]     = add_string("in");
  ui_strings[UI_STR_PREP_WITH]   = add_string("with");
  ui_strings[UI_STR_PREP_ON]     = add_string("on");
  ui_strings[UI_STR_PREP_TO]     = add_string("to");
  ui_strings[UI_STR_PAUSED]      = add_string("Game paused, press SPACE to continue.");
  ui_strings[UI_STR_SWITCH_DISK] = add_string("Please Insert Disk %d.  Press RETURN");
  ui_strings[UI_STR_RESTART]     = add_string("Are you sure you want to restart? (y/n)");

  restart_key_yes = 'y';
}

void init_strings_de(void)
{
  ui_strings[UI_STR_PREP_IN]     = add_string("mit");
  ui_strings[UI_STR_PREP_WITH]   = add_string("mit");
  ui_strings[UI_STR_PREP_ON]     = add_string("mit");
  ui_strings[UI_STR_PREP_TO]     = add_string("zu");
  ui_strings[UI_STR_PAUSED]      = add_string("PAUSE - Zum Spielen Leertaste dr\x5b""cken.");
  ui_strings[UI_STR_SWITCH_DISK] = add_string("Diskette %d einlegen.  Best\x5ctige EINGABE.");
  ui_strings[UI_STR_RESTART]     = add_string("Wollen Sie neu starten? (j/n)");

  restart_key_yes = 'j';
}

static char *add_string(const char *str)
{
  char *ptr = malloc(strlen(str) + 1);
  strcpy(ptr, str);
  return ptr;
}
