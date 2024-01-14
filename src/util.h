#ifndef __UTIL_H
#define __UTIL_H

#include "dma.h"
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


void fatal_error(const char *message);

extern char msg[80];
#define debug_out(...) sprintf(msg, __VA_ARGS__); \
                       debug_msg(msg);

void debug_msg(char* msg);

// Memory functions (overloaded using DMA functionality)
void *memcpy(void *dest, const void *src, size_t n);
__far void *memcpy_to_bank(void __far *dest, const void *src, size_t n);
void memcpy_to_io(void __far *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);

#endif // __UTIL_H
