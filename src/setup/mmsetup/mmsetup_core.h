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

extern process_t process;
extern procstate_t procstate;


extern uint8_t config_required;
extern uint8_t config_select;
extern char file_extensions[3][4];

extern uint32_t kernalWriteSrc;
extern uint32_t kernalWriteSiz;

extern settingDetail_t dest_details[6];

uint8_t readDirectoryFiles(const char *ext);


void initiateProcess(void);
void cancelProcess(void);
void continueProcess(void);
void updateProcess(void);

void prepareKernalWrite(char *filename);
extern void _prepareKernalWrite(void);

void performKernalWrite(uint32_t source, uint32_t size);
extern void _performKernalWrite(void);

void finishKernalWrite(void);
extern void _finishKernalWrite(void);


extern void processTest(void);

