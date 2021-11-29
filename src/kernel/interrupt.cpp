#include "interrupt.h"

extern "C" UINT64 IDT64_PTR;

/*
 * TODO: Add comments, fix spaghetti
 */
namespace interrupt
{
    using namespace log;
    /***************************************
     *            Exceptions
     *  TODO: Setup proper exception handling
     ***************************************/
    extern "C" KE_TASK_DESCRIPTOR ERR_INFO;
    KE_TASK_DESCRIPTOR ERR_INFO;

    void GenericExceptionHandler(UINT64 ErrorCode, UINT64 RIP, UINT64 CS, UINT64 RFLAGS, UINT64 RSP)
    {
        /* Print error message to serial */
        char output[420];
        SerialOut("-------EXCEPTION %16x -------\nRIP: 0x%16x\nCS: 0x%16x\nRFLAGS: 0x%16x\nRSP: 0x%16x\n", ErrorCode, RIP, CS, RFLAGS, RSP);
        SerialOut("Scheduler: %u\n", APICID());

        /* Perform a stack trace */

        SerialOut("Scheduler stats:\n");
        for (UINT64 i = 0; i < keSysDescriptor->nCores; i++)
        {
            auto scheduler = keSchedulers[i];
            SerialOut("Scheduler %u:\n", i);
            SerialOut("Current task(0x%16x) handle: 0x%08x\n", scheduler->currentTask, scheduler->currentTask->handle);
            SerialOut("Current task pTaskInfo: %16x\n", scheduler->currentTask->pTaskInfo);
            SerialOut("Current task RIP: 0x%16x\n", scheduler->currentTask->pTaskInfo->IRETQRIP);
            SerialOut("Current task RSP: 0x%16x\n", scheduler->currentTask->pTaskInfo->rsp);
        }

        hlt();
    }

    void ISR0DivideZero()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR1Debug()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR2NMI()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR3Breakpoint()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR4Overflow()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR5BoundRange()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR6InvalidOpcode()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR7DeviceUnavailable()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR8DoubleFault()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR9CoprocessorOverrun()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR10InvalidTSS()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR11SegmentUnpresent()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR12Stack()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR13GeneralProtectionFault()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
        hlt();
    }

    void ISR14PageFault()
    {
        SerialOut("%s!\n", __FUNCTION__ + 3);
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
                &ISR0DivideZero,&ISR1Debug,&ISR2NMI,&ISR3Breakpoint,
                &ISR4Overflow,&ISR5BoundRange,&ISR6InvalidOpcode,&ISR7DeviceUnavailable,
                &ISR8DoubleFault,&ISR9CoprocessorOverrun,&ISR10InvalidTSS,&ISR11SegmentUnpresent,
                &ISR12Stack,&ISR13GeneralProtectionFault,&ISR14PageFault,&GenericException,

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