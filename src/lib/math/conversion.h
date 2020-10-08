#ifndef _CONVERSION_H
#define _CONVERSION_H

#include <defs/int.h>
#include "math.h"

namespace conversion
{
    /*
     * Convert any 8,16,32,64 bit integer to its
     * hex representation. Caller should allocate space
     * for buf
     */
    template <class T> void to_hex(T num, char* buf)
    {
        uint8_t hex_size    = sizeof(num) * 2;

        for ( uint8_t i = 0; i < hex_size; i++ )
        {
            uint8_t c = ((uint8_t)((UINT64)num >> (4*i))) & 0x0F;

            //  ASCII conversion; capital
            if ( c > 9 ) c += 7;
            c += '0';

            buf[hex_size - (i+1)] = c;
        }

        //  Null terminate
        buf[hex_size] = 0;
    }

    template <class T> void to_int(T num, char* buf)
    {
        uint8_t int_size = math::ilog((UINT64)10, (UINT64)num);

        for ( uint8_t i = 0; i < int_size; i++)
        {
            auto c = (uint8_t)((UINT64)num % 10);

            buf[int_size - (i+1)] = c + '0';

            num = (T)((UINT64)num / 10);
        }
    }
}

#endif