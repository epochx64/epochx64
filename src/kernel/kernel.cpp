#include "kernel.h"

/*
 * TODO: This ENTIRE project need to turn PascalCase to camelCase
 * TODO: TOO MANY NAMESPACES
 * TODO: IMPROVE ALGORITHMS (nothing specific just look for unoptimized code)
 */

/**********************************************************************
 *  Global variables
 *********************************************************************/

ext2::RAMDisk *keRamDisk;
KE_SYS_DESCRIPTOR *keSysDescriptor;
UINT8 __attribute__((aligned(16))) keInitStack[8192];

/**********************************************************************
 *  Local variables
 *********************************************************************/

static UINT8 __attribute__((aligned(16))) keInitFXSave[512];

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/// For debugging, walks through the free block linked-list
/// and prints the location and size of each block
void heapPrint()
{
    auto heapIterator = heap::head;

    while(heapIterator != nullptr)
    {
        log::kout << "[ HEAP ] Free block size 0x"<<HEX<<heapIterator->size << " at 0x"<<HEX<<(UINT64)heapIterator << "\n";
        heapIterator = heapIterator->nextFree;
    }
}

/**********************************************************************
 *  @details Entry point of the kernel
 *  @param kernelInfo - Pointer to KE_SYS_DESCRIPTOR struct
 *  @return None
 *********************************************************************/
void KeMain(KE_SYS_DESCRIPTOR *kernelInfo)
{
    /* Initialize heap (malloc, free, etc.) */
    heap::init();

    /* Change the stack pointer */
    SetRSP((UINT64)keInitStack + 8192);

    /* Populate keSysDescriptor struct */
    keSysDescriptor = new KE_SYS_DESCRIPTOR;
    *keSysDescriptor = *kernelInfo;

    /* Set kout framebuffer pointer */
    log::kout.pFramebufferInfo = &(keSysDescriptor->gopInfo);

    /* Zero the sysmemory bitmap */
    for(UINT64 i = 0; i < keSysDescriptor->sysMemoryBitMapSize; i+=8)
    {
        *(UINT64*)(keSysDescriptor->pSysMemoryBitMap + i) = 0;
    }

    /* Set the blocks which store the SysMemory bitmap as occupied */
    KeSysMemBitmapSet(0, keSysDescriptor->sysMemoryBitMapSize/BLOCK_SIZE + 1);

    /*
     * Setup IDT, PS/2 devices, PIT, and APIC timer
     * Interrupts enabled after here
     */
    EnableSSE(keInitFXSave);
    interrupt::KeInitIDT();
    KeInitPS2();
    KeInitPIC();
    KeInitAPI();
    KeInitACPI();

    //CalibrateAPIC(1e4);

    /* Temporary */
    {
        using namespace log;

        keRamDisk = new ext2::RAMDisk(keSysDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);;

        double sysMemSize = (double)keSysDescriptor->sysMemorySize / 0x40000000;
        kout << "Free SysMemory: "<<DOUBLE_DEC << sysMemSize << "GiB\n";

        /* Spawn a process (will change) */
        Process p("/boot/test.elf");
    }

    /* Start interrupts and make the CPU idle (this is now the idle task for scheduler 0) */
    sti();
    hlt();
}

/**********************************************************************
 *  @details Populates keSysDescriptor function pointer list
 *  @return None
 *********************************************************************/
void KeInitAPI()
{
    keSysDescriptor->KeScheduleTask = &KeScheduleTask;
    keSysDescriptor->KeGetTime = &KeGetTime;
}

/**********************************************************************
 *  @details Kernel API function to spawn a task aka thread
 *  @param entry - Pointer to function entry point
 *  @param startTime - KE_TIME when the task should start
 *  @param reschedule - Whether to reschedule the task once it completes
 *  @param periodMicroSeconds - Period of consecutive executions of the task
 *  @param args - Array of pointers to arguments
 *  @return None
 *********************************************************************/
KE_HANDLE KeScheduleTask(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, KE_TASK_ARG* args)
{
    Task *task = new Task(entry, startTime, reschedule, periodNanoSeconds, args);
    Scheduler::ScheduleTask(task);
    return task->ID;
}

/**********************************************************************
 *  @details Kernel API function to get the current KE_TIME
 *  @return Time in nanoseconds since system startup
 *********************************************************************/
KE_TIME KeGetTime()
{
    return Scheduler::GetCurrentTime();
}