#ifndef __MAP_H
#define __MAP_H

#include <stdint.h>

union map_t {
  struct {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t z;
  };
  uint32_t quad;
};

extern union map_t __attribute__((zpage)) map_regs;

#define MAP_CS_MAIN_PRIV \
    __asm(" .extern map_cs_main_priv\n" \
          " jsr map_cs_main_priv\n" \
          : \
          : \
          : "x", "y", "z");

#define MAP_CS_DISKIO \
    __asm(" .extern map_cs_diskio2\n" \
          " jsr map_cs_diskio2\n" \
          : \
          : \
          : "x", "y", "z");

#define MAP_CS_GFX \
    __asm(" .extern map_cs_gfx2\n" \
          " jsr map_cs_gfx2\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_CS \
    __asm(" .extern unmap_cs2\n" \
          " jsr unmap_cs2\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_DS \
    __asm(" .extern unmap_ds2\n" \
          " jsr unmap_ds2\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_ALL \
    __asm(" .extern unmap_all2\n" \
          " jsr unmap_all2\n" \
          : \
          : \
          : "x", "y", "z");

#define SAVE_MAP \
    __asm(" .extern map_regs\n" \
          " phw map_regs\n" \
          " phw map_regs + 2\n" \
          :::);

#define SAVE_CS \
    __asm(" .extern map_regs\n" \
          " phw map_regs\n" \
          :::);

#define SAVE_CS_AUTO_RESTORE \
    __asm(" .extern map_regs\n" \
          " .extern map_auto_restore_cs\n" \
          " phw map_regs\n" \
          /* 0xf4 = PHW immediate mode */ \
          " .byte 0xf4, .byte1(map_auto_restore_cs-1), .byte0(map_auto_restore_cs-1)\n" \
          :::);

#define SAVE_DS \
    __asm(" .extern map_regs\n" \
          " phw map_regs + 2\n" \
          :::);

#define SAVE_DS_AUTO_RESTORE \
    __asm(" .extern map_regs\n" \
          " .extern map_auto_restore_ds\n" \
          " phw map_regs + 2\n" \
          /* 0xf4 = PHW immediate mode */ \
          " .byte 0xf4, .byte1(map_auto_restore_ds-1), .byte0(map_auto_restore_ds-1)\n" \
          :::);

#define RESTORE_MAP \
    __asm(" .extern map_restore\n" \
          " plz\n" \
          " ply\n" \
          " plx\n" \
          " pla\n" \
          " map\n" \
          " eom\n" \
          : \
          : \
          : "a", "x", "y", "z");

#define RESTORE_CS \
    __asm(" .extern map_restore_cs\n" \
          " plx\n" \
          " pla\n" \
          " jsr map_restore_cs\n" \
          : \
          : \
          : "a", "x", "y", "z");

#define RESTORE_DS \
    __asm(" .extern map_restore_ds\n" \
          " plz\n" \
          " ply\n" \
          " jsr map_restore_ds\n" \
          : \
          : \
          : "a", "x", "y", "z");

// code functions
void map_init(void);
uint32_t map_get(void);
uint16_t map_get_cs(void);
uint16_t map_get_ds(void);
void map_set(uint32_t map_reg);
void map_set_cs(uint16_t map_reg);
void map_set_ds(uint16_t map_reg);
void unmap_all(void);
void unmap_cs(void);
void unmap_ds(void);
void map_cs_diskio(void);
void map_cs_gfx(void);
uint8_t *map_ds_ptr(void __huge *ptr);
void map_ds_resource(uint8_t res_page);
void map_ds_heap(void);
uint8_t *map_ds_room_offset(uint16_t room_offset);

#endif // __MAP_H
