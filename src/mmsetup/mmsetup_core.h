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


#include <stdint.h>

#define LISTBOXLINESMEM 0x00016000

typedef enum SETTINGTYPE {
  SETTINGT_NONE,
  SETTINGT_IMGE,
  SETTINGT_DISK
} settingType_t;

typedef struct SETTINGDETAIL {
  settingType_t type;
  uint8_t index;
  char fileName[65];
  uint8_t namelen;
} settingDetail_t;

typedef enum PROCESS {
  PROC_NONE,
  PROC_CONFIGURE,
  PROC_EXTRACT,
  PROC_BUILD,
  PROC_VERIFY,
  PROC_COMPLETE
} process_t;

typedef enum PROCSTATE {
  PROCST_IDLE,
  PROCST_INIT,
  PROCST_WAIT,
  PROCST_READ,
  PROCST_WRITE,
  PROCST_FINISH
} procstate_t;

typedef enum PROCFLAGS {
  PROCFL_CONFIGURE = 1,
  PROCFL_EXTRACT = 2,
  PROCFL_BUILD = 4,
  PROCFL_VERIFY = 8,
} procflags_t;

typedef void (* procbehaviour_t[6])(void);

extern process_t process;
extern procstate_t procstate;

extern procbehaviour_t proc_behaviours[]; 


extern uint8_t config_select;
extern uint8_t configProcFlags;


extern char file_extensions[3][4];

extern uint32_t kernalWriteSrc;
extern uint32_t kernalWriteSiz;

extern settingDetail_t dest_details[];

extern uint32_t bootflags;

uint8_t readDirectoryFiles(const char *ext);

uint8_t configurationInvalid(void);
void initiateProcess(void);
void cancelProcess(void);
void continueProcess(void);
void updateProcess(void);

void prepareKernalWrite(char *filename, uint8_t len);
void performKernalWrite(uint32_t source, uint32_t size);
void finishKernalWrite(void);

uint8_t kernal_get_status(void);

void core_init(void);

void behaviourConfigIdle(void);
void behaviourConfigInit(void);
void behaviourConfigWait(void);
void behaviourBuildIdle(void);
void behaviourBuildInit(void);
void behaviourBuildRead(void);
void behaviourBuildWrite(void);
void behaviourBuildFinish(void);
void behaviourExtractIdle(void);
void behaviourExtractInit(void);
void behaviourExtractRead(void);
void behaviourExtractWrite(void);
void behaviourExtractFinish(void);
