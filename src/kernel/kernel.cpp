#include "kernel.h"

namespace kernel
{
    KERNEL_DESCRIPTOR *KernelDescriptor;
    UINT8 KernelStack[8192];
}

void KernelMain(KERNEL_DESCRIPTOR *KernelInfo)
{
    using namespace kernel;

    //  Change the stack pointer
    ASMx64::SetRSP((UINT64)KernelStack + 8192);

    KernelDescriptor = new KERNEL_DESCRIPTOR;
    {
        mem::copy((byte*)KernelInfo, (byte*)KernelDescriptor, sizeof(KERNEL_DESCRIPTOR));
        log::kout.pFramebufferInfo = &(KernelDescriptor->GOPInfo);

        EFI_TIME_DESCRIPTOR *Time = &KernelDescriptor->TimeDescriptor;
        log::kout <<DEC<< Time->Year <<"-"<< Time->Month <<"-"<< Time->Day
                  << " | " << Time->Hour << ":" << Time->Minute << ":" << Time->Second << "\n";
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

        double SysMemSize = (double)KernelDescriptor->SysMemorySize / 0x40000000;
        kout << "Free SysMemory: "<<DOUBLE_DEC << SysMemSize << "GiB\n";

        double Number = -124.069420;
        UINT64 EncodedDouble = *(UINT64*)&Number;
        UINT8 Sign = EncodedDouble & 0x8000000000000000;
        kout << "EncodedDouble 0x" << HEX << EncodedDouble << "\n";

        ext2::RAMDisk RAMDisk(KernelDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);
        ext2::FILE TestFile;

        TestFile.Type = FILETYPE_REG;
        string::strncpy((UINT8*)"/boot/Makefile", TestFile.Path, MAX_PATH);

        RAMDisk.ReadFile(&TestFile, (UINT8*)KernelDescriptor->pSysMemory);

        auto pKernelWindow = new GUI::Window(500, 100, 400, 900);
        pKernelWindow->cout << "sizeof(DIRECTORY_ENTRY) = 0x"<<HEX<<sizeof(ext2::DIRECTORY_ENTRY) << "\n";
        pKernelWindow->cout << "Sizeof BLOCK_GROUP: 0x"<<HEX<<sizeof(ext2::BLOCK_GROUP) << "\n";
        pKernelWindow->cout << "Read " << TestFile.Size << " bytes from /boot/Makefile: \n" << (UINT8*)KernelDescriptor->pSysMemory << "\n";
        pKernelWindow->Draw();
    }

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
