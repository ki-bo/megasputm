#include <stdint.h>
#include "hdos.h"
#include "adf.h"

#include "jude.h"
#include "karljr.h"
#include "mmsetup.h"

#include "mmsetup_core.h"

uint8_t config_required = 1;
uint8_t config_select = 0xff;

process_t process = PROC_NONE;
procstate_t procstate = PROCST_IDLE;

uint8_t procAbort = 0;
uint8_t procContinue = 0;


char file_extensions[3][4] = {"D81", "ADF", "D64"};

settingDetail_t dest_details[6] = {
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0, ""}, 
  {SETTINGT_NONE, 0, ""}
};

char adfFileName[] =  "MANIACM1.ADF";


char toUpperCase(char data) {
  if ((data >= 97) && (data <= 122)) {
    return data - 32;
  } else {
    return data;
  }
}


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

      if (found && (result < 255)) {
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
      }
    }
  }
  hdos_close_dir(desc);
  
  //if (!result) {
    //*(uint8_t *)(0xd020) = 10;
  //}

  return result;
}

void initiateProcess(void) {
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
  process = PROC_EXTRACT;
  procstate = PROCST_IDLE;

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

struct Device *dev;
struct Volume *vol;
struct List *list, *cell;
uint32_t file_size;

#define ADF_MEMORY ((uint8_t __huge *)0x08000000)
#define FILE_MEMORY ((uint8_t __huge *)0x40000)

void updateProcess(void) {
  switch(process) {
    case PROC_BUILD:
    case PROC_VERIFY: 
      break;  
    case PROC_EXTRACT: {
      //*(uint8_t *)(0xd020) = *(uint8_t *)(0xd020) + 1;
      switch(procstate) {
        case PROCST_IDLE:
          procstate = PROCST_INIT;
          break;
        case PROCST_INIT:
          judeSetPointer(MPTR_WAIT);
          
          //hdos_set_filename(adfFileName);
          //hdos_load_file_attic(0);

          uint8_t __huge *out = ADF_MEMORY;
          uint8_t data;
      
          while (!hdos_read_byte(&data)) {
            *out = data;
            ++out;
          }
      
          hdos_close_file();
      
          if (adf_init(ADF_MEMORY) != ADF_OK) {
            *(volatile uint8_t *)(0xd020) = 2;
            procstate = PROCST_FINISH;
            return;
          }
          
          if (adf_chdir("rooms") != ADF_OK) {
            *(volatile uint8_t *)(0xd020) = 2;
            procstate = PROCST_FINISH;
            return;
          }

          procstate = PROCST_WAIT;
          procContinue = 0;

          zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
          karlObjIncludeState(STATE_VISIBLE);

          zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
          karlObjIncludeState(STATE_ENABLED);
          
          judeSetPointer(MPTR_NORMAL);
          break;
        case PROCST_WAIT:
          if (procContinue) {
            zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_1_1);
            karlObjExcludeState(STATE_ENABLED);
            
            zptrself = (uint32_t)((karlObject_t __huge *)&pnl_mmsetup_proc_0);
            karlObjIncludeState(STATE_DIRTY);

            zptrself = (uint32_t)((karlObject_t __huge *)&ctl_mmsetup_proc_0_2);
            karlObjExcludeState(STATE_VISIBLE);

            procstate = PROCST_READ;

            procContinue = 0;
           } else if (procAbort) {
            procstate = PROCST_FINISH;
           }
          break;
        case PROCST_READ:
          if (adf_read_file("00.LFL", FILE_MEMORY, &file_size) != ADF_OK) {
            *(volatile uint8_t *)(0xd020) = 2;
            procstate = PROCST_FINISH;
            return;
          }
          
          prepareKernalWrite("@:00.LFL,S,W");

          procstate = PROCST_WRITE;
          break;
        case PROCST_WRITE:
          judeSetPointer(MPTR_WAIT);

          performKernalWrite((uint32_t)FILE_MEMORY, file_size);
          procstate = PROCST_FINISH;
          break;
        case PROCST_FINISH:
          finishKernalWrite();

          judeSetPointer(MPTR_NORMAL);
          process = PROC_COMPLETE;
          break;
        default:
          ;
      }
      break;
    }
    case PROC_COMPLETE:
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
      break;
    case PROC_NONE:
    default:
      break;
  }
}