#ifndef ASM_H
#define ASM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace x86_64_asm
{
    #define COM1 0x3f8

    typedef uint8_t     BYTE;
    typedef uint16_t    WORD;
    typedef uint32_t    DWORD;
    typedef uint64_t    QWORD;

    extern "C" void asm_outb(WORD port, BYTE val);
    extern "C" void asm_outw(WORD port, WORD val);
    extern "C" void asm_outd(WORD port, DWORD val);
    extern "C" void asm_outq(WORD port, QWORD val);
}


#endif