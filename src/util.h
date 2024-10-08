/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "error.h"
#include <calypsi/intrinsics6502.h>
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

#define FAR_U8_PTR(X) ((uint8_t __far *)(X))
#define FAR_I8_PTR(X) ((int8_t __far *)(X))
#define FAR_U16_PTR(X) ((uint16_t __far *)(X))
#define FAR_I16_PTR(X) ((int16_t __far *)(X))

#define FAR_VU8_PTR(X) ((volatile uint8_t __far *)(X))
#define FAR_VI8_PTR(X) ((volatile int8_t __far *)(X))

#define HUGE_U8_PTR(X) ((uint8_t __huge *)(X))
#define HUGE_I8_PTR(X) ((int8_t __huge *)(X))
#define HUGE_U16_PTR(X) ((uint16_t __huge *)(X))
#define HUGE_I16_PTR(X) ((int16_t __huge *)(X))

#define HUGE_VU8_PTR(X) ((volatile uint8_t __huge *)(X))
#define HUGE_VI8_PTR(X) ((volatile int8_t __huge *)(X))

#define LSB(X)   ((uint8_t)(X))
#define MSB(X)   ((uint8_t)((uint16_t)(X) >> 8))
#define LSB16(X) ((uint16_t)(X))
#define BANK(X)  ((uint8_t)(((uint8_t)((uintptr_t)(X) >> 16)) & 0x0f))
#define MB_LO(X) ((uint8_t)(((uint8_t)((uintptr_t)(X) >> 16)) >> 4));

#define U8(x) ((uint8_t)(x))

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

void fatal_error(error_code_t error);
void fatal_error_str(const char *message);

uint8_t abs8(int8_t x);

#ifdef DEBUG
extern char msg[80];
#define debug_out(...) sprintf(msg, __VA_ARGS__); \
                       debug_msg(msg);
#define debug_out2(...) sprintf(msg, __VA_ARGS__); \
                        debug_msg2(msg);
#else
#define debug_out(...)
#define debug_out2(...)
#endif

void debug_msg(char* msg);
void debug_msg2(char* msg);

// Memory functions (overloaded using DMA functionality)
void __far *memcpy_to_bank(void __far *dest, const void *src, size_t n);
void __far *memcpy_chipram(void __far *dest, const void __far *src, size_t n);
void __far *memcpy_far(void __far *dest, const void __far *src, size_t n);
void __far *memset20(void __far *s, int c, size_t n);
void __far *memset32(void __far *s, uint32_t c, size_t n);

inline uint16_t make16(uint8_t low, uint8_t high) 
{
  uint16_t result;
  __asm(" sta %0\n"
        " stx %0+1\n"
        : "=Kzp16"(result)
        : "Ka"(low),
          "Kx"(high)
        :);
  return result;
}

/**
 * @brief Optimized signed division by 8 for int16_t numbers (arithmetic shift incl. rounding toward zero)
 */
inline int16_t i16_div_by_8(int16_t x)
{
  if (x < 0) {
    x += 7;
  }
  return x >> 3;
}
