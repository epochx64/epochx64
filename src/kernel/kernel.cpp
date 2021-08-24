#include "kernel.h"
#include <fs/ext2.h>
#include <io.h>
#include <elf/elf.h>
#include <elf/relocation.h>
#include <fault.h>
#include <asm/asm.h>
#include <mem.h>
#include <interrupt.h>
#include <acpi.h>
#include <log.h>

/**********************************************************************
 *  Global variables
 *********************************************************************/
KE_SYS_DESCRIPTOR *keSysDescriptor;

/**********************************************************************
 *  Local variables
 *********************************************************************/
static ext2::RAMDisk *keRamDisk;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/// For debugging, walks through the free block linked-list
/// and prints the location and size of each block
void heapPrint()
{
    auto heapIterator = heap::head;

    printf("#################### BEGIN HEAP PRINTOUT #######################\n");

    while(heapIterator != nullptr)
    {
        printf("-------------------------------------------------------------\n"
               "HEAP FREE BLOCK SIZE: %u\n"
               "NEXT_FREE: 0x %16x\n"
               "PREV_FREE: 0x %16x\n"
               "-------------------------------------------------------------\n",
               heapIterator->size, heapIterator->nextFree, heapIterator->prevFree, (UINT64)heapIterator);

        heapIterator = heapIterator->nextFree;
    }

    printf("#################### END HEAP PRINTOUT #######################\n");
}

static void testAvl()
{

}

/**********************************************************************
 *  @details Reads a word from the PCI configuration space
 *  @param bus - Bus number
 *  @param device - Device number
 *  @param function - Function number
 *  @param offset - Register offset. Bits 1..0 must be 0
 *********************************************************************/
UINT16 keReadPciWord(UINT32 bus, UINT32 device, UINT32 function, UINT32 offset)
{
    /* Calculate address in PCI configuration space based on:
     * Enable Bit       31
     * Reserved         30 - 24
     * Bus Number       23 - 16
     * Device Number    15 - 11
     * Function Number  10 - 8
     * Register Offset  7 - 0
     */
    UINT32 address = 0x80000000 | offset | (function << 8) | (device << 11) | (bus << 16);

    /* Write the address to the CONFIG_ADDRESS register */
    outd(0xCF8, address);

    /* Read and return data from the CONFIG_DATA register */
    return ind(0xCFC) & 0xFFFF;
}

/**********************************************************************
 *  @details Populates the keSysDescriptor PCI-E structures
 *********************************************************************/
 static void keInitializePci()
 {
     /**************************************/

     //AvlTree tree;
     //KE_HANDLE counter = 0;
     //KE_HANDLE handles[30];

     //while(counter++ < 10)
     //{
     //    handles[counter] = counter;
     //    tree.Insert((void *) counter, &handles[counter]);
     //}

     ////treePrintout(tree.root);

     //tree.Remove(3);
     ////treePrintout(tree.root);

     //tree.Remove(1);
     ////treePrintout(tree.root);
     //tree.Remove(7);

     //while(counter++ < 20)
     //{
     //    handles[counter] = counter;
     //    tree.Insert((void *) counter, &(handles[counter]));
     //}

     //while(true);
     /***************************************/
 }

/**********************************************************************
 *  @details Entry point of the kernel
 *  @param kernelInfo - Pointer to KE_SYS_DESCRIPTOR struct
 *  @return None
 *********************************************************************/
void KeMain(KE_SYS_DESCRIPTOR *kernelInfo)
{
    static UINT8 __attribute__((aligned(16))) keInitFXSave[512];

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
    memset64((void*)keSysDescriptor->pSysMemoryBitMap, keSysDescriptor->sysMemoryBitMapSize, 0);

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
    sti();

    KeCalibrateAPICTimer(1e4);

    double sysMemSize = (double)keSysDescriptor->sysMemorySize / 0x40000000;
    printf("[ MEM ] System memory: %f GiB\n", sysMemSize);
    printf("HELLO 0x%04x\n", 0x4333);

    /* Initialize RAM disk */
    keRamDisk = new ext2::RAMDisk(keSysDescriptor->pRAMDisk, INITRD_SIZE_BYTES, false);

    PROCESS_PROPERTIES properties = { 0 };
    properties.pStdout = keStdout;

    /* Spawn DWM process */
    properties.path = "/boot/dwm.elf";
    properties.startTime = KeGetTime();
    properties.reschedule = false;
    properties.periodNanoSeconds = 0;
    properties.noWindow = true;
    KeCreateProcess(&properties);

    /* Spawn test process */
    properties.path = "/boot/terminal.elf";
    properties.startTime = KeGetTime() + 0.5e9;
    properties.noWindow = false;
    KeCreateProcess(&properties);

    /* Make the CPU idle (this is now the idle task for scheduler 0) */
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
    keSysDescriptor->KeSuspendTask = &KeSuspendTask;
    keSysDescriptor->KeSuspendCurrentTask = &KeSuspendCurrentTask;
    keSysDescriptor->KeResumeTask = &KeResumeTask;
    keSysDescriptor->KeGetCurrentTaskHandle = &KeGetCurrentTaskHandle;
    keSysDescriptor->KeGetTime = &KeGetTime;
    keSysDescriptor->KeQueryTask = &KeQueryTask;
    keSysDescriptor->KeQueryScheduler = &KeQueryScheduler;
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

    /* Find a scheduler to insert the task into */
    UINT64 chosenOne = 0;
    for(UINT64 i = 1; i < keSysDescriptor->nCores; i++)
    {
        if (keSchedulers[chosenOne]->nTasks > keSchedulers[i]->nTasks)
            chosenOne = i;
    }

    AcquireLock(&keSchedulers[chosenOne]->schedulerLock);
    keSchedulers[chosenOne]->TaskTree.Insert(task, &task->handle);
    keSchedulers[chosenOne]->AddTask(task);
    ReleaseLock(&keSchedulers[chosenOne]->schedulerLock);

    return task->handle;
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
            4,
            keSysDescriptor,
            properties->pStdout,
            properties->pStdin,
            properties->noWindow
            );

    return 0;
}

