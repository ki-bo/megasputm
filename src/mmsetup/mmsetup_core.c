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


#include "system.h"

#include <stdint.h>

#include "jude_widgets.h"
#include "mega65.h"
#include "hdos.h"

#include "adf.h"
#include "room54.h"

#include "jude.h"
#include "karljr.h"
#include "mmsetup.h"
#include "mmsetup_core.h"


uint8_t config_select = 0;

process_t process = PROC_NONE;
procstate_t procstate = PROCST_IDLE;

uint8_t procAbort = 0;
uint8_t procContinue = 0;
uint8_t procError = 0;

uint8_t outputDisk;
uint8_t newDisk = 0;
uint8_t roomidx = 0xff;
uint8_t roompreprep = 0xff;
uint8_t haveRoom54 = 0;
uint16_t room54size = 0;
uint8_t langidx = 0xff;
uint8_t configProcFlags = 0;
uint8_t doneProcFlags = 0;

uint32_t bootflags;

char file_extensions[3][4] = {"D81", "ADF", "D64"};

settingDetail_t dest_details[8] = {
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0, ""}, 
  {SETTINGT_NONE, 0, ""}
};


typedef struct ROOMDATA {
  uint8_t room;
  uint32_t size;
  uint16_t check;
} roomdata_t;


roomdata_t disk1_en[] = {
  {0,  1988, 0xdd69},
  {30, 24926, 0x5eab},
  {33, 21257, 0x1ff8},
  {40,  5177, 0xe4b1},
  {44, 33250, 0x53c8},
  {45, 16284, 0x7da1},
  {49,  4710, 0xf024},
  {50,  6248, 0x978a},
  {51, 23349, 0x79f1},
  {53, 71634, 0x3dec},
  {0xFF, 0, 0}
};

roomdata_t disk2_en[] = {
  { 1, 30581, 0x4b34},
  { 2,  5103, 0x973a},
  { 3, 11334, 0x3d4d},
  { 4, 40861, 0x2b19},
  { 5, 18561, 0xd0e0},
  { 6, 13210, 0x253e},
  { 7, 23597, 0x1bd9},
  { 8, 12815, 0x1b1f},
  { 9,  6871, 0xda29},
  {10, 10289, 0xe716},
  {11, 12199, 0xc3c3},
  {12, 23914, 0x06d1},
  {13, 16369, 0x330f},
  {14,  4966, 0xfe87},
  {15, 11551, 0x9ca0},
  {16, 21756, 0x505c},
  {17, 31241, 0x82c9},
  {18, 11756, 0xca59},
  {19, 11990, 0xdf3c},
  {20,  9461, 0xde87},
  {21, 20473, 0x9234},
  {22,  7355, 0xa9fe},
  {23, 16858, 0x35cb},
  {24, 36731, 0x88e6},
  {25, 14062, 0x07b7},
  {26, 17456, 0x0302},
  {27, 14657, 0x9cbb},
  {28,  4672, 0xf5ad},
  {29, 22821, 0x4bd0},
  {31, 15006, 0xa243},
  {32,  7656, 0x1b6a},
  {34,  4994, 0xcf9f},
  {35, 10849, 0xdb19},
  {36,  9360, 0xf492},
  {37, 17128, 0xab2b},
  {38, 19278, 0xe06f},
  {39,  2518, 0x3947},
  {41,  5564, 0x5858},
  {42,  6625, 0xd889},
  {43,  3138, 0xc19b},
  {44, 33250, 0x53c8},
  {46,  6651, 0x6954},
  {47, 20175, 0x8f94},
  {48,  4050, 0x9067},
  {52,  4014, 0x503e},
  {53, 71634, 0x3dec},
  {0xff, 0, 0}
};


roomdata_t disk1_de[] = {
  { 0,  1988, 0xcfe5},
  {30, 25087, 0x5d0b},
  {33, 21315, 0xfa4d},
  {40,  5255, 0xdd8b},
  {44, 33261, 0x678c},
  {45, 16376, 0x987a},
  {49,  4710, 0xf024},
  {50,  6703, 0x61d4},
  {51, 23342, 0x7a93},
  {53, 71641, 0x222f},
  {0xff, 0, 0}
};

