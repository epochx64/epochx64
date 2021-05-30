#ifndef _ACPI_H
#define _ACPI_H

#include <defs/int.h>
#include <log.h>
#include <epstring.h>
#include <asm/asm.h>
#include <mem.h>
#include <typedef.h>
#include <scheduler.h>
#include <io.h>

extern bool x2APIC;
extern "C" KE_AP_BOOTSTRAP_INFO *pAPBootstrapInfo;

void KeInitACPI();

void KeInitAPIC();
void CalibrateAPIC(UINT64 frequency);

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
        SetMSR(Register, (UINT64)Value);

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
        ReadMSR(Register, &MSRValue);

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


#endif
