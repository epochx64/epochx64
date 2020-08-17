//
// Created by Beta on 2020-05-30.
//TODO: This and other files should be moved around & merged
//

#include "io.h"

namespace io
{
    uint64_t strlen(char *str)
    {
        uint64_t return_value = 0;
        while ( str[return_value++] );

        return return_value - 1;
    }
}
