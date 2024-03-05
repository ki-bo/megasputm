#ifndef __ERROR_H__
#define __ERROR_H__

enum errorcode_t {
    ERR_FILE_NOT_FOUND = 1,
    ERR_SECTOR_READ_FAILED = 2,
    ERR_INVALID_DISK_LOCATION = 3,
    ERR_RESOURCE_NOT_FOUND = 4,
    ERR_RESOURCE_TOO_LARGE = 5,
};

#endif // __ERROR_H__
