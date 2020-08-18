#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "heap.h"
#include "../include/math.h"

namespace io
{
    inline uint64_t strlen(char *str)
    {
        uint64_t return_value = 0;
        while ( str[return_value++] );

        return return_value - 1;
    }

    /***
     * Takes in a uint{8,16,32,64}_t and outputs
     * a char string
     * @tparam T
     * @param val
     * @return
     */
    template<class T>
    inline char *to_hex(T val)
    {
        uint8_t hex_size    = sizeof(val) * 2;
        auto hex_out        = new char[hex_size + 1];

        for ( uint8_t i = 0; i < hex_size; i++ )
        {
            uint8_t c = ((uint8_t)(val >> (4*i))) & 0x0F;

            //  ASCII conversion; capital
            if ( c > 9 ) c += 7;
            c += '0';

            hex_out[hex_size - (i+1)] = c;
        }

        //  Null terminate
        hex_out[hex_size] = 0;

        return hex_out;
    }

    template<class T>
    inline char *to_int(T val)
    {
        uint8_t int_size = math::ilog((T)10, val);
        auto int_out = new char[int_size + 1];

        for ( uint8_t i = 0; i < int_size; i++)
        {
            auto c = (uint8_t)(val % 10);

            int_out[int_size - (i+1)] = c + '0';

            val /= 10;
        }

        int_out[int_size] = 0;

        return int_out;
    }
}

#endif
