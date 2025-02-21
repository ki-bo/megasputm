#include <stdint.h>
#include "hdos.h"

#include "mmsetup_core.h"

uint8_t config_required = 1;
uint8_t config_select = 0xff;

char file_extensions[3][4] = {"D81", "ADF", "D64"};

settingDetail_t dest_details[6] = {
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0xff, ""}, 
  {SETTINGT_NONE, 0, ""}, 
  {SETTINGT_NONE, 0, ""}
};



char toUpperCase(char data) {
  if ((data >= 97) && (data <= 122)) {
    return data - 32;
  } else {
    return data;
  }
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