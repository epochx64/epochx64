#include "acpi.h"

namespace ACPI
{
    using namespace log;
    bool x2APIC;

    /*
     * TODO:    This whole file is a bunch of spaghetti
     */

    void InitACPI(KERNEL_BOOT_INFO *KernelInfo, KERNEL_ACPI_INFO *KernelACPIInfo)
    {
        KernelACPIInfo->RSDPDescriptor = (RSDP_DESCRIPTOR*)(KernelInfo->RSDP);

        auto pXSDT = (EXTENDED_SYSTEM_DESCRIPTOR_TABLE*)(KernelACPIInfo->RSDPDescriptor->XSDTAddress);
        KernelACPIInfo->XSDT = pXSDT;
        if(pXSDT != nullptr) kout << "XSDT Address: 0x" << HEX << (UINT64)pXSDT << "\n";

        InitAPIC(KernelACPIInfo);
    }

    void InitAPIC(KERNEL_ACPI_INFO *KernelACPIInfo)
    {
        using namespace ASMx64;

        /*
         * Disable the 8259 PIC by masking all IRQs except for IRQ0
         * we will need to use a PIC sleep function to determine LAPIC's CPU bus frequency
         */
        outb(0xA1, 0xFF);
        outb(0x21, 0b11111110);

        /*
         * APIC setup for the Boostrap Processor (BSP)
         */
        {
            //  Grab the APIC BASE MSR value
            UINT64 MSRValue = 0;
            ReadMSR(0x01B, &MSRValue);

            //  Check APIC version
            UINT64 rax = 1, rbx = 0, rcx = 0, rdx = 0;
            cpuid(&rax, &rbx, &rcx, &rdx);
            kout << "APIC Version: ";

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
                KernelACPIInfo->APICBase = MSRValue & 0x0000000FFFFFF000;

                kout << "xAPIC\n";
            }

            //  Set the spurious interrupt vector register to vector 39
            //  Set APIC enable bit
            auto RegValue = GetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, 0xF0) | 0x100 | 0xFF;
            SetLAPICRegister(KernelACPIInfo->APICBase, 0xF0, RegValue);
        }

        /*
         * APIC timer setup for the BSP
         */
        {
            //  Setup Divide Configuration Register to divide by 1
            auto APICDivideReg = GetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, 0x3E0);
                 APICDivideReg |= 0b00001011;
            SetLAPICRegister(KernelACPIInfo->APICBase, 0x3E0, APICDivideReg);

            //  Vector: 48, unmask, one-shot mode
            auto APICTimerReg = GetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, 0x320) & (~0x000000FF);
                 APICTimerReg = (APICTimerReg | 0x00020000 | 48) & (~0x00050000);
            SetLAPICRegister(KernelACPIInfo->APICBase, 0x320, APICTimerReg);

            kout << "APICTimerReg 0x" << HEX << APICTimerReg << "\n";

            //  Set Initial Count Register
            KernelACPIInfo->APICInitCount = 0x0010000;
            SetLAPICRegister(KernelACPIInfo->APICBase, 0x380, KernelACPIInfo->APICInitCount);
        }

        /*
         * Multicore bootstrap
         */
        {
            auto pMADT = (MULTIPLE_APIC_DESCRIPTOR_TABLE*) FindSDT(KernelACPIInfo->XSDT, "APIC");
            kout << "MADT Address: 0x" << HEX << (UINT64)pMADT << "\n";

            /*
             * Parse the MADT to find all APICs
             */
            UINT8 nCores = 0;
            UINT8 nEnabled = 0;
            auto Iterator = (UINT8*)((UINT64)pMADT + 0x2C);

            while(true)
            {
                auto Type  = *Iterator;
                auto Size  = *(Iterator + 1);
                auto Flags = *(Iterator + 4);

                if( (Type == MADT_LAPIC) && (Flags & 0b00000011) ) nCores++;
                if( (Type == MADT_LAPIC) && (Flags & 0b00000001) ) nEnabled++;

                if(Size == 0) break;
                Iterator += Size;
            }
            kout << DEC << "Found " << nCores << " cores, " << nEnabled << " are enabled\n";

            /*
             * Broadcast INIT Interprocessor Interrupt (IPI) to all Application Processors (APs)
             */
            SetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, APIC_REGISTER_ICR, 0x000C4500);

            //  TODO: If this don't work we gotta implement a sleep function and wait for 10ms here

            /*
             * Broadcast SIPI IPI to all APs where 10 = address 0x100000, vector to bootstrap code
             * We have to do it twice for some reason
             */
            SetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, APIC_REGISTER_ICR, 0x000C4610);

            //  TODO: If it still don't work we gotta implement 20us sleep here

            SetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, APIC_REGISTER_ICR, 0x000C4610);
        }
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
