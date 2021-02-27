#include "kernel.h"

namespace kernel
{
    KERNEL_DESCRIPTOR *KernelDescriptor;
    UINT8 __attribute__((aligned(16))) KernelStack[16384];

    ext2::RAMDisk *RAMDisk;
    /*
     * TODO: Should convert a lot of libs to be more portable, which requires moving
     *       a lot of global variables here
     * TODO: This ENTIRE project need to turn CapitalCase to camelCase
     * TODO: TOO MANY NAMESPACES
     */
}

void KernelMain(KERNEL_DESCRIPTOR *kernelInfo)
{
    using namespace kernel;

    //  Change the stack pointer
    ASMx64::SetRSP((UINT64)KernelStack + 16384);

    KernelDescriptor = new KERNEL_DESCRIPTOR;
    {
        mem::copy((byte*)kernelInfo, (byte*)KernelDescriptor, sizeof(KERNEL_DESCRIPTOR));
        log::kout.pFramebufferInfo = &(KernelDescriptor->GOPInfo);

        EFI_TIME_DESCRIPTOR *Time = &KernelDescriptor->TimeDescriptor;
        log::kout << "UEFI Time: " <<DEC<< Time->Year <<"-"<< Time->Month <<"-"<< Time->Day
                  << " | " << Time->Hour << ":" << Time->Minute << ":" << Time->Second << "\n";

        /*
         * Zero the sysmemory bitmap
         */
        for(UINT64 i = 0; i < KernelDescriptor->SysMemoryBitMapSize; i+=8)
            *(UINT64*)(KernelDescriptor->pSysMemoryBitMap + i) = 0;

        /*
         * Set the blocks which store the sysmemory bitmap as occupied
         */
        SysMemBitMapSet(0, KernelDescriptor->SysMemoryBitMapSize/BLOCK_SIZE + 1);

        ext2::RAMDisk ramDisk(KernelDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);
        kernel::RAMDisk = &ramDisk;
    }

    /*
     * Setup IDT, SSE, PIT, and PS2
     */
    {
        using namespace interrupt;
        using namespace ASMx64;

        auto pSSEINFO = heap::MallocAligned(512, 16);
        EnableSSE(pSSEINFO);

        cli();
        FillIDT64();
        InitPS2();
        InitPIT();
        ACPI::InitACPI();
    }

    ASMx64::sti();

    //  Temporary
    {
        using namespace log;

        double SysMemSize = (double)KernelDescriptor->SysMemorySize / 0x40000000;
        kout << "Free SysMemory: "<<DOUBLE_DEC << SysMemSize << "GiB\n";

        Process p("/boot/test.elf");
    }

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
