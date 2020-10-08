#ifndef MEM_H
#define MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte;

namespace mem
{
    void set(byte *dst, byte val, uint64_t count);
    void copy(byte *src, byte *dst, size_t count);
    void zero(byte *dst, size_t count);
    uint8_t *alloc(size_t count);
    void dealloc(byte *log);
}

#endif
