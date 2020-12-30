#ifndef MEM_H
#define MEM_H

#include <defs/int.h>
#include <kernel/typedef.h>

typedef uint8_t byte;

void* operator new   ( size_t count );
void* operator new[] ( size_t count );

void SysMemBitMapSet(UINT64 BlockID, UINT64 nBlocks = 1);
void *SysMalloc(UINT64 Size);

namespace mem
{
    static void FORCE_INLINE set(byte *dst, byte val, uint64_t count)
    {
        for ( uint64_t i = 0; i < count; i++ )
        {
            dst[i] = val;
        }
    }

    static void FORCE_INLINE copy(byte *src, byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = src[i];
        }
    }

    static void FORCE_INLINE zero(byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = 0x00;
        }
    }
}

namespace heap
{
    /*
     * malloc function but with align
     */
    void *MallocAligned(UINT64 Size, UINT64 Align);
}

#endif
