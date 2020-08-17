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

namespace log
{
    /***
     * Writes the string to COM1 port
     * @param str
     * @return Number of bytes written
     */
    uint64_t write(char *str);
}

#endif
