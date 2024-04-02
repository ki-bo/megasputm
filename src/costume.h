#ifndef __COSTUME_H
#define __COSTUME_H

#include <stdint.h>

struct costume_header {
    uint16_t chunk_size;
    uint16_t unused1;
    uint8_t  num_animations;
    uint8_t  enable_mirroring_and_format;
    uint8_t  unused2;
    uint16_t animation_commands_offset;
    uint16_t limb_table_offsets[16];
};

#endif // __COSTUME_H
