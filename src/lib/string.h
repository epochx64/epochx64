#ifndef _STRING_H
#define _STRING_H

#include <defs/int.h>
#include <lib/math/math.h>

namespace string
{
    inline uint64_t strlen(unsigned char *str, char terminator = 0, UINT64 MaxLength = -1)
    {
        uint64_t return_value = 0;

        /*
         * Always check the null char
         */
        while ( str[return_value] != terminator && str[return_value] != 0 && return_value <= MaxLength )
            return_value++;

        return return_value;
    }

    inline bool strncmp(unsigned char *str1, unsigned char *str2, UINT64 count)
    {
        for(UINT64 i = 0; i < count; i++)
        {
            if(str1[i] != str2[i]) return false;
        }

        return true;
    }

    inline void strncpy(unsigned char *src, unsigned char *dst, UINT64 count)
    {
        for(UINT64 i = 0; i < count; i++) dst[i] = src[i];
    }

    /***
     * Takes in a uint{8,16,32,64}_t and outputs
     * a char string
     * @tparam T
     * @param val
     * @return
     */
     //These funcs aren't used anymore
    template<class T> inline char *to_hex(T val)
    {
        uint8_t hex_size    = sizeof(val) * 2;
        auto hex_out        = (char*)33;//new char[hex_size + 1];

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

    template<class T> inline char *to_int(T val)
    {
        uint8_t int_size = math::ilog((T)10, val);
        auto int_out = (char*)33;//new char[int_size + 1];

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
