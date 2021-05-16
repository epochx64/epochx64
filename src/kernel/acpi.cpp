#include "acpi.h"

namespace interrupt
{
    extern double nsPerTick;
}

bool x2APIC;
UINT64 nCores;

/*
 * TODO:    This whole file is a bunch of spaghetti
 */

/*
 * Temporary
 */
void gfxroutine(KE_TASK_ARG *TaskArgs)
{
    double x = *((double*)TaskArgs[0]);
    double y = *((double*)TaskArgs[1]);
    int radius = *((int*)TaskArgs[2]);

    graphics::Color c(128, 192, 128, 0);

    while (true)
    {
        using namespace graphics;
        using namespace math;

        double divisions = 120.0;
        double dtheta = 2 * PI / divisions;

        //  Draw a fat circle
        for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
            for (int j = 0; j < radius; j++)
                PutPixel(RoundDouble<UINT64>(x + j*cos(theta)),RoundDouble<UINT64>(y + j*sin(theta)),
                         &(keSysDescriptor->gopInfo), c);
        }

        c.u8_R += 10;
        c.u8_G += 20;
        c.u8_B += 10;
    }
}

void KeInitACPI()
{
    auto RSDPDescriptor = (RSDP_DESCRIPTOR*)(keSysDescriptor->pRSDP);
    auto pXSDT = RSDPDescriptor->XSDTAddress;

    keSysDescriptor->pXSDT = (UINT64)pXSDT;

    KeInitAPIC();
}

void KeInitAPIC()
{
    using namespace log;
    /*
     * APIC setup for the Bootstrap Processor (BSP)
     */
    {
        /*
         * Disable the 8259 PIC by masking all IRQs except for IRQ0
         * we will need to use a PIC sleep function to determine LAPIC's CPU bus frequency
         */

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
            keSysDescriptor->apicBase = MSRValue & 0x0000000FFFFFF000;

            kout << "xAPIC\n";
        }

        //  Set the spurious interrupt vector register to vector 39
        //  Set APIC enable bit
        auto RegValue = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0xF0) | 0x100 | 0xFF;
        SetLAPICRegister(keSysDescriptor->apicBase, 0xF0, RegValue);
    }

    /*
     * APIC timer setup for the BSP
     */
    {
        //  Setup Divide Configuration Register to divide by 1
        auto APICDivideReg = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0x3E0);
        APICDivideReg |= 0b00001011;
        SetLAPICRegister(keSysDescriptor->apicBase, 0x3E0, APICDivideReg);

        //  Vector: 48, unmask, one-shot mode
        auto APICTimerReg = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0x320) & (~0x000000FF);
        APICTimerReg = (APICTimerReg | 0x00020000 | 48) & (~0x00050000);
        SetLAPICRegister(keSysDescriptor->apicBase, 0x320, APICTimerReg);

        //  Set Initial Count Register
        keSysDescriptor->apicInitCount = 0x0010000;
        SetLAPICRegister(keSysDescriptor->apicBase, 0x380, keSysDescriptor->apicInitCount);
    }

    /*
     * HPET Setup
     */
    {
        auto pHPET = (HPET_DESCRIPTOR_TABLE*) FindSDT(
                (EXTENDED_SYSTEM_DESCRIPTOR_TABLE *)keSysDescriptor->pXSDT,
                "HPET"
        );

        UINT64 hpetAddress = pHPET->address;
        keSysDescriptor->pHPET = hpetAddress;

        UINT64 hpetPeriod  = *(UINT64*)(hpetAddress) >> 32;
        keSysDescriptor->hpetPeriod = hpetPeriod;
        kout << "[ HPET ] period in femptoseconds: " << DEC << hpetPeriod << "\n";

        //  Enable HPET
        *(UINT64*)(hpetAddress + 0x10) |= 1;

        UINT64 configHPET = *(UINT64*)(hpetAddress + 0x100);
        if(configHPET & (1<<4))
        {
            kout << "[ HPET ] Periodic mode supported\n";

            if(configHPET & (1<<1))
                kout << "[ HPET ] Level triggered\n";

            //  Enable periodic mode
            configHPET |= (1<<3) | (1<<6) & ~(1<<2);

            //  Enable interrupts
            //configHPET |= (1<<2);
        }

        //  Write the config
        *(UINT64*)(hpetAddress + 0x100) = configHPET;

        //  Set comparator register
        //*(UINT64*)(hpetAddress + 0x108) = 0x2ffff;

        //interrupt::nsPerTick = 0x2ffff*(double)hpetPeriod/1e6;
    }

    /*
     * Multicore bootstrap
     */
    {
        auto pMADT =(MULTIPLE_APIC_DESCRIPTOR_TABLE*) FindSDT(
                (EXTENDED_SYSTEM_DESCRIPTOR_TABLE *)keSysDescriptor->pXSDT,
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
        keSysDescriptor->nCores = nCores;

        /*
         * Setup a scheduler before the APIC timer is enabled
         * and add a couple of tasks
         */
        keSchedulers = (Scheduler**)(new UINT64[nCores]);
        keTasks = (KE_TASK_DESCRIPTOR**)(new UINT64[nCores]);

        keSysDescriptor->pkeSchedulers = (UINT64)keSchedulers;
        keSysDescriptor->pTaskInfos = (UINT64)keTasks;

        for(UINT64 i = 0; i < nCores; i++)
        {
            auto t = new Task(
                    (UINT64)&gfxroutine,
                    0,
                    false,
                    0,
                    new KE_TASK_ARG[3] {
                            (KE_TASK_ARG)(new double(550.0 + 42.0*i)),
                            (KE_TASK_ARG)(new double(40.0)),
                            (KE_TASK_ARG)(new int(16))
                    });

            keSchedulers[i] = (Scheduler*)(new Scheduler(i));
            keSchedulers[i]->AddTask(t);
        }

        pAPBootstrapInfo = new KE_AP_BOOTSTRAP_INFO[nCores];
        for(UINT8 APIC_ID = 0x00; APIC_ID < nCores; APIC_ID++)
            pAPBootstrapInfo[APIC_ID].pStack = (UINT64)KeSysMalloc(8192) + 8191;
        /*
         * The Application Processors (APs) are going to need these for when they
         * bootstrap from 16-bit real mode to 64-bit long mode
         */
        UINT64 cr3Value;
        GetCR3Value(&cr3Value);
        CR3Value = cr3Value;

        UINT64 Vector = (UINT64)&APBootstrap >> 12;

        /*
         * Broadcast INIT Interprocessor Interrupt (IPI) to all APs
         */
        SetLAPICRegister<UINT32>(keSysDescriptor->apicBase, APIC_REGISTER_ICR, 0x000C4500);

        //  TODO: If this don't work we gotta implement a sleep function and wait for 10ms here

        /*
         * Broadcast SIPI IPI to all APs where a vector 0x10 = address 0x10000, vector to bootstrap code
         * We have to do it twice for some reason
         */
        SetLAPICRegister<UINT32>(keSysDescriptor->apicBase, APIC_REGISTER_ICR, 0x000C4600 | Vector);

        //  TODO: If it still don't work we gotta implement 200us sleep here
        SetLAPICRegister<UINT32>(keSysDescriptor->apicBase, APIC_REGISTER_ICR, 0x000C4600 | Vector);
    }
}

