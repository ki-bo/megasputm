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
} settingDetail_t;


extern uint8_t config_required;
extern uint8_t config_select;
extern char file_extensions[3][4];

extern settingDetail_t dest_details[6];

uint8_t readDirectoryFiles(const char *ext);

