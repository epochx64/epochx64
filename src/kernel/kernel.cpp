#include "kernel.h"

namespace kernel
{
    KERNEL_DESCRIPTOR *KernelDescriptor;
    UINT8 KernelStack[8192];
}

void KernelMain(KERNEL_DESCRIPTOR *KernelInfo)
{
    using namespace kernel;

    KernelDescriptor = new KERNEL_DESCRIPTOR;
    {
        ASMx64::SetRSP((UINT64)KernelStack + 8192);

        mem::copy((byte*)KernelInfo, (byte*)KernelDescriptor, sizeof(KERNEL_DESCRIPTOR));
        log::kout.pFramebufferInfo = &(KernelDescriptor->GOPInfo);

        EFI_TIME_DESCRIPTOR *Time = &KernelDescriptor->TimeDescriptor;
        log::kout <<DEC<< Time->Year <<"-"<< Time->Month <<"-"<< Time->Day <<"\n";
    }

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

    ACPI::InitACPI();

    ASMx64::sti();

    //  Temporary
    {
        auto pKernelWindow = new GUI::Window(500, 100, 400, 900);
        pKernelWindow->Draw();

        double SysMemSize = (double)KernelDescriptor->SysMemorySize / 0x40000000;
        log::kout << "Free SysMemory: "<<DEC<<math::RoundDouble<UINT64>(SysMemSize) << "GiB\n";
        log::kout << "Sizeof BLOCK_GROUP: 0x"<<HEX<<sizeof(ext2::BLOCK_GROUP) << "\n";
    }

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