void CalibrateAPIC(UINT64 frequency)
{
    using namespace log;
    while(true)
    {
        TTY_COORD oldCOORD = kout.GetPosition();

        TTY_COORD timerCOORD;
        timerCOORD.Lin = 5;
        timerCOORD.Col = 70;
        kout.SetPosition(timerCOORD);

        UINT64 hpetCounter = *(UINT64*)(keSysDescriptor->pHPET + 0xF0);
        log::kout << "[ HPET ] seconds: "<<DEC<<(UINT64)((double)hpetCounter*keSysDescriptor->hpetPeriod/1e15) << "\n";
        kout.SetPosition(oldCOORD);

        asm volatile ("pause");
    }


}

/*
 * C entry point of the AP bootstrap routine
 */
extern "C" void C_APBootstrap();
void C_APBootstrap()
{
    EnableSSE(keSchedulers[APICID()]->currentTask->pTaskInfo);

    /*
     * Setup APIC
     */
    {
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
        auto RegValue = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0xF0) | 0x100 | 0xFF;
        SetLAPICRegister(keSysDescriptor->apicBase, 0xF0, RegValue);
    }

    /*
     *  Setup APIC timer
     */
    {
        //  Setup Divide Configuration Register to divide by 1
        auto APICDivideReg = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0x3E0);
        APICDivideReg |= 0b00001011;
        SetLAPICRegister(keSysDescriptor->apicBase, 0x3E0, APICDivideReg);

        //  Vector: 48, unmask, one-shot mode
        auto APICTimerReg = GetLAPICRegister<UINT32>(keSysDescriptor->apicBase, 0x320) & (~0x000000FF);
        APICTimerReg = (APICTimerReg | 0x00020000 | 48) & (~0x00050000);
        SetLAPICRegister(keSysDescriptor->apicBase, 0x320, APICTimerReg);

        //  Set Initial Count Register
        SetLAPICRegister(keSysDescriptor->apicBase, 0x380, keSysDescriptor->apicInitCount);
    }

    sti();

    /*
     * This is now an idle task
     */
    hlt();
}

UINT64 FindSDT(EXTENDED_SYSTEM_DESCRIPTOR_TABLE *XSDT, char *ID)
{
    UINT16 nEntries = (XSDT->SDTHeader.Length - sizeof(XSDT->SDTHeader)) / 8;

    for(UINT16 i = 0; i < nEntries; i++)
    {
        auto Header = (SDT_HEADER *)(XSDT->pSDTArray[i]);
        if(string::strncmp((UINT8*)Header->Signature, (UINT8*)ID, 4)) return (UINT64)Header;
    }

    return 0;
}

