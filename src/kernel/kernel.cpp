#include "kernel.h"

namespace kernel
{
    KERNEL_DESCRIPTOR *KernelDescriptor;
}

void KernelMain(KERNEL_DESCRIPTOR *KernelInfo)
{
    using namespace kernel;

    KernelDescriptor = new KERNEL_DESCRIPTOR;
    mem::copy((byte*)KernelInfo, (byte*)KernelDescriptor, sizeof(KERNEL_DESCRIPTOR));

    /*
     * Setup IDT, SSE, PIT, and PS2
     */
    {
        using namespace interrupt;
        using namespace ASMx64;
        using namespace PIC;

        auto pSSEINFO = heap::MallocAligned(512, 16);
        EnableSSE(pSSEINFO);

        cli();
        FillIDT64();
        InitPS2();
        InitPIT();
    }

    /*
     *  So far this only initializes APIC timer
     */
    ACPI::InitACPI();

    ASMx64::sti();

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
