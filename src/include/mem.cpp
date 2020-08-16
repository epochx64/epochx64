#include "mem.h"

namespace mem
{
    void set(byte *dst, byte val, uint64_t count)
    {
        for ( uint64_t i = 0; i < count; i++ )
        {
            dst[i] = val;
        }
    }

    void copy(byte *src, byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = src[i];
        }
    }

    void zero(byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = 0x00;
        }
    }

    uint8_t *alloc(size_t count)
    {
        return 0;
    }

    void dealloc(byte *loc)
    {

    }
}
