//
// Created by Beta on 2020-07-18.
//

#include "log.h"

extern graphics::frame_buf_attr_t _Frame_Buffer_Info;

namespace log
{
    uint64_t write(char *str)
    {
        uint64_t len = io::strlen(str);

        using namespace x86_64_asm;
        for ( uint64_t i = 0; i < len; i++ ) asm_outb(COM1, str[i]);

        return len;
    }

    //TODO: Write a function that automatically does newline and prints time, importance level, yaknow, a proper logger
}
