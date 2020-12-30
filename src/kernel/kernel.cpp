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

        /*
         * Set the blocks which store the sysmemory bitmap as occupied
         */
        SysMemBitMapSet(0, KernelDescriptor->SysMemoryBitMapSize/BLOCK_SIZE + 1);
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

        kout << "Test SysMalloc: 0x" << HEX << (UINT64)SysMalloc(0x4200) << "\n";

        double SysMemSize = (double)KernelDescriptor->SysMemorySize / 0x40000000;
        kout << "Free SysMemory: "<<DOUBLE_DEC << SysMemSize << "GiB\n";

        ext2::RAMDisk RAMDisk(KernelDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);

        ext2::FILE TestFile;
        TestFile.Type = FILETYPE_REG;

        TestFile.Size = RAMDisk.GetFileSize((UINT8*)"/boot/test.elf");
        string::strncpy((UINT8*)"/boot/test.elf", TestFile.Path, MAX_PATH);

        auto TestFileBuf = SysMalloc(TestFile.Size);
        RAMDisk.ReadFile(&TestFile, (UINT8*)TestFileBuf);

        auto pKernelWindow = new GUI::Window(500, 100, 400, 900);
        pKernelWindow->cout << "Read " <<DEC<< TestFile.Size << " bytes from /boot/test.elf: \n" << (UINT8*)TestFileBuf << "\n";
        pKernelWindow->Draw();

        elf::LoadELF64((Elf64_Ehdr*)TestFileBuf);
    }

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();
}
