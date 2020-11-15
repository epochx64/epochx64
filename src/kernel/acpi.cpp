#include "acpi.h"

namespace ACPI
{
    using namespace log;
    bool x2APIC;
    UINT64 nCores;

    /*
     * TODO:    This whole file is a bunch of spaghetti
     */

    /*
     * Temporary
     */
    void gfxroutine(scheduler::TASK_ARG *TaskArgs)
    {
        double x = *((double*)TaskArgs[0]);
        double y = *((double*)TaskArgs[1]);
        int radius = *((int*)TaskArgs[2]);

        graphics::Color c(128, 192, 128, 0);

        while (true)
        {
            using namespace graphics;
            using namespace kernel;
            using namespace math;

            double divisions = 120.0;
            double dtheta = 2 * PI / divisions;

            //  Draw a fat circle
            for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
                for (int j = 0; j < radius; j++)
                    PutPixel(RoundDouble<UINT64>(x + j*cos(theta)),RoundDouble<UINT64>(y + j*sin(theta)),
                                            &(KernelDescriptor->GOPInfo), c);
            }

            c.u8_R += 10;
            c.u8_G += 20;
            c.u8_B += 10;
        }
    }

    void InitACPI()
    {
        using namespace kernel;

        auto RSDPDescriptor = (RSDP_DESCRIPTOR*)(KernelDescriptor->pRSDP);
        auto pXSDT = RSDPDescriptor->XSDTAddress;

        KernelDescriptor->pXSDT = (UINT64)pXSDT;

        InitAPIC();
    }

    void InitAPIC()
    {
        using namespace kernel;
        /*
         * APIC setup for the Bootstrap Processor (BSP)
         */
        {
            using namespace ASMx64;
            /*
             * Disable the 8259 PIC by masking all IRQs except for IRQ0
             * we will need to use a PIC sleep function to determine LAPIC's CPU bus frequency
             */
            outb(0xA1, 0b11101111);
            outb(0x21, 0b11111100);

            //  Grab the APIC BASE MSR value
            UINT64 MSRValue = 0;
            ReadMSR(0x01B, &MSRValue);

            //  Check APIC version
            UINT64 rax = 1, rbx = 0, rcx = 0, rdx = 0;
            cpuid(&rax, &rbx, &rcx, &rdx);

            if(rcx & X2APIC_BIT)
            {
                //  Set x2APIC mode in APIC base MSR
                x2APIC = true;
                MSRValue |= (1<<10);
                SetMSR(0x01B, MSRValue);

                kout << "x2APIC\n";
            }
            else
            {
                KernelDescriptor->APICBase = MSRValue & 0x0000000FFFFFF000;

                kout << "xAPIC\n";
            }

            //  Set the spurious interrupt vector register to vector 39
            //  Set APIC enable bit
            auto RegValue = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0xF0) | 0x100 | 0xFF;
            SetLAPICRegister(KernelDescriptor->APICBase, 0xF0, RegValue);
        }

        /*
         * APIC timer setup for the BSP
         */
        {
            //  Setup Divide Configuration Register to divide by 1
            auto APICDivideReg = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0x3E0);
                 APICDivideReg |= 0b00001011;
            SetLAPICRegister(KernelDescriptor->APICBase, 0x3E0, APICDivideReg);

            //  Vector: 48, unmask, one-shot mode
            auto APICTimerReg = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0x320) & (~0x000000FF);
                 APICTimerReg = (APICTimerReg | 0x00020000 | 48) & (~0x00050000);
            SetLAPICRegister(KernelDescriptor->APICBase, 0x320, APICTimerReg);

            //  Set Initial Count Register
            KernelDescriptor->APICInitCount = 0x0010000;
            SetLAPICRegister(KernelDescriptor->APICBase, 0x380, KernelDescriptor->APICInitCount);
        }

        /*
         * Multicore bootstrap
         */
        {
            auto pMADT =(MULTIPLE_APIC_DESCRIPTOR_TABLE*) FindSDT(
                    (EXTENDED_SYSTEM_DESCRIPTOR_TABLE *)KernelDescriptor->pXSDT,
                    "APIC"
                    );

            /*
             * Parse the MADT to find all APICs
             */
            nCores = 0;
            auto Iterator = (UINT8*)((UINT64)pMADT + 0x2C);

            while(true)
            {
                auto Type  = *Iterator;
                auto Size  = *(Iterator + 1);
                auto ID    = *(Iterator + 3);
                auto Flags = *(Iterator + 4);

                if( (Type == MADT_LAPIC) && (Flags & 0b00000001) ) nCores++;

                if(Size == 0) break;
                Iterator += Size;
            }
            kout << DEC << "Found " << nCores << " logical processors\n";

            /*
             * Setup a scheduler before the APIC timer is enabled
             * and add a couple of tasks
             */
            using namespace scheduler;

            Schedulers = (Scheduler**)(new UINT64[nCores]);
            TASK_INFOS = (TASK_INFO**)(new UINT64[nCores]);

            for(UINT64 i = 0; i < nCores; i++)
            {
                Schedulers[i] = (Scheduler*)(new Scheduler(false, i));
                Schedulers[i]->AddTask(new Task(
                        (UINT64)&gfxroutine,
                        true,
                        new TASK_ARG[3] {
                                (TASK_ARG)(new double(550.0 + 42.0*i)),
                                (TASK_ARG)(new double(40.0)),
                                (TASK_ARG)(new int(16))
                        }
                ));
            }

            pAPBootstrapInfo = new AP_BOOTSTRAP_INFO[nCores];
            for(UINT8 APIC_ID = 0x00; APIC_ID < nCores; APIC_ID++)
                pAPBootstrapInfo[APIC_ID].pStack = (UINT64)heap::MallocAligned(8192, 16) + 8192;

            /*
             * The Application Processors (APs) are going to need these for when they
             * bootstrap from 16-bit real mode to 64-bit long mode
             */
            UINT64 CR3Value;
            ASMx64::GetCR3Value(&CR3Value);
            ASMx64::CR3Value = CR3Value;

            UINT64 Vector = (UINT64)&ASMx64::APBootstrap >> 12;

            /*
             * Broadcast INIT Interprocessor Interrupt (IPI) to all APs
             */
            SetLAPICRegister<UINT32>(KernelDescriptor->APICBase, APIC_REGISTER_ICR, 0x000C4500);

            //  TODO: If this don't work we gotta implement a sleep function and wait for 10ms here

            /*
             * Broadcast SIPI IPI to all APs where a vector 0x10 = address 0x100000, vector to bootstrap code
             * We have to do it twice for some reason
             */
            SetLAPICRegister<UINT32>(KernelDescriptor->APICBase, APIC_REGISTER_ICR, 0x000C4600 | Vector);

            //  TODO: If it still don't work we gotta implement 200us sleep here
            SetLAPICRegister<UINT32>(KernelDescriptor->APICBase, APIC_REGISTER_ICR, 0x000C4600 | Vector);
        }
    }

    /*
     * C entry point of the AP bootstrap routine
     */
    extern "C" void C_APBootstrap();
    void C_APBootstrap()
    {
        ASMx64::EnableSSE(scheduler::Schedulers[ASMx64::APICID()]->CurrentTask->pTaskInfo);

        using namespace kernel;

        /*
         * Setup APIC
         */
        {
            using namespace ASMx64;

            //  Grab the APIC BASE MSR value
            UINT64 MSRValue = 0;
            ReadMSR(0x01B, &MSRValue);

            //  Check APIC version
            UINT64 rax = 1, rbx = 0, rcx = 0, rdx = 0;
            cpuid(&rax, &rbx, &rcx, &rdx);

            if(rcx & X2APIC_BIT)
            {
                //  Set x2APIC mode in APIC base MSR
                MSRValue |= (1<<10);
                SetMSR(0x01B, MSRValue);
            }

            //  Set the spurious interrupt vector register to vector 39
            //  Set APIC enable bit
            auto RegValue = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0xF0) | 0x100 | 0xFF;
            SetLAPICRegister(KernelDescriptor->APICBase, 0xF0, RegValue);
        }

        /*
         *  Setup APIC timer
         */
        {
            //  Setup Divide Configuration Register to divide by 1
            auto APICDivideReg = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0x3E0);
            APICDivideReg |= 0b00001011;
            SetLAPICRegister(KernelDescriptor->APICBase, 0x3E0, APICDivideReg);

            //  Vector: 48, unmask, one-shot mode
            auto APICTimerReg = GetLAPICRegister<UINT32>(KernelDescriptor->APICBase, 0x320) & (~0x000000FF);
            APICTimerReg = (APICTimerReg | 0x00020000 | 48) & (~0x00050000);
            SetLAPICRegister(KernelDescriptor->APICBase, 0x320, APICTimerReg);

            //  Set Initial Count Register
            SetLAPICRegister(KernelDescriptor->APICBase, 0x380, KernelDescriptor->APICInitCount);
        }

        ASMx64::sti();

        /*
         * This is now an idle task
         */
        ASMx64::hlt();
    }

    UINT64 FindSDT(EXTENDED_SYSTEM_DESCRIPTOR_TABLE *XSDT, char *ID)
    {
        UINT16 nEntries = (XSDT->SDTHeader.Length - sizeof(XSDT->SDTHeader)) / 8;

        for(UINT16 i = 0; i < nEntries; i++)
        {
            auto Header = (SDT_HEADER *)(XSDT->pSDTArray[i]);
            if(string::strncmp((char*)Header->Signature, ID, 4)) return (UINT64)Header;
        }

        return 0;
    }
}
