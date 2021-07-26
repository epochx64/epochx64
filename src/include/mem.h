#ifndef MEM_H
#define MEM_H

#include <defs/int.h>
#include <typedef.h>
#include <ipc.h>

#define memset64(buffer, length, value) for(UINT64 i = 0; i < (length); i+=8) *(UINT64*)((UINT64)(buffer) + i) = value
#define memset32(buffer, length, value) for(UINT64 i = 0; i < length; i+=4) *(UINT32*)((UINT64)buffer + i) = value
#define memset16(buffer, length, value) for(UINT64 i = 0; i < length; i+=2) *(UINT16*)((UINT64)buffer + i) = value
#define memset8(buffer, length, value) for(UINT64 i = 0; i < length; i++) *(UINT8*)((UINT64)buffer + i) = value

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

    /*
     * malloc function but with align
     * Strongly advised to not use because you cannot free() this yet
     */
    void *MallocAligned(UINT64 Size, UINT64 Align);
}

#endif
