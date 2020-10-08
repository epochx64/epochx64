#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <defs/int.h>
#include <asm/asm.h>
#include <kernel/log.h>
#include <lib/mem.h>
#include <kernel/typedef.h>
#include <kernel/scheduler.h>
#include <kernel/acpi.h>

namespace interrupt
{
    extern "C" void GenericException();

    extern "C" void ISR32();
    extern "C" void ISR33();
    extern "C" void ISR255();
    extern "C" void ISR48();
    extern "C" void ISR49();
    extern "C" void ISR44();

    extern "C" void int49();

    extern "C" void GenericExceptionHandler(UINT64 ErrorCode, UINT64 RIP, UINT64 CS, UINT64 RFLAGS);

    extern "C" void ISR32TimerHandler();
    extern "C" void ISR33KeyboardHandler();
    extern "C" void ISR255SpuriousHandler();
    extern "C" void ISR44MouseHandler();
    extern "C" void ISR48APICTimerHandler(UINT64 RIP, UINT64 CS, UINT64 RFLAGS, UINT64 RSP);
    extern "C" void ISR49Handler();

    extern "C" void FillIDT64();

    void SetIDTGate(UINT64 nGate, void (ISRHandler)());
}

namespace PIC
{
    void InitPS2();
    void InitPIT();
}

#endif
