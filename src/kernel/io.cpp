//
// Created by Beta on 2020-05-30.
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

    dbgout::dbgout()
    {

    }

    dbgout &dbgout::operator<<(char *str)
    {
        log::write(str);
        return *this;
    }

    kout &kout::operator<<(char *str)
    {

        return *this;
    }
}
