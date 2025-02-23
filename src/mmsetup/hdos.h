#pragma once

#include <stdint.h>

typedef uint8_t err_t;

enum FILETYPE {
    FT_READONLY = 1,
    FT_HIDDEN = 2,
    FT_SYSTEM = 4,
    FT_VOLLABEL = 8,
    FT_SUBDIR = 16,
    FT_ARCHIVE = 32,
    FT_UNDEF6 = 64,
    FT_UNDEF7 = 128
};

typedef uint8_t filetype_t;  //set of FILETYPE

typedef struct DIRENT {
    char filename[64];   //no null?
    uint8_t namelen;                    //$40
    char fileshort[8];  //no null       //$41
    char fileext[3];    //no period     //$49
    
    uint8_t unknown[2];

    uint32_t cluster;
    uint32_t filelen;
    filetype_t filetype;
} dirent_t;

void hdos_init(uint16_t dataBuf, uint16_t xferBuf);

void hdos_closeall(void);
err_t hdos_set_filename(char *fileName);
err_t hdos_open_file(void);
void hdos_close_file(void);
err_t hdos_read_byte(uint8_t *data);
void hdos_close_dir(uint8_t desc);
err_t hdos_open_dir(uint8_t *desc);
err_t hdos_read_dir(uint8_t desc);
err_t hdos_change_dir(void);
err_t hdos_load_file_attic(uint32_t offset);
void hdos_getcurrdrive(uint8_t *drive);
void hdos_getdefdrive(uint8_t *drive);
err_t hdos_selectdrive(uint8_t drive);
err_t hdos_cdrootdir(uint8_t drive);

err_t hdos_attachD810(void);

void hdos_restart(void);
