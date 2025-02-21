#include <stdint.h>

#include "mega65.h"
#include "hdos.h"
//#include "_kernal.h"

#include "adf.h"
#include "room90.h"

#include "jude.h"
#include "karljr.h"
#include "mmsetup.h"
#include "mmsetup_core.h"


uint8_t config_select = 0;

process_t process = PROC_NONE;
procstate_t procstate = PROCST_IDLE;

uint8_t procAbort = 0;
uint8_t procContinue = 0;
uint8_t outputDisk;
uint8_t roomidx;
uint8_t haveRoom90 = 0;
uint16_t room90size = 0;

uint8_t configProcFlags = 0;
uint8_t doneProcFlags = 0;


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

//PROC_NONE,
//PROC_CONFIGURE,
//PROC_EXTRACT,
//PROC_BUILD,
//PROC_VERIFY,
//PROC_COMPLETE


//void (* onIdle)(void);
//void (* onInit)(void);
//void (* onWait)(void);
//void (* onRead)(void);
//void (* onWrite)(void);
//void (* onFinish)(void);


procbehaviour_t proc_behaviours[] = {
  {0, 0, 0, 0, 0, 0},
  {&behaviourConfigIdle, &behaviourConfigInit, 0, 0, 0, 0},
  {&behaviourExtractIdle, &behaviourExtractInit, &behaviourExtractWait, &behaviourExtractRead, &behaviourExtractWrite, &behaviourExtractFinish},
  {&behaviourBuildIdle, &behaviourBuildInit, 0, 0, 0, &behaviourBuildFinish},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0}
}; 

char adfFileName[65];
char lflfilename[] = "@:00.LFL,S,W";
char lflfileadf[] = "00.LFL";

struct Device *dev;
struct Volume *vol;
struct List *list, *cell;
uint32_t file_size;
uint8_t __huge *procOutput;


#define ADF_MEMORY ((uint8_t __huge *)0x08000000)
#define FILE_MEMORY ((uint8_t __huge *)0x40000)


extern uint16_t mouseXPos;
extern uint16_t mouseYPos;
extern uint8_t kernal_get_last_error(void);


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
        : "a", "x");

  if (abs_diff > 10) {
    return (value << 2) + value;
  }
  if (abs_diff > 5) {
    return (value << 1) + value;
  }
  return value;
}

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

  //while (1) {
    //__asm(" inc 0xd020 ");
  //}

  handle_mouse();
  //handle_keyboard();
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

void prepareLFLFileName(uint8_t roomno) {
  uint8_t tens = (roomno / 10);
  uint8_t ones = (roomno - (tens * 10)) + 0x30;
  tens +=  0x30;

  lflfilename[2] = tens;
  lflfilename[3] = ones;
  lflfileadf[0] = tens;
  lflfileadf[1] = ones;
}


void performKernalHeaderChange(uint8_t diskNo) {
  __asm(
    " .extern _performKernalHeaderChange \n"
    //"   lda %[dn] \n"
    "   jsr _performKernalHeaderChange \n"
    :
    : "Ka" (diskNo)
    : "a", "x", "y", "z"
  );
};

void performKernalScatchAllRooms(void) {
  __asm(
    " .extern _performKernalScatchAllRooms \n"
    //"   lda %[dn] \n"
    "   jsr _performKernalScatchAllRooms \n"
    :
    : 
    : "a", "x", "y", "z"
  );
};


void prepareKernalWrite(char *filename) {
  __asm(
    " .extern _prepareKernalWrite \n"
    "   lda %[fn] \n"
    "   ldx %[fn] + 1 \n"
    "   jsr _prepareKernalWrite \n"
    :
    : [fn] "Kzp16" (filename)
    : "a", "x", "y", "z"
  );
}

void performKernalWrite(uint32_t source, uint32_t size) {
  kernalWriteSrc = source;
  kernalWriteSiz = size;

  _performKernalWrite();
}

void finishKernalWrite(void) {
  _finishKernalWrite();
}



//const char defstr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+_[]!@#$%^&*()              ";