roomdata_t disk2_de[] = {
  { 1, 30685, 0x7b2a},
  { 2,  5107, 0x4df9},
  { 3, 11366, 0x0bb4},
  { 4, 41223, 0x0329},
  { 5, 18602, 0xe067},
  { 6, 13235, 0xed04},
  { 7, 23680, 0x9288},
  { 8, 12910, 0x36a1},
  { 9,  6885, 0xecd0},
  {10, 10270, 0x8f17},
  {11, 12216, 0x9d90},
  {12, 23956, 0x2782},
  {13, 16374, 0x3edf},
  {14,  4989, 0xb382},
  {15, 11572, 0x2ab7},
  {16, 21829, 0x7a65},
  {17, 31300, 0x0991},
  {18, 11899, 0xed3a},
  {19, 12042, 0x6308},
  {20,  9548, 0x476f},
  {21, 20565, 0x8152},
  {22,  7443, 0x84a8},
  {23, 16939, 0xa7d3},
  {24, 36748, 0xb6cb},
  {25, 14082, 0x7790},
  {26, 17687, 0x26b8},
  {27, 14709, 0xf7bb},
  {28,  4709, 0x6a08},
  {29, 22830, 0xbada},
  {31, 15054, 0x5529},
  {32,  7656, 0x1b6a},
  {34,  4994, 0xcf9f},
  {35, 10849, 0xdb19},
  {36,  9383, 0x7a22},
  {37, 17132, 0x8102},
  {38, 19297, 0x6873},
  {39,  2518, 0x3947},
  {41,  5632, 0xd734},
  {42,  6670, 0x415d},
  {43,  3151, 0xd240},
  {44, 33261, 0x678c},
  {46,  6653, 0xfdc8},
  {47, 20186, 0xf8c1},
  {48,  4050, 0x9067},
  {52,  4022, 0x4bd2},
  {53, 71641, 0x222f},
  {0xff, 0, 0}
};


roomdata_t *langs_disk1[] = {
  disk1_en,
  disk1_de,
  0
};

roomdata_t *langs_disk2[] = {
  disk2_en,
  disk2_de,
  0
};


procbehaviour_t proc_behaviours[] = {
  {0, 0, 0, 0, 0, 0},
  {&behaviourConfigIdle, &behaviourConfigInit, &behaviourConfigWait, 0, 0, 0},
  {&behaviourExtractIdle, &behaviourExtractInit, 0, &behaviourExtractRead, &behaviourExtractWrite, &behaviourExtractFinish},
  {&behaviourBuildIdle, &behaviourBuildInit, 0, &behaviourBuildRead, &behaviourBuildWrite, &behaviourBuildFinish},
  {&behaviourValidIdle, &behaviourValidInit, 0, &behaviourValidRead, 0, &behaviourValidFinish},
  {0, 0, 0, 0, 0, 0}
}; 

char adfFileName[65];
char lflfilename[] = "00.LFL,S,W";
char lflfileread[] = "00.LFL,S,R";
char lflfileadf[] = "00.LFL";
char str_filesize[] = "                    ";

struct Device *dev;
struct Volume *vol;
struct List *list, *cell;

uint8_t __huge *file_pos;
uint32_t file_size;

uint8_t __huge *procOutput;


#define ADF_MEMORY ((uint8_t __huge *)0x08000000)
#define FILE_MEMORY ((uint8_t __huge *)0x40000)


extern uint16_t mouseXPos;
extern uint16_t mouseYPos;
extern uint8_t kernal_get_last_error(void);
extern void _prepareKernalWrite(void);
extern void _performKernalWrite(void);
extern void _prepareKernalRead(void);
extern void _performKernalRead(void);
extern void _finishKernalReadWrite(void);


int16_t new_x;
int16_t new_y;


static int8_t check_mouse_movement(uint8_t pot, uint8_t old_pot) 
{
  uint8_t diff = (pot - old_pot) & 0x7f;
  if (diff < 64) {
    return (int8_t)diff >> 1; // divide by 2 but keep sign/msb
  }
  // handle negative value, add two msb sign bits and mask out noise bit
  diff |= 0xc1;
  if (diff != 0xff) {
    ++diff;
    return (int8_t)diff >> 1;
  }
  return 0;
}


static int8_t apply_acceleration(int8_t value)
{
  //uint8_t abs_diff = abs8(value);
  uint8_t abs_diff;
  __asm(" tax\n"
        " bpl done\n"
        " neg a\n"
        "done:"
        : "=Ka"(abs_diff)
        : "Ka"(value)
        : __REGA, __REGX);

  if (abs_diff > 10) {
    return (value << 2) + value;
  }
  if (abs_diff > 5) {
    return (value << 1) + value;
  }
  return value;
}

// move this to common header to be used by the engine and mmsetup
struct __pot {
  uint8_t x;
  uint8_t y;
};
#define POT         (*(volatile struct __pot *)         0xd419)


static void handle_mouse(void)
{
  static uint8_t old_potx = 0;
  static uint8_t old_poty = 0;
  uint8_t potx = POT.x;
  uint8_t poty = POT.y;
  // prepare CIA1 already now for joystick handling, as this takes some time
  CIA1.pra  = 0xff;

  int8_t diff = check_mouse_movement(potx, old_potx);
  if (diff) {
    new_x += apply_acceleration(diff);
    old_potx = potx;
  }
  diff = check_mouse_movement(poty, old_poty);
  if (diff) {
    new_y -= apply_acceleration(diff);
    old_poty = poty;
  }
}

