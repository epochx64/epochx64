#include "interrupt.h"

extern "C" UINT64 IDT64_PTR;

/*
 * TODO: Add comments, fix spaghetti
 */
namespace interrupt
{
    using namespace log;
    /*--------------------------------------
     *            Exceptions
     *  TODO: Setup proper exception handling
     ---------------------------------------*/
    extern "C" KE_TASK_DESCRIPTOR ERR_INFO;
    KE_TASK_DESCRIPTOR ERR_INFO;

    void KeHalt()
    {
        /* Clear interrupts */
        asm volatile ("cli");

        while(true) asm volatile ("hlt");
    }

    void GenericExceptionHandler(UINT64 ErrorCode, UINT64 RIP, UINT64 CS, UINT64 RFLAGS)
    {
        /* Create halt task */
        //Task t((UINT64)&KeHalt,  0, false, 0, 0, va_list{});

        /* Halt all schedulers */
        //for (UINT64 i = 0; i < keSysDescriptor->nCores; i++)
        //{
        //    keSchedulers[i]->ScheduleTask(&t);
        //}

        /* Print error message */
        printf("-------EXCEPTION-------\nRIP: 0x%16x\nCS: 0x%16x\nRFLAGS: 0x%16x\n", RIP, CS, RFLAGS);

        //kout << "*** Exception 0x" << HEX << ErrorCode << " generated, halting ***\n"
        //<< "External: " << (UINT8)(ErrorCode & 1) << "\n"
        //<< "GDT 0, IDT 1, LDT 2, IDT, 3: " << (UINT8)(ErrorCode & 0b110) << "\n"
        //<< "Selector Index: 0x" << (UINT16)((ErrorCode >> 3) & 0x1FFF) << "\n"
        //<< "CS:EIP: 0x" << (UINT8)CS << ":" << RIP << "\n"
        //<< "EFLAGS: 0x" << RFLAGS << "\n";

        hlt();
    }

    /*--------------------------------------
    *          Other ISRs
    ---------------------------------------*/

    void ISR49Handler()
    {
        kout << "IRQ49: Test interrupt\n";

        outb(0xA0, 0x20);
        outb(0x20, 0x20);   //  End of interrupt
    }

    //  One tick is one call to the timer ISR
    double nsPerTick;
    UINT64 Ticks = 1;

    //  Old tick from PIT
    void ISR32TimerHandler()
    {
        TTY_COORD oldCOORD = kout.GetPosition();

        TTY_COORD timerCOORD;
        timerCOORD.Lin = 0;
        timerCOORD.Col = 70;
        kout.SetPosition(timerCOORD);

        kout << DEC << (UINT64)(nsPerTick*(double)Ticks/1e6) << " ms\n";
        kout.SetPosition(oldCOORD);

        Ticks++;

        

        outb(0x20, 0x20);   //  End of interrupt
    }

    void ISR48APICTimerHandler(UINT64 RIP, UINT64 CS, UINT64 RFLAGS, UINT64 RSP)
    {
        keSchedulers[APICID()]->Tick();

        /* Signal End of Interrupt */
        SetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0x0B0, 0x00);
    }

    void ISR33KeyboardHandler()
    {
        UINT8 Scancode = inb(0x60);

        /* The function pointer is null until dwm is launched */
        if (((DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor)->DwmKeyboardEvent != nullptr)
        {
            ((DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor)->DwmKeyboardEvent(Scancode);
        }

        outb(0x20, 0x20);   //  End of interrupt
    }

    void ISR255SpuriousHandler()
    {
        kout << "Spurious ISR";
        while(true);
    }

    void ISR44MouseHandler()
    {
        /* Wait for PS/2 mouse data */
        UINT8 Scancode = inb(0x60);

        /* The function pointer is null until dwm is launched */
        if (((DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor)->DwmMouseEvent != nullptr)
        {
            ((DWM_DESCRIPTOR*)keSysDescriptor->pDwmDescriptor)->DwmMouseEvent(Scancode, MousePacketSize);
        }

        outb(0xA0, 0x20);   //  EOI for the slave PIC
        outb(0x20, 0x20);   //  End of interrupt
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

    void KeInitIDT()
    {
        cli();

        //  UEFI makes its own GDT that we don't want, so this contains
        //  More than just a lgdt instruction
        lgdt();

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
        SetIDTGate(44, &ISR44);     //  Mouse IRQ12
        SetIDTGate(48, &ISR48);     //  APIC Timer
        SetIDTGate(49, &ISR49);     //  Test Interrupt

        lidt();
    }
}