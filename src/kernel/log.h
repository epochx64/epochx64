//
// Created by Dennis on 2020-07-18.
//

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "asm.h"
#include "io.h"
#include "graphics.h"
#include "font_rom.h"

extern graphics::frame_buf_attr_t _Frame_Buffer_Info;

namespace log
{
    /***
     * Writes the string to COM1 port
     * @param str
     * @return Number of bytes written
     */
    inline uint64_t write(char *str)
    {
        uint64_t len = io::strlen(str);

        using namespace x86_64_asm;
        for ( uint64_t i = 0; i < len; i++ ) outb(COM1, str[i]);

        return len;
    }

    //TODO: Write a function that automatically does newline and prints time, importance level, yaknow, a proper logger
}

#endif
