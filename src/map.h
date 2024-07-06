#pragma once

#include <stdint.h>

union map_t {
  struct {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t z;
  };
  struct {
    uint16_t cs;
    uint16_t ds;
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
    __asm(" .extern map_cs_diskio\n" \
          " jsr map_cs_diskio\n" \
          : \
          : \
          : "x", "y", "z");

#define MAP_CS_GFX \
    __asm(" .extern map_cs_gfx\n" \
          " jsr map_cs_gfx\n" \
          : \
          : \
          : "x", "y", "z");

#define MAP_CS_GFX2 \
    __asm(" .extern map_cs_gfx2\n" \
          " jsr map_cs_gfx2\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_CS \
    __asm(" .extern unmap_cs\n" \
          " jsr unmap_cs\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_DS \
    __asm(" .extern unmap_ds\n" \
          " jsr unmap_ds\n" \
          : \
          : \
          : "x", "y", "z");

#define UNMAP_ALL \
    __asm(" .extern unmap_all\n" \
          " jsr unmap_all\n" \
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
          " phw #.swap(map_auto_restore_cs - 1)" \
          :::);

#define SAVE_DS \
    __asm(" .extern map_regs\n" \
          " phw map_regs + 2\n" \
          :::);

#define SAVE_DS_AUTO_RESTORE \
    __asm(" .extern map_regs\n" \
          " .extern map_auto_restore_ds\n" \
          " phw map_regs + 2\n" \
          " phw #.swap(map_auto_restore_ds - 1)" \
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

/**
  * @brief Map the DS to the specified address.
  *
  * Will map DS to the address of ptr in a way that the first
  * address of the memory ptr is pointing to will land in the
  * first 256 bytes page of the mapped DS. The total mapped window will 
  * always be at 0x8000-0xbfff.
  *
  * Example: If mapping 0x12345 to DS, then 0x12300 will be mapped
  * to 0x8000 and the first byte of the mapped memory will be available
  * at 0x8045. The function will then return the pointer (uint8_t *)0x8045.
  *
  * @param ptr A 28 bit pointer to map to DS
  * @return The pointer to the mapped memory (pointer will be between
  *         0x8000 and 0x80ff)
  */
uint8_t *map_ds_ptr(void __huge *ptr);

/**
  * @brief Maps a resource slot to DS (0x8000-0xbfff)
  * 
  * Resource slots are located at RESOURCE_MEMORY (0x18000-0x27fff). 
  * This function maps the memory starting at 0x18000 + (res_page * 256) to 0x8000.
  * We are always mapping 16KB of memory.
  *
  * @param res_page The resource slot to map. The slot is 0-indexed.
  *
  * Code section: code
  */
void map_ds_resource(uint8_t res_page);

/**
  * @brief Maps the DS to the heap memory
  * 
  * Maps the DS to the default heap memory. The heap memory is located at the beginning of
  * the resource memory (0x18000-0x187ff). The heap is always 1 page (256 bytes) in size.
  *
  * Code section: code
  */
void map_ds_heap(void);

/**
  * @brief Maps a room offset to DS
  *
  * Maps a room offset to DS. The room offset is a 16-bit value that points to a location
  * in the room data. The room data is stored in the resource memory.
  *
  * The room data is mapped so that the specified offset will be between 0x8000 and 0x80ff.
  * The mapped block is 16kb in size (mapped to 0x8000 - 0xbfff).
  * 
  * @param room_offset The room offset to map
  * @return* The pointer to the mapped room offset (is between 0x8000 and 0x80ff)
  *
  * Code section: code
  */
uint8_t *map_ds_room_offset(uint16_t room_offset);
