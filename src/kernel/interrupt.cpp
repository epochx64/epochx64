#include "interrupt.h"

extern "C" UINT64 IDT64_PTR;

//  TODO:   Add comments, fix spaghetti
namespace interrupt
{
    using namespace log;
    using namespace scheduler;
    /*--------------------------------------
     *            Exceptions
     ---------------------------------------*/
    extern "C" TASK_INFO ERR_INFO;
    TASK_INFO ERR_INFO;

    void GenericExceptionHandler(UINT64 ErrorCode, UINT64 RIP, UINT64 CS, UINT64 RFLAGS)
    {
        kout << "*** Exception 0x" << HEX << ErrorCode << " generated, halting ***\n"
        << "External: " << (UINT8)(ErrorCode & 1) << "\n"
        << "GDT 0, IDT 1, LDT 2, IDT, 3: " << (UINT8)(ErrorCode & 0b110) << "\n"
        << "Selector Index: 0x" << (UINT16)((ErrorCode >> 3) & 0x1FFF) << "\n"
        << "CS:EIP: 0x" << (UINT8)CS << ":" << RIP << "\n"
        << "EFLAGS: 0x" << RFLAGS << "\n";

        while(true);
    }

    /*--------------------------------------
    *          Other ISRs
    ---------------------------------------*/

    void ISR49Handler()
    {
        kout << "IRQ49: Test interrupt\n";

        ASMx64::outb(0xA0, 0x20);
        ASMx64::outb(0x20, 0x20);   //  End of interrupt
    }

    //  One tick is one call to the timer ISR
    double msPerTick;
    double ms = 0.0;

    //  Old tick from PIT
    void ISR32TimerHandler()
    {
        log::TTY_COORD OldCOORD = kout.GetPosition();

        log::TTY_COORD TimerCOORD;
        TimerCOORD.Lin = 0;
        TimerCOORD.Col = 70;
        kout.SetPosition(TimerCOORD);

        kout << DEC << (UINT64)ms << " ms\n";
        kout.SetPosition(OldCOORD);

        ms = ms + msPerTick;

        ASMx64::outb(0x20, 0x20);   //  End of interrupt
    }

    void ISR48APICTimerHandler(UINT64 RIP, UINT64 CS, UINT64 RFLAGS, UINT64 RSP)
    {
        Schedulers[ASMx64::APICID()]->Tick();

        //  Signal End of Interrupt
        ACPI::SetLAPICRegister<UINT32>(kernel::KernelDescriptor->APICBase, 0x0B0, 0x00);
    }

    void ISR33KeyboardHandler()
    {
        using namespace ASMx64;

        while(!(inb(0x64) & 1));
        UINT8 Scancode = inb(0x60);

        kout << HEX << Scancode << " ";

        outb(0x20, 0x20);   //  End of interrupt
    }

    void ISR255SpuriousHandler()
    {
        kout << "Spurious ISR";
        while(true);
    }

    void ISR44MouseHandler()
    {
        kout << "Mouse ISR";
        while(true);
        //FYI this is PIC #2 handled so you need 2 outb to signal EOI
    }

    void SetIDTGate(UINT64 nGate, void (*ISRHandler)())
    {
        IDT_DESCRIPTOR idtDescriptor;

        //  These never change
        idtDescriptor.css           = 0x08;
        idtDescriptor.IST_offset    = 0;
        idtDescriptor.attributes    = 0b10001110;
        idtDescriptor.reserved      = 0;

        idtDescriptor.offset_lower  = (uint16_t)((uint64_t)(*(ISRHandler)));
        idtDescriptor.offset_higher = (uint64_t)((uint64_t)(*(ISRHandler)) >> 0x10);

        mem::copy (
                (byte*)&idtDescriptor,
                (byte*)((uint64_t)&IDT64_PTR + nGate*sizeof(IDT_DESCRIPTOR)),
                sizeof(IDT_DESCRIPTOR)
        );
    }

    void FillIDT64()
    {
        ASMx64::cli();

        //  UEFI makes its own GDT that we don't want, so this contains
        //  More than just a lgdt instruction
        ASMx64::lgdt();

        //  Might make individual exception ISRs later
        void (*ISR_list[])() = {
                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException,

                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException,
                &GenericException,&GenericException,&GenericException,&GenericException
        };

        //  Reserved IRQs
        for(uint16_t i = 0; i < 32; i++) SetIDTGate(i, ISR_list[i]);

        SetIDTGate(32, &ISR32);     //  8259 PIC Timer IRQ0
        SetIDTGate(33, &ISR33);     //  Keyboard IRQ1
        SetIDTGate(255, &ISR255);   //  APIC Spurious
        SetIDTGate(44, &ISR44);     //  Mouse handler
        SetIDTGate(48, &ISR48);     //  APIC Timer
        SetIDTGate(49, &ISR49);     //  Test Interrupt

        ASMx64::lidt();
    }
}