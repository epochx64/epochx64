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
        using namespace log;

        auto pKernelWindow = new GUI::Window(500, 100, 500, 1200);

        for(EFI_MEM_DESCRIPTOR *MemDescriptor = KernelDescriptor->MemoryMap;
            (UINT64)MemDescriptor < (UINT64)KernelDescriptor->MemoryMap + KernelDescriptor->MemoryMapSize;
            MemDescriptor = (EFI_MEM_DESCRIPTOR*)((UINT64)MemDescriptor + KernelDescriptor->DescriptorSize)
                )
        {
            dout    << "0x"<<HEX<<MemDescriptor->PhysicalStart
                    << " is at 0x"<< MemDescriptor->VirtualStart
                    << " with 0x"<<MemDescriptor->NumberOfPages << " pages "
                    << "with type: " << MemDescriptor->Type << "\n";
        }

        double SysMemSize = (double)KernelDescriptor->SysMemorySize / 0x40000000;
        kout << "Free SysMemory: "<<DEC<<math::RoundDouble<UINT64>(SysMemSize) << "GiB\n";

        pKernelWindow->Draw();

        kout << "Testing SysMemory\n";
        for(auto Byte = (byte*)KernelDescriptor->pSysMemory;
        (UINT64)Byte < KernelDescriptor->pSysMemory + KernelDescriptor->SysMemorySize;
        Byte = Byte + 0x200000)
        {
            pKernelWindow->cout << "Testing address: 0x"<<HEX<<(UINT64)Byte << "\n";
            *Byte = 0;
        }

        kout << "Tested SysMemory\n";
    }

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