/*static void handle_joystick(void)
{
  static uint8_t old_joy1;
  uint8_t joy2 = CIA1.pra;
  uint8_t joy1 = CIA1.prb;
  if (!(joy2 & 0x01)) {
    new_y -= 2;
  } 
  else if (!(joy2 & 0x02)) {
    new_y += 2;
  }
  if (!(joy2 & 0x04)) {
    new_x -= 2;
  }
  else if (!(joy2 & 0x08)) {
    new_x += 2;
  }
  if (ui_state & UI_FLAGS_ENABLE_CURSOR) {
    input_button_pressed = (!(joy2 & 0x10) || !(joy1 & 0x10)) ? INPUT_BUTTON_LEFT : 0;
  }
  
  if ((old_joy1 & 1) && !(joy1 & 1)) {
    // edge triggered right mouse button is handled as override key
    input_key_pressed = vm_read_var8(VAR_OVERRIDE_KEY);
  }
  old_joy1 = joy1;
}*/


void input_update(void) {
  new_x = mouseXPos;
  new_y = mouseYPos;

  handle_mouse();
  //handle_joystick();
  CIA1.pra = 0x40; // prepare CIA1 alredy for sampling mouse, as this takes some time

  if (new_x < 0) {
    new_x = 0;
  }
  else if (new_x > 319) {
    new_x = 319;
  }
  if (new_y < 0) {
    new_y = 0;
  }
  else if (new_y > 199) {
    new_y = 199;
  }

  mouseXPos = new_x;
  mouseYPos = new_y;
}





char toUpperCase(char data) {
  if ((data >= 97) && (data <= 122)) {
    return data - 32;
  } else {
    return data;
  }
}

void writetwodecimalstr(char *buf, uint8_t pos, uint8_t value) {
  uint8_t tens = (value / 10);
  uint8_t ones = (value - (tens * 10)) + 0x30;
  tens +=  0x30;

  buf[pos] = tens;
  buf[pos+1] = ones;
}

uint32_t places[] = {
  100000,
   10000,
    1000,
     100,
      10,
       1
};

void writesixdecimalstr(char *buf, uint8_t pos, uint32_t value) {
  uint32_t residual = (value > 999999L) ? 0 : value;
  uint8_t  values[6];

  for (uint8_t i = 0; i < 6; i++) {
    values[i] = (uint8_t)(residual / places[i]);
    residual -= (uint32_t)values[i] * places[i];
  }

  uint8_t doout = 0;
  uint8_t idx = pos;
  for (uint8_t i = 0; i < 6; i++) {
    doout = doout || (values[i] > 0) || (i == 5);
    if (doout) {
      buf[idx] = values[i] + 0x30;
    } else {
      buf[idx] = 0x20;
    }
    idx++;
  }
}

void prepareLFLFileName(uint8_t roomno) {
  writetwodecimalstr(lflfilename, 0, roomno);
  writetwodecimalstr(lflfileread, 0, roomno);
  writetwodecimalstr(lflfileadf, 0, roomno);
}


void performKernalHeaderChange(uint8_t diskNo) {
  __asm(
    " .extern _performKernalHeaderChange \n"
    "   jsr _performKernalHeaderChange \n"
    :
    : "Ka" (diskNo)
    : __REGA, __REGX, __REGY, __REGZ
  );
};

void performKernalScatchAllRooms(void) {
  __asm(
    " .extern _performKernalScatchAllRooms \n"
    "   jsr _performKernalScatchAllRooms \n"
    :
    : 
    : __REGA, __REGX, __REGY, __REGZ
  );
};

void performKernalValidate(void) {
  __asm(
    " .extern _performKernalValidate \n"
    "   jsr _performKernalValidate \n"
    :
    : 
    : __REGA, __REGX, __REGY, __REGZ
  );
};



void prepareKernalWrite(char *filename, uint8_t len) {
  __asm(
    " .extern _prepareKernalWrite \n"
    "   lda %[ln] \n"
    "   ldx %[fn] \n"
    "   ldy %[fn] + 1 \n"
    "   jsr _prepareKernalWrite \n"
    :
    : [fn] __KZ16 (filename), [ln] __KZ08 (len)
    : __REGA, __REGX, __REGY, __REGZ
  );
}

void performKernalWrite(uint32_t source, uint32_t size) {
  kernalWriteSrc = source;
  kernalWriteSiz = size;

  _performKernalWrite();
}

void finishKernalReadWrite(void) {
  _finishKernalReadWrite();
}

void prepareKernalRead(char *filename, uint8_t len) {
  __asm(
    " .extern _prepareKernalRead \n"
    "   lda %[ln] \n"
    "   ldx %[fn] \n"
    "   ldy %[fn] + 1 \n"
    "   jsr _prepareKernalRead \n"
    :
    : [fn] __KZ16 (filename), [ln] __KZ08 (len)
    : __REGA, __REGX, __REGY, __REGZ
  );
}

void performKernalRead(uint32_t *source, uint32_t *size) {
  kernalWriteSrc = *source;
  kernalWriteSiz = *size;

  _performKernalRead();

  *source = kernalWriteSrc;
  *size = kernalWriteSiz;
}



