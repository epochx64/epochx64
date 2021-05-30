#include "kernel.h"

/**********************************************************************
 *  Global variables
 *********************************************************************/
KE_SYS_DESCRIPTOR *keSysDescriptor;

/**********************************************************************
 *  Local variables
 *********************************************************************/

static ext2::RAMDisk *keRamDisk;

static UINT8 __attribute__((aligned(16))) keInitStack[8192];
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
    /* Enable SIMD instructions */
    EnableSSE(keInitFXSave);

    /* Initialize heap (malloc, free, etc.) */
    heap::init();

    /* Populate keSysDescriptor struct */
    keSysDescriptor = new KE_SYS_DESCRIPTOR;
    *keSysDescriptor = *kernelInfo;

    /* Create an stdout object */
    auto keStdout = createStdout();
    setStdout(keStdout);

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
    KeInitAPI();
    interrupt::KeInitIDT();
    KeInitPS2();
    KeInitPIC();
    KeInitACPI();

    //CalibrateAPIC(1e4);

    double sysMemSize = (double)keSysDescriptor->sysMemorySize / 0x40000000;
    //log::kout << "Free SysMemory: "<<DOUBLE_DEC << sysMemSize << "GiB\n";
    printf("[ MEM ] Free system memory: %f GiB\n", sysMemSize);
    printf("HELLO 0x%04x\n", 0x4333);

    /* Initialize RAM disk */
    keRamDisk = new ext2::RAMDisk(keSysDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);

    PROCESS_PROPERTIES properties;
    properties.pStdout = keStdout;

    /* Spawn DWM process */
    properties.path = "/boot/dwm.elf";
    properties.startTime = KeGetTime();
    properties.reschedule = false;
    properties.periodNanoSeconds = 0;
    properties.noWindow = true;
    KeCreateProcess(&properties);

    /* Spawn test process */
    properties.path = "/boot/test.elf";
    properties.startTime = KeGetTime() + 0.5e9;
    properties.noWindow = false;
    KeCreateProcess(&properties);

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
    keSysDescriptor->KeCreateProcess = &KeCreateProcess;
    keSysDescriptor->KeScheduleTask = &KeScheduleTask;
    keSysDescriptor->KeSuspendCurrentTask = &KeSuspendCurrentTask;
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
KE_HANDLE KeScheduleTask(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, ...)
{
    va_list argList;

    va_start(argList, nArgs);
    Task *task = new Task(entry, startTime, reschedule, periodNanoSeconds, nArgs, argList);
    va_end(argList);

    Scheduler::ScheduleTask(task);
    return task->ID;
}

/**********************************************************************
 *  @details Kernel API function to spawn a process from the RAM disk
 *           Eventually will support spawning from persistent storage
 *  @param properties - Pointer to PROCESS_PROPERTIES struct
 *  @param path - RAM disk path to executable elf
 *  @param startTime - KE_TIME when the task should start
 *  @param reschedule - Whether to reschedule the task once it completes
 *  @param periodMicroSeconds - Period of consecutive executions of the task
 *  @param stdout - Pointer to stdout struct or nullptr
 *  @param noWindow - If stdout is nullptr, a console window is made, use this to override
 *                    and don't spawn a window
 *  @return None
 *********************************************************************/
KE_HANDLE KeCreateProcess(PROCESS_PROPERTIES *properties)
{
    ext2::File file(keRamDisk, FILETYPE_REG, properties->path);

    auto fileBuf = KeSysMalloc(file.Size());
    keRamDisk->ReadFile(file.Data(), (UINT8*)fileBuf);

    auto entry = (void (*)(KE_SYS_DESCRIPTOR*))elf::LoadELF64((Elf64_Ehdr*)fileBuf);

    KeScheduleTask((UINT64)entry,
            properties->startTime,
            properties->reschedule,
            properties->periodNanoSeconds,
            3,
            keSysDescriptor,
            properties->pStdout,
            properties->noWindow
            );

    return 0;
}

/**********************************************************************
 *  @details Stops executing the current thread until unsuspend
 *********************************************************************/
void KeSuspendCurrentTask()
{
    UINT64 coreID = APICID();
    auto scheduler = keSchedulers[coreID];
    auto currentTask = scheduler->currentTask;

    /* Lock the scheduler to prevent it from accessing the structure we're about to edit */
    scheduler->isLocked = true;

    /* Signal to the scheduler that the task is completed */
    currentTask->ended = true;
    currentTask->suspended = true;

    /* Unlock the scheduler */
    scheduler->isLocked = false;

    /* Wait for the scheduler tick */
    hlt();
}

/**********************************************************************
 *  @details Kernel API function to get the current KE_TIME
 *  @return Time in nanoseconds since system startup
 *********************************************************************/
KE_TIME KeGetTime()
{
    return Scheduler::GetCurrentTime();
}