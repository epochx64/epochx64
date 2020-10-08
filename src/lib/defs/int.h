#ifndef _INT_H
#define _INT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef UINT64 UINTN;

#define FORCE_INLINE __attribute__((always_inline))
#endif