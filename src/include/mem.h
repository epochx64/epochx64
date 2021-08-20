#ifndef MEM_H
#define MEM_H

#include <defs/int.h>
#include <typedef.h>
#include <ipc.h>
#include <asm/asm.h>

typedef uint8_t byte;

void* operator new   ( size_t count );
void* operator new[] ( size_t count );

void operator delete (void *ptr, UINT64 len);
void operator delete[] (void *ptr);

void KeSysMemBitmapSet(UINT64 BlockID, UINT64 nBlocks = 1);
void *KeSysMalloc(UINT64 Size);

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

    template<class T>
    void swap(T &first, T &second)
    {
        T temp = first;

        first = second;
        second = temp;
    }
}

namespace heap
{
    void init();

    void free(void *ptr);
    void *malloc(UINT64 size);
}

#endif