uint8_t readDirectoryFiles(const char *ext) {
  uint8_t result = 0;
  uint8_t drive, desc;
  dirent_t *entry = (dirent_t *)(0x0800);

  hdos_closeall();

  hdos_close_file();

  hdos_getdefdrive(&drive);

  if (hdos_selectdrive(drive)) {
    *(uint8_t *)(0xd020) = 3;
    return result;
  };

  if (hdos_cdrootdir(drive)) {
    *(uint8_t *)(0xd020) = 4;
    return result;
  }

  if (hdos_open_dir(&desc)) {
    *(uint8_t *)(0xd020) = 5;
    return result;
  }

  uint8_t  __huge *out = (uint8_t __huge *)(0x016000);

  while (!hdos_read_dir(desc)) {
    uint8_t found = 0;

    if (!(entry->filetype & (FT_SUBDIR | FT_HIDDEN | FT_SYSTEM  | FT_VOLLABEL))) {
      found = 1;

      for (uint8_t i = 0; i < 3; ++i) {
        if (toUpperCase(entry->fileext[i]) != ext[i]) {
          found = 0;
          break;
        }
      }

      if (found && (result < 254)) {
        result++;

        for (uint8_t i = 0; i < 64; ++i) {
          *out = entry->filename[i];
          ++out;
        }
        *out = 0;
        ++out;
        *out = entry->namelen;
        ++out;
      }
    }
  }
  hdos_close_dir(desc);
  
  return result;
}


uint8_t configurationInvalid(void) {
  uint8_t result = 0;

  if (ctl_mmsetup_welc_0_3._element._object.tag) {
    if (dest_details[0].type == SETTINGT_NONE && dest_details[1].type == SETTINGT_NONE) {
      result = 1;
    } else {
      if (dest_details[0].type != SETTINGT_NONE && dest_details[2].type == SETTINGT_NONE) {
        result = 1;
      }
      if (dest_details[1].type != SETTINGT_NONE && dest_details[3].type == SETTINGT_NONE) {
        result = 1;
      }
    }
  }

  if (result == 0 && ctl_mmsetup_welc_0_4._element._object.tag) {
    if (dest_details[4].type == SETTINGT_NONE || dest_details[5].type == SETTINGT_NONE) {
      result = 2;
    }
  }

  if (result == 0 && ctl_mmsetup_welc_0_5._element._object.tag) {
    if (dest_details[0].type == SETTINGT_NONE && dest_details[1].type == SETTINGT_NONE) {
      result = 3;
    }
  }


  return result;
}

void writeToProcOutput(char *text) {
  uint8_t i = 0;
  while (i < 40) {
    uint8_t data = text[i];
    *procOutput = data;
    ++procOutput;
    ++i;
    if (!data) {
      break;
    }
  }

  while (i < 40) {
    ++procOutput;
    ++i;
  }

  lbx_mmsetup_proc_0_11.linescnt = lbx_mmsetup_proc_0_11.linescnt + 1;

  if (lbx_mmsetup_proc_0_11.linescnt > lbx_mmsetup_proc_0_11._control._element.height) {
    lbx_mmsetup_proc_0_11.linesoff = lbx_mmsetup_proc_0_11.linescnt - lbx_mmsetup_proc_0_11._control._element.height + 1;
  }

  zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_proc_0_11);
  karlObjIncludeState(STATE_CHANGED);
}

void initiateProcess(void) {
  outputDisk = 0;
  newDisk = 1;
  roomidx = 0;
  roompreprep = 0;
  haveRoom54 = 0;
  langidx = 0xff;

  procOutput = (uint8_t __huge *)LISTBOXLINESMEM;

  lbx_mmsetup_proc_0_11.linescnt = 0;
  lbx_mmsetup_proc_0_11.currline = 0;
  lbx_mmsetup_proc_0_11.linesoff = 0;
  lbx_mmsetup_proc_0_11.hotline = 0;
  lbx_mmsetup_proc_0_11.selline = 0xff;

  uint16_t max = 0;

  if (configProcFlags & PROCFL_EXTRACT) {
    if (dest_details[0].type != SETTINGT_NONE) {
      max += (sizeof(disk1_en) / sizeof(roomdata_t)) - 1;
    }
    if (dest_details[1].type != SETTINGT_NONE) {
      max += (sizeof(disk2_en) / sizeof(roomdata_t)) - 1;
    }
  }

  if (configProcFlags & PROCFL_BUILD) {
    if (dest_details[0].type != SETTINGT_NONE) {
      max++;
    }
    if (dest_details[1].type != SETTINGT_NONE) {
      max++;
    }
  }

  if (configProcFlags & PROCFL_VALIDATE) {
    if (dest_details[0].type != SETTINGT_NONE) {
      max += (sizeof(disk1_en) / sizeof(roomdata_t)) - 1;
    }
    if (dest_details[1].type != SETTINGT_NONE) {
      max += (sizeof(disk2_en) / sizeof(roomdata_t)) - 1;
    }
  }

  zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_4);
  progressResetMax(max);

  writeToProcOutput("PROCESSING...");

  ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)0;
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_6);
  karlObjIncludeState(STATE_DIRTY);

  ctl_mmsetup_proc_0_10.text_p = (karlFarPtr_t)0;
  ctl_mmsetup_proc_0_10._element.colour = CLR_TEXT;
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_10);
  karlObjIncludeState(STATE_DIRTY);

  zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_proc_0_11);
  karlObjExcludeState(STATE_ENABLED);
  karlObjIncludeState(STATE_CHANGED);

  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
  karlObjExcludeState(STATE_ENABLED);

  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_0);
  karlObjExcludeState(STATE_ENABLED);

  ctl_mmsetup_proc_1_2._element._object.tag = 1;
  ctl_mmsetup_proc_1_2.text_p = (karlFarPtr_t)str_mmsetup_buttons_1;
  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_2);
  karlObjIncludeState(STATE_CHANGED);

  judeActivateCtrl();

  procAbort = 0;
  procContinue = 0;
  procError = 0;
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;

  //configProcFlags = PROCFL_CONFIGURE | PROCFL_BUILD | PROCFL_EXTRACT;
  doneProcFlags = 0;
}


