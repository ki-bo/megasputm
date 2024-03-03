#ifndef __UTIL_H
#define __UTIL_H

#include "error.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/// Poke a byte to the given address
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y
/// Poke two bytes to the given address
#define POKE16(X, Y) (*(volatile uint16_t*)(X)) = Y
/// Poke four bytes to the given address
#define POKE32(X, Y) (*(volatile uint32_t*)(X)) = Y
/// Peek a byte from the given address
#define PEEK(X) (*(volatile uint8_t*)(X))
/// Peek two bytes from the given address
#define PEEK16(X) (*(volatile uint16_t*)(X))
/// Peek four bytes from the given address
#define PEEK32(X) (*(volatile uint32_t*)(X))

#define NEAR_U8_PTR(X) ((uint8_t *)(X))
#define NEAR_I8_PTR(X) ((int8_t *)(X))
#define NEAR_U16_PTR(X) ((uint16_t *)(X))
#define NEAR_I16_PTR(X) ((int16_t *)(X))
#define NEAR_U32_PTR(X) ((uint32_t *)(X))
#define NEAR_I32_PTR(X) ((int32_t *)(X))

#define NEAR_VU8_PTR(X) ((volatile uint8_t *)(X))
#define NEAR_VI8_PTR(X) ((volatile int8_t *)(X))
#define NEAR_VU16_PTR(X) ((volatile uint16_t *)(X))
#define NEAR_VI16_PTR(X) ((volatile int16_t *)(X))
#define NEAR_VU32_PTR(X) ((volatile uint32_t *)(X))
#define NEAR_VI32_PTR(X) ((volatile int32_t *)(X))

#define FAR_U8_PTR(X) ((__far uint8_t *)(X))
#define FAR_I8_PTR(X) ((__far int8_t *)(X))

#define FAR_VU8_PTR(X) ((__far volatile uint8_t *)(X))
#define FAR_VI8_PTR(X) ((__far volatile int8_t *)(X))

#define LSB(X)   ((uint8_t)((uint8_t)(X)))
#define MSB(X)   ((uint8_t)((uint16_t)(X) >> 8))
#define LSB16(X) ((uint16_t)(X))
#define MB(X)    ((uint8_t)((uintptr_t)(X) >> 16))
#define BANK(X)  ((uint8_t)(((uint8_t)((uintptr_t)(X) >> 16)) & 0x0f))

void fatal_error(enum errorcode_t error);
void fatal_error_str(const char *message);

#if 1
extern char msg[80];
#define debug_out(...) sprintf(msg, __VA_ARGS__); \
                       debug_msg(msg);
#else
#define debug_out(...)
#endif

void debug_msg(char* msg);

// Memory functions (overloaded using DMA functionality)
void *memcpy(void *dest, const void *src, size_t n);
void __far *memcpy_to_bank(void __far *dest, const void *src, size_t n);
void __far *memcpy_far(void __far *dest, const void __far *src, size_t n);
void memcpy_to_io(void __far *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif // __UTIL_H
