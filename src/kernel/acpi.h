#ifndef _ACPI_H
#define _ACPI_H

#include <defs/int.h>
#include <kernel/log.h>
#include <string.h>
#include <asm/asm.h>
#include <kernel/typedef.h>

namespace ACPI
{
    #define X2APIC_BIT (1 << 21)

    extern bool x2APIC;

    void InitACPI(kernel::KERNEL_BOOT_INFO *KernelInfo, KERNEL_ACPI_INFO *KernelACPIInfo);

    void InitAPIC(KERNEL_ACPI_INFO *KernelACPIInfo);

    /*
     * These depend on the value of bool x2APIC
     * If x2APIC is enabled, MSRs are used instead of MMIO for configuration
     * Offset is always in MMIO form
     */
    template <class T> void SetLAPICRegister(UINT64 pLAPIC, UINT64 Offset, T Value)
    {
        /*
         * Register will either be
         * - x2APIC - The LAPIC MSR ID
         * - xAPIC  - The MMIO offset within pLAPIC
         */
        UINT64 Register = Offset;

        if(x2APIC)
        {
            Register = (Register/0x10) + 0x800;
            ASMx64::SetMSR(Register, (UINT64)Value);

            return;
        }

        Register += pLAPIC;
        *((T*)Register) = Value;
    }

    template <class T> T GetLAPICRegister(UINT64 pLAPIC, UINT64 Offset)
    {
        /*
         * Register will either be
         * - x2APIC - The LAPIC MSR ID
         * - xAPIC  - The MMIO offset within pLAPIC
         */
        UINT64 Register = Offset;

        if(x2APIC)
        {
            Register = (Register/0x10) + 0x800;
            UINT64 MSRValue;
            ASMx64::ReadMSR(Register, &MSRValue);

            return (T)MSRValue;
        }

        Register += pLAPIC;
        return *((T*)Register);
    }

    /*
     * Locate an SDT (System Descriptor Table) within the RSDT (XSDT in this case)
     * Returns pointer to the requested SDT
     */
    UINT64 FindSDT(EXTENDED_SYSTEM_DESCRIPTOR_TABLE *XSDT, char *ID);
}

#endif