/**********************************************************************
 *  @details Gets the handle of the currently executing thread
 *  @param handle - Handle of the task
 *********************************************************************/
KE_HANDLE KeGetCurrentTaskHandle()
{
    UINT64 coreID = APICID();
    auto scheduler = keSchedulers[coreID];

    return scheduler->currentTask->handle;
}

/**********************************************************************
 *  @details Stops executing the current thread until unsuspend
 *  @param handle - Handle of the task
 *********************************************************************/
void KeSuspendTask(KE_HANDLE handle)
{
    auto scheduler = (Scheduler*)KeQueryScheduler(handle);

    /* Lock the scheduler to prevent it from accessing the structure we're about to edit */
    AcquireLock(&scheduler->schedulerLock);

    /* Get the task object */
    auto task = (Task*)scheduler->TaskTree.Search(handle);
    FaultLogAssertSerial(task, "Could not find task with handle %u\n", handle);

    /* Signal to the scheduler that the task is completed */
    task->ended = true;
    task->suspended = true;

    /* Unlock the scheduler */
    ReleaseLock(&scheduler->schedulerLock);
}

/**********************************************************************
 *  @details Stops executing the current thread until unsuspend
 *  @param handle - Handle of the task
 *********************************************************************/
void KeSuspendCurrentTask()
{
    KE_HANDLE currentHandle = KeGetCurrentTaskHandle();
    auto task = (Task*)KeQueryTask(currentHandle);
    KeSuspendTask(currentHandle);

    /* Invoke the APIC timer interrupt, repeating if the scheduler is locked */
    while (true)
    {
        asm volatile("int $48");
        asm volatile("pause");

        /* If the task has resumed, break out of this loop */
        if (!task->ended)
        {
            break;
        }
    }
}

/**********************************************************************
 *  @details Resumes a previously suspended task
 *  @param handle - Handle of the task to resume
 *  @param resumeTime - Time to resume the task in KE_TIME
 *********************************************************************/
void KeResumeTask(KE_HANDLE handle, KE_TIME resumeTime)
{
    auto scheduler = (Scheduler*)KeQueryScheduler(handle);

    /* Lock the scheduler to prevent it from accessing the structure we're about to edit */
    AcquireLock(&scheduler->schedulerLock);

    /* Get the task object */
    auto task = (Task*)scheduler->TaskTree.Search(handle);
    FaultLogAssertSerial(task, "Could not find task with handle %u\n", handle);

    task->ended = false;
    task->suspended = false;
    task->startTime = resumeTime;
    scheduler->AddTask(task);

    /* Unlock the scheduler */
    ReleaseLock(&scheduler->schedulerLock);
}

/**********************************************************************
 *  @details Kernel API function to get the current KE_TIME
 *  @return Time in nanoseconds since system startup
 *********************************************************************/
KE_TIME KeGetTime()
{
    return Scheduler::GetCurrentTime();
}

/**********************************************************************
 *  @details Gets a scheduler that contains the desired task
 *  @return Pointer to Scheduler
 *********************************************************************/
void *KeQueryScheduler(KE_HANDLE handle)
{
    /* For each scheduler */
    for (UINT64 i = 0; i < keSysDescriptor->nCores; i++)
    {
        Scheduler *scheduler = keSchedulers[i];

        /* Acquire lock for thread safety */
        AcquireLock(&scheduler->schedulerLock);

        /* Get the task if it exists */
        if (scheduler->TaskTree.Search(handle))
        {
            ReleaseLock(&scheduler->schedulerLock);
            return scheduler;
        }

        ReleaseLock(&scheduler->schedulerLock);
    }

    return nullptr;
}

/**********************************************************************
 *  @details Checks if a task exists, queries the info if it exists
 *  @return Pointer to Task
 *********************************************************************/
void *KeQueryTask(KE_HANDLE handle)
{
    /* For each scheduler */
    for (UINT64 i = 0; i < keSysDescriptor->nCores; i++)
    {
        Scheduler *scheduler = keSchedulers[i];

        /* Acquire lock for thread safety */
        AcquireLock(&scheduler->schedulerLock);

        /* Get the task if it exists */
        auto task = (Task*)scheduler->TaskTree.Search(handle);
        if (task)
        {
            ReleaseLock(&scheduler->schedulerLock);
            return task;
        }

        ReleaseLock(&scheduler->schedulerLock);
    }

    return nullptr;
}
