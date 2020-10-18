#include "kernel.h"

namespace kernel
{
    KERNEL_BOOT_INFO *KernelBootInfo;
    KERNEL_DESCRIPTOR KernelDescriptor;
}

void KernelMain(KERNEL_BOOT_INFO *KernelInfo)
{
    using namespace kernel;

    /*
     * Setup IDT, SSE, PIT, and PS2
     */
    KernelBootInfo = new KERNEL_BOOT_INFO;
    mem::copy((byte*)KernelInfo, (byte*)KernelBootInfo, sizeof(KERNEL_BOOT_INFO));
    KernelDescriptor.KernelBootInfo = KernelBootInfo;
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
    ACPI::InitACPI(KernelBootInfo, &(KernelDescriptor.KernelACPIInfo));

    ASMx64::sti();

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();

    /*
     * Test graphics routine (draws a circle)
     * TODO: Double buffering
     */
    //gfxroutine(new scheduler::TASK_ARG[3] {
    //    (TASK_ARG)(new double(500.0)),
    //    (TASK_ARG)(new double(500.0)),
    //    (TASK_ARG)(new int(100))
    //});
}