uint8_t readDirectoryFiles(const char *ext) {
  uint8_t result = 0;
  uint8_t drive, desc;
  dirent_t *entry = (dirent_t *)(0x0800);

  hdos_closeall();

  hdos_close_file();

  hdos_getdefdrive(&drive);
  //hdos_getcurrdrive(&drive);

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

      //if (entry->filetype & FT_SYSTEM) {
        //while(1) {
          //*(uint8_t *)(0xd020) = *(uint8_t *)(0xd020) + 1;
        //}
      //}

      for (uint8_t i = 0; i < 3; ++i) {
        if (toUpperCase(entry->fileext[i]) != ext[i]) {
          found = 0;
          break;
        }
      }

      if (found && (result < 254)) {
        result++;

        /*for (uint8_t i = 0; i < 65; ++i) {
          *out = defstr[i];
          ++out;
        }*/

        for (uint8_t i = 0; i < 64; ++i) {
          *out = entry->filename[i];
          ++out;
        }
        //for (uint8_t i = 0; i < 65 - entry->namelen; ++i) {
          *out = 0;
          ++out;
        //}
          *out = entry->namelen;
          ++out;
      }
    }
  }
  hdos_close_dir(desc);
  
  //if (!result) {
    //*(uint8_t *)(0xd020) = 10;
  //}

  return result;
}


uint8_t configurationInvalid(void) {
  uint8_t result = 0;

  if (ctl_mmsetup_welc_0_3._element._object.tag) {
    //configProcFlags |= PROCFL_EXTRACT;
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

  if (ctl_mmsetup_welc_0_4._element._object.tag) {
    //configProcFlags |= PROCFL_BUILD;
    if (dest_details[4].type == SETTINGT_NONE || dest_details[5].type == SETTINGT_NONE) {
      result = 2;
    }
  }

  return result;
}

void initiateProcess(void) {
  outputDisk = 0;
  roomidx = 0;
  haveRoom90 = 0;

  procOutput = (uint8_t __huge *)LISTBOXLINESMEM;

  lbx_mmsetup_proc_0_11.linescnt = 0;
  lbx_mmsetup_proc_0_11.currline = 0;
  lbx_mmsetup_proc_0_11.linesoff = 0;
  lbx_mmsetup_proc_0_11.hotline = 0;
  lbx_mmsetup_proc_0_11.selline = 0xff;

  zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_proc_0_11);
  karlObjExcludeState(STATE_ENABLED);
  karlObjIncludeState(STATE_CHANGED);

  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
  karlObjExcludeState(STATE_ENABLED);

  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_0);
  karlObjExcludeState(STATE_ENABLED);

  zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_2);
  karlObjIncludeState(STATE_ENABLED);

  judeActivateCtrl();

  __asm(
    "   sei \n"
  );

  procAbort = 0;
  procContinue = 0;
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;

  //configProcFlags = PROCFL_CONFIGURE | PROCFL_BUILD | PROCFL_EXTRACT;
  doneProcFlags = 0;

  __asm(
    "   cli \n"
  );
}


void cancelProcess(void) {
  __asm(
    "   sei \n"
  );

  procAbort = 1;

  __asm(
    "   cli \n"
  );
}

void continueProcess(void) {
  __asm(
    "   sei \n"
  );

  procContinue = 1;

  __asm(
    "   cli \n"
  );
}


void copyFileName(uint8_t detail, char *fileName) {
  for (uint8_t i = 0; i < dest_details[detail].namelen; ++i) {
    fileName[i] = dest_details[detail].fileName[i];

    *(uint8_t *)(0x0800 + i) = dest_details[detail].fileName[i];
  }
  fileName[dest_details[detail].namelen] = 0;
}

uint8_t disk1Rooms[] = 
    {0, 30, 33, 40, 44, 45, 49, 50,
    51, 53, 0xff};
uint8_t disk2Rooms[] =
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29,  
    31, 32, 34, 35, 36, 37, 38, 39, 41, 
    42, 43, 44, 46, 47, 48, 52, 53, 0xff};

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


