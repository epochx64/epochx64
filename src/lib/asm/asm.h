#ifndef _ASM_H
#define _ASM_H

#include <defs/int.h>

namespace ASMx64
{
    #define COM1 0x3F8

    typedef UINT8     BYTE;
    typedef UINT16    WORD;
    typedef UINT32    DWORD;
    typedef UINT64    QWORD;

    extern "C" BYTE inb(WORD port);
    extern "C" void outb(WORD port, BYTE val);

    extern "C" void sti();
    extern "C" void cli();
    extern "C" void hlt();

    extern "C" void lgdt();
    extern "C" void lidt();

    extern "C" void cpuid(UINT64 *rax, UINT64 *rbx, UINT64 *rcx, UINT64 *rdx);
    extern "C" void ReadMSR(UINT64 MSRID, UINT64 *MSRValue);
    extern "C" void SetMSR(UINT64 MSRID, UINT64 MSRValue);
    extern "C" void IOWait();

    extern "C" void ReadRFLAGS(UINT64 *RFLAGS);
    extern "C" void EnableSSE(void *pSSEINFO);

    extern "C" void APBootstrap();

    extern "C" void GetCR3Value(UINT64 *Value);
    extern "C" UINT32 CR3Value;

    extern "C" UINT64 APICID();
}


#endif