void cancelProcess(void) {
  procAbort = 1;
}

void continueProcess(void) {
  procContinue = 1;
}


void copyFileName(uint8_t detail, char *fileName) {
  for (uint8_t i = 0; i < dest_details[detail].namelen; ++i) {
    fileName[i] = dest_details[detail].fileName[i];

    //dengland Is this debugging and can go?
    *(uint8_t *)(0x0800 + i) = dest_details[detail].fileName[i];
  }
  fileName[dest_details[detail].namelen] = 0;
}


void updateProcess(void) {
  if (process ==  PROC_COMPLETE) {
    writeToProcOutput("COMPLETE");

    if (procError) {
      ctl_mmsetup_proc_0_10.text_p = (karlFarPtr_t)str_mmsetup_proc_9;
      ctl_mmsetup_proc_0_10._element.colour = CLR_SYSS_TEXT | 0x0A;
    } else {
      ctl_mmsetup_proc_0_10.text_p = (karlFarPtr_t)str_mmsetup_proc_10;
      ctl_mmsetup_proc_0_10._element.colour = CLR_TEXT;
    }

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_10);
    karlObjIncludeState(STATE_DIRTY);

    zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_proc_0_11);
    karlObjIncludeState(STATE_ENABLED);

    zptrself = (uint32_t)((karlObject_t __huge *)&pnl_mmsetup_proc_0);
    karlObjIncludeState(STATE_DIRTY);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
    karlObjExcludeState(STATE_VISIBLE);
    
    ctl_mmsetup_proc_1_2._element._object.tag = 0;
    ctl_mmsetup_proc_1_2.text_p = (karlFarPtr_t)str_mmsetup_buttons_6;
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_2);
    karlObjIncludeState(STATE_CHANGED);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
    karlObjExcludeState(STATE_ENABLED);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_0);
    karlObjIncludeState(STATE_ENABLED);

    judeActivateCtrl();

    judeSetPointer(MPTR_NORMAL);

    process = PROC_NONE;
  } else if (process != PROC_NONE) {
    void (* behaviour)(void) = proc_behaviours[process][procstate];

    if (behaviour) {
      behaviour();
    } else {
      writeToProcOutput("INTERNAL ERROR");
      process = PROC_NONE;
    }
  }
}

void processError(char *reason) {
  process = PROC_COMPLETE;
  procstate = PROCST_IDLE;

  procError = 1;

  writeToProcOutput(reason);
}


uint16_t chks = 0;

uint8_t verifyMemory(uint8_t __huge *mem, uint32_t size, uint16_t check) {
  uint32_t nw = size >> 1;
  uint16_t num_words = (uint16_t)nw;
  
  if (size & 0x00000001) {
    mem[size] = 0;
    num_words++;
  }
  
  uint8_t __huge *ptr = mem;
  chks = 0;

  
  uint8_t b;
  uint16_t word;

  for (uint16_t i = 0; i < num_words; i++) {
    b = *ptr++;
    word = b ^ 0xff;
  
    b = *ptr++;
    word |= (uint16_t)(b ^ 0xff) << 8;

    chks += word;
  } 
  
  return chks == check;
}


void behaviourConfigIdle(void) {
  procstate = PROCST_INIT;
}

void behaviourConfigInit(void) {
  //find next job or disk
  if (procAbort || (outputDisk > 1)) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
  } else {
    if (dest_details[outputDisk].type != SETTINGT_NONE) {
      if (newDisk) {
        if (dest_details[outputDisk].type == SETTINGT_DISK) {

          char text[20] = "REQUIRE DISK  #00";
      
          writetwodecimalstr(text, 15, outputDisk + 1);
          writeToProcOutput(text);
      
          procstate = PROCST_WAIT;
          procContinue = 0;
      
          zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
          karlObjIncludeState(STATE_VISIBLE);
      
          zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
          karlObjIncludeState(STATE_ENABLED);

          return;
        } else {
          writeToProcOutput("MOUNT   IMAGE...");
          
          procstate = PROCST_WAIT;
          return;
        }
      } else {
        if (configProcFlags & PROCFL_EXTRACT && !(doneProcFlags & PROCFL_EXTRACT)) {
          process = PROC_EXTRACT;
          procstate = PROCST_IDLE;
          return;
        } else if (configProcFlags & PROCFL_BUILD && !(doneProcFlags & PROCFL_BUILD)) {
          process = PROC_BUILD;
          procstate = PROCST_IDLE;
          return;
        } else if (configProcFlags & PROCFL_VALIDATE && !(doneProcFlags & PROCFL_VALIDATE)) {
          process = PROC_VALIDATE;
          procstate = PROCST_IDLE;
          return;
        }
      }
    }

    //try next disk   
    newDisk = 1;   
    outputDisk++;
    doneProcFlags = 0;
    procstate = PROCST_IDLE;
  }
}