void updateProcess(void) {
  if (process ==  PROC_COMPLETE) {
    writeToProcOutput("COMPLETE");

    zptrself = (uint32_t)((karlObject_t __huge *)&lbx_mmsetup_proc_0_11);
    karlObjIncludeState(STATE_ENABLED);

        zptrself = (uint32_t)((karlObject_t __huge *)&pnl_mmsetup_proc_0);
    karlObjIncludeState(STATE_DIRTY);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
    karlObjExcludeState(STATE_VISIBLE);
    
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_2);
    karlObjExcludeState(STATE_ENABLED);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
    karlObjExcludeState(STATE_ENABLED);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_0);
    karlObjIncludeState(STATE_ENABLED);

    judeActivateCtrl();

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


void behaviourConfigIdle(void) {
  procstate = PROCST_INIT;
}

void behaviourConfigInit(void) {
  //find next job or disk
  writeToProcOutput("CONFIGURE");

  if (outputDisk > 1) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
  } else {
    if (configProcFlags & PROCFL_BUILD && !(doneProcFlags & PROCFL_BUILD)) {
      process = PROC_BUILD;
      procstate = PROCST_IDLE;
    } else if (configProcFlags & PROCFL_EXTRACT && !(doneProcFlags & PROCFL_EXTRACT)) {
      process = PROC_EXTRACT;
      procstate = PROCST_IDLE;
    } else {
      //try next disk      
      outputDisk++;
      doneProcFlags = 0;
      procstate = PROCST_IDLE;
    }
  }
}


void behaviourBuildIdle(void) {
  procstate = PROCST_INIT;
}

void behaviourBuildInit(void) {
  writeToProcOutput("BUILD INIT");
  if (!haveRoom90) {
    judeSetPointer(MPTR_WAIT);

    copyFileName(4, adfFileName);

    if (hdos_set_filename(adfFileName)) {
      while(1) {
        __asm(" inc 0xd020 ");
      }
    };
    hdos_load_file_attic(0);

    copyFileName(5, adfFileName);
    if (hdos_set_filename(adfFileName)) {
      while(1) {
        __asm(" inc 0xd020 ");
      }
    };
    hdos_load_file_attic(0x32000);
    
    readIndexFile((uint8_t __huge *)(0x08000000));
    
    uint8_t __huge *image1 = (uint8_t __huge *)(0x08000000);
    uint8_t __huge *image2 = (uint8_t __huge *)(0x08032000);
    uint8_t __huge *dest = (uint8_t __huge *)(0x0001B000);
    
    room90size = makeRoom90(image1,image2, dest);

    haveRoom90 = 1;

    judeSetPointer(MPTR_WAIT);
  }

  procstate = PROCST_FINISH;
}

void behaviourBuildFinish(void) {
  writeToProcOutput("BUILD FINISH");
  
  doneProcFlags |= PROCFL_BUILD;
  process = PROC_CONFIGURE;
  procstate = PROCST_IDLE;
}


void behaviourExtractIdle(void) {
    procstate = PROCST_INIT;
}

void behaviourExtractInit(void) {
  writeToProcOutput("EXTRACT INIT");

  if (outputDisk > 1) {
    process = PROC_COMPLETE;
    procstate = PROCST_IDLE;
    return;
  } else if  (dest_details[outputDisk].type == SETTINGT_NONE) {
    //outputDisk++;
    procstate = PROCST_FINISH;
    return;
  }
            
  judeSetPointer(MPTR_WAIT);
  roomidx = 0;
  
  copyFileName(outputDisk + 2, adfFileName);
  hdos_set_filename(adfFileName);
  //if (hdos_open_file()) {
    //procstate = PROCST_FINISH;
    //return;
  //}

  hdos_load_file_attic(0);

  //while (!hdos_read_byte(&data)) {
    //*out = data;
    //++out;
  //}

  //hdos_close_file();

  if (adf_init(ADF_MEMORY) != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    
    //while(1) {
      //__asm(" inc 0xd020 ");
    //}
    
    writeToProcOutput("ADF INIT ERROR");

    procstate = PROCST_FINISH;
    return;
  }
  
  if (adf_chdir("rooms") != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    writeToProcOutput("ADF CHDIR ERROR");
    procstate = PROCST_FINISH;
    return;
  }

  if (dest_details[outputDisk].type == SETTINGT_DISK) {
    procstate = PROCST_WAIT;
    procContinue = 0;

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
    karlObjIncludeState(STATE_VISIBLE);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
    karlObjIncludeState(STATE_ENABLED);
  } else if (dest_details[outputDisk].type == SETTINGT_IMGE) {
    copyFileName(outputDisk, adfFileName);
    
    
    if (hdos_set_filename(adfFileName)) {
      writeToProcOutput(adfFileName);
      writeToProcOutput("SETNAME D81 ERROR");
      //outputDisk++;
      procstate = PROCST_FINISH;
    } else if (hdos_attachD810()) {
      writeToProcOutput(adfFileName);
      writeToProcOutput("MOUNT D81 ERROR");
      //outputDisk++;
      procstate = PROCST_FINISH;
    } else {
      performKernalHeaderChange(outputDisk + 1);
      performKernalScatchAllRooms();
      procstate = PROCST_READ;
    }
  }

  judeSetPointer(MPTR_NORMAL);
}

