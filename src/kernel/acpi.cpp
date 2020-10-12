#include "acpi.h"

namespace ACPI
{
    using namespace log;
    bool x2APIC;

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

        auto pMADT = (MULTIPLE_APIC_DESCRIPTOR_TABLE*) FindSDT(KernelACPIInfo->XSDT, "APIC");
        kout << "MADT Address: 0x" << HEX << (UINT64)pMADT << "\n";

        //  APIC setup
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
                kout << "x2APIC\n";
                x2APIC = true;

                //  Set x2APIC mode in APIC base MSR
                MSRValue |= (1<<10);
                SetMSR(0x01B, MSRValue);
            }
            else
            {
                kout << "xAPIC\n";

                KernelACPIInfo->APICBase = MSRValue & 0x0000000FFFFFF000;
            }

            kout << "APIC Base MSR Value: 0x" << HEX << MSRValue << "\n";

            //  Set the spurious interrupt vector register to vector 39
            //  Set APIC enable bit
            auto RegValue = GetLAPICRegister<UINT32>(KernelACPIInfo->APICBase, 0xF0) | 0x100 | 0xFF;
            SetLAPICRegister(KernelACPIInfo->APICBase, 0xF0, RegValue);

            //  TODO: Set the task priority register
        }

        //  APIC timer setup
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