void behaviourConfigWait(void) {
  if (!procAbort && dest_details[outputDisk].type == SETTINGT_IMGE) {
    copyFileName(outputDisk, adfFileName);

    if (hdos_set_filename(adfFileName)) {
      writeToProcOutput(adfFileName);
      
      processError("SETNAME D81 ERROR");
      return;
    } else if (hdos_attachD810()) {
      writeToProcOutput(adfFileName);
      processError("MOUNT D81 ERROR");
      return;
    } else {    
      performKernalHeaderChange(outputDisk + 1);
      
      if (configProcFlags & PROCFL_EXTRACT) {
        performKernalScatchAllRooms();
      }

      performKernalValidate();

      procstate = PROCST_INIT;
      newDisk = 0;
      return;
    }
  }
  else if (procContinue) {
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
    karlObjExcludeState(STATE_ENABLED);
    
    zptrself = (uint32_t)((karlObject_t __huge *)&pnl_mmsetup_proc_0);
    karlObjIncludeState(STATE_DIRTY);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
    karlObjExcludeState(STATE_VISIBLE);

    performKernalHeaderChange(outputDisk + 1);

    if (configProcFlags & PROCFL_EXTRACT) {
      performKernalScatchAllRooms();
    }

    performKernalValidate();

    newDisk = 0;
    procstate = PROCST_INIT;
    procContinue = 0;
  } else if (procAbort) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
  }
}

void behaviourBuildIdle(void) {
  procstate = PROCST_INIT;
}

void behaviourBuildInit(void) {
  if (!haveRoom54) {
    writeToProcOutput("BUILD   READ  D64 IMAGES");
    judeSetPointer(MPTR_WAIT);

    procstate = PROCST_READ;

  } else {
    judeSetPointer(MPTR_WAIT);

    file_size = room54size;
    file_pos = (uint8_t __huge *)(0x0001B000);

    writesixdecimalstr(str_filesize, 14, file_size);
    ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)((char __huge *)str_filesize);
    zptrself = (uint32_t)((karlFarPtr_t)&ctl_mmsetup_proc_0_6);
    karlObjIncludeState(STATE_DIRTY);
  
    writeToProcOutput("BUILD   WRITE 54.LFL");

    zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_8);
    progressResetMax(file_size);
  
    prepareLFLFileName(54);
    prepareKernalWrite(lflfilename, 10);

    procstate = PROCST_WRITE;
  }
}

void behaviourBuildRead(void) {
  copyFileName(4, adfFileName);

  if (hdos_set_filename(adfFileName)) {
    //error and finish
    processError("HDOS SETFN FAIL #1");
    return;
  };
  hdos_load_file_attic(0);

  copyFileName(5, adfFileName);
  if (hdos_set_filename(adfFileName)) {
    processError("HDOS SETFN FAIL #2");
    return;
  };
  hdos_load_file_attic(0x32000);
  
  readIndexFile((uint8_t __huge *)(0x08000000));
  
  uint8_t __huge *image1 = (uint8_t __huge *)(0x08000000);
  uint8_t __huge *image2 = (uint8_t __huge *)(0x08032000);
  uint8_t __huge *dest = (uint8_t __huge *)(0x0001B000);
  
  room54size = makeRoom54(image1,image2, dest);

  haveRoom54 = 1;
  judeSetPointer(MPTR_NORMAL);
  procstate = PROCST_INIT;
}

void behaviourBuildWrite(void) {
  uint32_t next_size = 254;

  if (next_size > file_size) {
    next_size = file_size;
  }

  judeSetPointer(MPTR_WAIT);

  if (!procAbort) {
    performKernalWrite((uint32_t)file_pos, next_size);

    uint8_t error = kernal_get_last_error();

    if (error) {
      processError("D81 WRITE ERROR");
      finishKernalReadWrite();
      
      return;
    }
  }
  
  file_size -= next_size;
  file_pos += next_size;
  zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_8);
  progressIncValue(next_size);

  if (file_size == 0) {
    finishKernalReadWrite();

    if (kernal_get_status()) {
      processError("FILE CLOSE ERROR");
      return;
    }

    zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_4);
    progressIncValue(1);
    procstate = PROCST_FINISH;
  }

  judeSetPointer(MPTR_NORMAL);
}

void behaviourBuildFinish(void) {
  
  doneProcFlags |= PROCFL_BUILD;
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;
}


void behaviourValidIdle(void) {
  roomidx = 0;
  roompreprep = 0;

  procstate = PROCST_INIT;
}

