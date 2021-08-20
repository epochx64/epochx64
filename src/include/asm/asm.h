#ifndef _ASM_H
#define _ASM_H

#include <defs/int.h>

#define COM1 0x3F8

typedef UINT8     BYTE;
typedef UINT16    WORD;
typedef UINT32    DWORD;
typedef UINT64    QWORD;

extern "C" UINT32 CR3Value;

extern "C"
{
    void nop();

    BYTE inb(WORD port);
    void outb(WORD port, BYTE val);

    void sti();
    void cli();
    void hlt();

    void lgdt();
    void lidt();

    void cpuid(UINT64 *rax, UINT64 *rbx, UINT64 *rcx, UINT64 *rdx);
    void ReadMSR(UINT64 MSRID, UINT64 *MSRValue);
    void SetMSR(UINT64 MSRID, UINT64 MSRValue);
    void IOWait();

    void ReadRFLAGS(UINT64 *RFLAGS);
    void EnableSSE(void *pSSEINFO);

    void APBootstrap();

    void GetCR3Value(UINT64 *Value);

    UINT64 APICID();

    void SetRSP(UINT64 RSP);
    UINT64 GetRSP();

    void memset128(void *dest, UINT64 size, __uint128_t value);
    void memset64(void *dest, UINT64 size, UINT64 value);
    void memset32(void *dest, UINT64 size, UINT32 value);
    void memset16(void *dest, UINT64 size, UINT16 value);
    void memset8(void *dest, UINT64 size, UINT8 value);
}



#endif