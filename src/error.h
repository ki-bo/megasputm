#ifndef __ERROR_H__
#define __ERROR_H__

typedef enum {
    ERR_FILE_NOT_FOUND = 1,
    ERR_SECTOR_DATA_CORRUPT = 2,
    ERR_SECTOR_NOT_FOUND = 3,
    ERR_INVALID_DISK_LOCATION = 4,
    ERR_RESOURCE_NOT_FOUND = 5,
    ERR_RESOURCE_TOO_LARGE = 6,
    ERR_OUT_OF_RESOURCE_MEMORY = 7,
} error_code_t;

#endif // __ERROR_H__