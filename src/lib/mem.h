#ifndef MEM_H
#define MEM_H

#include <defs/int.h>
#include <kernel/typedef.h>

typedef uint8_t byte;

void* operator new   ( size_t count );
void* operator new[] ( size_t count );

namespace mem
{
    void set(byte *dst, byte val, uint64_t count);
    void copy(byte *src, byte *dst, size_t count);
    void zero(byte *dst, size_t count);
}

namespace heap
{
    /*
     * malloc function but with align
     */
    void *MallocAligned(UINT64 Size, UINT64 Align);
}

#endif