void behaviourValidInit(void) {
  if (outputDisk > 1) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
    return;
  } else if  (dest_details[outputDisk].type == SETTINGT_NONE) {
    procstate = PROCST_FINISH;
    return;
  }

  roomdata_t *rooms;
  
  if (outputDisk == 0) {
    rooms = langs_disk1[0];
  } else {
    rooms = langs_disk2[0];
  }

  if (rooms[roomidx].room == 0xff) {
    roomidx = 0;
    procstate = PROCST_FINISH;
    return;
  }

  judeSetPointer(MPTR_WAIT);

  char text[] = "VERIFY  READ  00.LFL";
  writetwodecimalstr(text, 14, rooms[roomidx].room);
  writeToProcOutput(text);

  prepareLFLFileName(rooms[roomidx].room);

  file_pos = FILE_MEMORY;
  file_size = 0;

  prepareKernalRead(lflfileread, 10);
  if (kernal_get_last_error()) {
    processError("D81 PREPARE ERROR");
    return;
  }

  judeSetPointer(MPTR_NORMAL);
  procstate = PROCST_READ;
}

void behaviourValidRead(void) {
  roomdata_t *rooms;
  
  if (outputDisk == 0) {
    rooms = langs_disk1[0];
  } else {
    rooms = langs_disk2[0];
  }
  
  uint32_t nextpos = (uint32_t)(file_pos);
  uint32_t nextsz = 253;
  performKernalRead(&nextpos, &nextsz);
  file_pos = (uint8_t __huge *)nextpos;
  file_size += 253 - nextsz;

  if (file_size > 80000) {
    processError("INTERNAL ERROR");
    return;
  }

  writesixdecimalstr(str_filesize, 14, file_size);
  ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)((char __huge *)str_filesize);
  zptrself = (uint32_t)((karlFarPtr_t)&ctl_mmsetup_proc_0_6);
  karlObjIncludeState(STATE_DIRTY);

  if (nextsz > 0) {
    finishKernalReadWrite();

    if (langidx != 0xff) {
      if (outputDisk == 0) {
        rooms = langs_disk1[langidx];
      } else {
        rooms = langs_disk2[langidx];
      }

      if (file_size != rooms[roomidx].size || !verifyMemory(FILE_MEMORY, file_size, rooms[roomidx].check)) {
        processError("VERIFY  FAIL");
        return;
      } else {
        writeToProcOutput("VERIFY  PASS");
      }
    } else {
      uint8_t nextlang = 0;

      for (nextlang = 0; nextlang < 2; nextlang++) {
        if (outputDisk == 0) {
          rooms = langs_disk1[nextlang];
        } else {
          rooms = langs_disk2[nextlang];
        }

        if (file_size == rooms[roomidx].size && verifyMemory(FILE_MEMORY, file_size, rooms[roomidx].check)) {
          break;
        }
      }
      
      if (nextlang == 2) {
        processError("VERIFY  FAIL");
        return;
      } else if (nextlang == 0) {
        writeToProcOutput("VERIFY  PASS ENGLISH");
      } else {
        writeToProcOutput("VERIFY  PASS GERMAN");
      }
    }

    roomidx++;
    procstate = PROCST_INIT;

    zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_4);
    progressIncValue(1);
  }
}

void behaviourValidFinish(void) {
  writeToProcOutput("VERIFY  FINISH");
  judeSetPointer(MPTR_NORMAL);
  
  doneProcFlags |= PROCFL_VALIDATE;
 
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;
}


void behaviourExtractIdle(void) {
    procstate = PROCST_INIT;
}