void behaviourExtractWait(void) {
  if (procContinue) {
    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
    karlObjExcludeState(STATE_ENABLED);
    
    zptrself = (uint32_t)((karlObject_t __huge *)&pnl_mmsetup_proc_0);
    karlObjIncludeState(STATE_DIRTY);

    zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
    karlObjExcludeState(STATE_VISIBLE);

    performKernalHeaderChange(outputDisk + 1);
    performKernalScatchAllRooms();
    procstate = PROCST_READ;

    procContinue = 0;
  } else if (procAbort) {
  procstate = PROCST_FINISH;
  }
}

void behaviourExtractRead(void) {
  uint8_t *rooms;
  
  char text[] = "EXTRACT READ   ";
  text[14] = roomidx + 0x30;
  writeToProcOutput(text);

  if (outputDisk == 0) {
    rooms = disk1Rooms;
  } else {
    rooms = disk2Rooms;
  }

  if (rooms[roomidx] == 0xff) {
    roomidx = 0;
    //outputDisk++;
    procstate = PROCST_FINISH;
    return;
  }

  prepareLFLFileName(rooms[roomidx]);

  if (adf_read_file(lflfileadf, FILE_MEMORY, &file_size) != ADF_OK) {
    *(volatile uint8_t *)(0xd020) = 2;
    writeToProcOutput("ADF READ ERROR");

    procstate = PROCST_FINISH;

    return;
  }
  
  prepareKernalWrite(lflfilename);
  if (kernal_get_last_error()) {
    writeToProcOutput("D81 PREPARE ERROR");

    while(1) {
      __asm(" inc 0xd020 ");
    }

    procstate = PROCST_FINISH;
    return;
  }

  procstate = PROCST_WRITE;
}

void behaviourExtractWrite(void) {
  writeToProcOutput("EXTRACT WRITE");

  judeSetPointer(MPTR_WAIT);

  if (!procAbort) {
    performKernalWrite((uint32_t)FILE_MEMORY, file_size);

    uint8_t error = kernal_get_last_error();

    if (error) {
      writeToProcOutput("D81 WRITE ERROR");

      *(uint8_t *)(0x0882) = error;

      while(1) {
        __asm(" inc 0xd020 ");
      }
  
      finishKernalWrite();
      procstate = PROCST_FINISH;
      return;
    }
  }
  finishKernalWrite();

  //procstate = PROCST_FINISH;

  if (!procAbort) {
    roomidx++;
    procstate = PROCST_READ;
  } else {
    procstate = PROCST_FINISH;
  }
  judeSetPointer(MPTR_NORMAL);
}


void behaviourExtractFinish(void) {
  if (haveRoom90 && !procAbort) {
    writeToProcOutput("WRITE ROOM 90");

    prepareLFLFileName(90);
    prepareKernalWrite(lflfilename);
    performKernalWrite((uint32_t)((uint8_t __huge *)0x01B000), room90size);
    finishKernalWrite();
  }

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