void behaviourExtractInit(void) {
  if (outputDisk > 1) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
    return;
  } else if  (dest_details[outputDisk].type == SETTINGT_NONE) {
    procstate = PROCST_FINISH;
    return;
  }

  writeToProcOutput("EXTRACT INIT");
            
  judeSetPointer(MPTR_WAIT);
  roomidx = 0;
  roompreprep = 0;
  
  copyFileName(outputDisk + 2, adfFileName);
  if (hdos_set_filename(adfFileName) ) {
    processError("HDOS SETFN FAIL");
    return;
  };
  hdos_load_file_attic(0);

  if (adf_init(ADF_MEMORY) != ADF_OK) {
    processError("ADF INIT ERROR");
    return;
  }
  
  if (adf_chdir("rooms") != ADF_OK) {
    processError("ADF CHDIR ERROR");
    return;
  }

  if (langidx == 0xff) {
    prepareLFLFileName(0);
    if (adf_read_file(lflfileadf, FILE_MEMORY, &file_size) != ADF_OK) {
      processError("ADF READ INDEX ERROR");
      return;
    }

    if (file_size == disk1_de[0].size && verifyMemory(FILE_MEMORY, file_size, disk1_de[0].check)) {
      writeToProcOutput("GERMAN  LANGUAGE DETECTED");
      langidx = 1;
    } else if (file_size == disk1_en[0].size && verifyMemory(FILE_MEMORY, file_size, disk1_en[0].check)) {
      writeToProcOutput("ENGLISH LANGUAGE DETECTED");
      langidx = 0;
    } else {
      writesixdecimalstr(str_filesize, 14, chks);
      ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)((char __huge *)str_filesize);
      zptrself = (uint32_t)((karlFarPtr_t)&ctl_mmsetup_proc_0_6);
      karlObjIncludeState(STATE_DIRTY);
    
      processError("ADF INDEX UNKNOWN LANG");
      return;
    }
  }

  roomdata_t *rooms;
  if (outputDisk == 0) {
    rooms = langs_disk1[langidx];
  } else {
    rooms = langs_disk2[langidx];
  }
  
  while (rooms[roompreprep].room != 0xff) {
    prepareLFLFileName(rooms[roompreprep].room);

    if (adf_read_file(lflfileadf, FILE_MEMORY, &file_size) != ADF_OK) {
      processError("ADF PREFETCH ERROR");
      return;
    }

    if (file_size != rooms[roompreprep].size) {
      processError("ADF SIZE CHECK INVALID");
      return;
    }

    if (!verifyMemory(FILE_MEMORY, file_size, rooms[roompreprep].check)) {
      writesixdecimalstr(str_filesize, 14, roompreprep);
      ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)((char __huge *)str_filesize);
      zptrself = (uint32_t)((karlFarPtr_t)&ctl_mmsetup_proc_0_6);
      karlObjIncludeState(STATE_DIRTY);
      
      processError("ADF CHECKSUM INVALID");
      return;
    }

    roompreprep++;
  }

  procstate = PROCST_READ;
  judeSetPointer(MPTR_NORMAL);
}

void behaviourExtractRead(void) {
  roomdata_t *rooms;
  
  if (outputDisk == 0) {
    rooms = langs_disk1[langidx];
  } else {
    rooms = langs_disk2[langidx];
  }

  if (rooms[roomidx].room == 0xff) {
    roomidx = 0;
    procstate = PROCST_FINISH;
    return;
  }

  char text[] = "EXTRACT READ  00.LFL";
  writetwodecimalstr(text, 14, rooms[roomidx].room);
  writeToProcOutput(text);

  prepareLFLFileName(rooms[roomidx].room);

  if (adf_read_file(lflfileadf, FILE_MEMORY, &file_size) != ADF_OK) {
    processError("ADF READ ERROR");
    return;
  }

  writesixdecimalstr(str_filesize, 14, file_size);
  ctl_mmsetup_proc_0_6.text_p = (karlFarPtr_t)((char __huge *)str_filesize);
  zptrself = (uint32_t)((karlFarPtr_t)&ctl_mmsetup_proc_0_6);
  karlObjIncludeState(STATE_DIRTY);
  
  prepareKernalWrite(lflfilename, 10);
  if (kernal_get_last_error()) {
    processError("D81 PREPARE ERROR");
    return;
  }

  file_pos = FILE_MEMORY;

  zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_8);
  progressResetMax(file_size);

  char text2[] = "EXTRACT WRITE 00.LFL";
  writetwodecimalstr(text2, 14, rooms[roomidx].room);
  writeToProcOutput(text2);

  procstate = PROCST_WRITE;
}

void behaviourExtractWrite(void) {
  uint32_t next_size = 254;

  if (next_size > file_size) {
    next_size = file_size;
  }

  judeSetPointer(MPTR_WAIT);

  if (!procAbort) {
    performKernalWrite((uint32_t)file_pos, next_size);

    uint8_t error = kernal_get_last_error();

    if (error) {
      processError("D81 WRITE ERROR");
      finishKernalReadWrite();
      return;
    }
  }
  
  file_size -= next_size;
  file_pos += next_size;
  zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_8);
  progressIncValue(next_size);

  if (file_size == 0) {
    finishKernalReadWrite();

    if (kernal_get_status()) {
      processError("FILE CLOSE ERROR");
      return;
    }

    if (!procAbort) {
      roomidx++;
      procstate = PROCST_READ;

      zptrself = (uint32_t)((karlObject_t __huge *)&pgb_mmsetup_proc_0_4);
      progressIncValue(1);
    } else {
      procstate = PROCST_FINISH;
    }
  }

  judeSetPointer(MPTR_NORMAL);
}


void behaviourExtractFinish(void) {
  writeToProcOutput("EXTRACT FINISH");
  judeSetPointer(MPTR_NORMAL);
  
  doneProcFlags |= PROCFL_EXTRACT;
 
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;
}


#define UART_E_PRA  (*(volatile uint8_t *)              0xd607)
#define UART_E_DDR  (*(volatile uint8_t *)              0xd608)


void core_init(void) {
  CIA1.ddra   = 0xff; // set CIA1 port A as output
  CIA1.ddrb   = 0xff; // set CIA1 port B as output
  CIA1.pra    = 0xff; // connect mouse port 1 to SID1
  CIA1.prb    = 0xff; // pull all pins of port B high
  UART_E_DDR |= 0x02; // set UART_E pin as output
  UART_E_PRA |= 0x02; // set UART_E pin to high (controlling keyboard column C8 on the C65/MEGA65)
}
