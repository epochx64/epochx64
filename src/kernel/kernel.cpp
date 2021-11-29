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
static UINT64 keSyscallTable[] = {
        (UINT64)&KeCreateProcess,
        (UINT64)&KeGetTime,
        (UINT64)&KeScheduleTask,
        (UINT64)&KeSuspendTask,
        (UINT64)&KeSuspendCurrentTask,
        (UINT64)&KeGetCurrentTaskHandle,
        (UINT64)&KeResumeTask,
        (UINT64)&KeQueryTask,
        (UINT64)&KeQueryScheduler
};

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/// For debugging, walks through the free block linked-list
/// and prints the location and size of each block
void heapAvlTest()
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

    //AvlTree t;
    //UINT64 *key = (UINT64*)heap::malloc(4096);
    //UINT64 data;
    //for (UINT64 i = 1; i < 512; i++)
    //{
    //    key[i] = i + 1;
    //    t.Insert(&data, &key[i]);
    //}

    //for (UINT64 i = 1; i < 512; i+=2)
    //{
    //    t.Insert(&data, &key[i]);
    //    t.Remove(key[i]);
    //}

    //for (UINT64 i = 1; i < 512; i+= 2)
    //{
    //    t.Insert(&data, &key[i]);
    //}

    //heap::malloc()

    //while(true);
}

static void testAvl()
{

}

/**********************************************************************
 *  @details Reads a word from the PCI configuration space using the legacy access mechanism
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
    UINT32 address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset;

    /* Write the address to the CONFIG_ADDRESS register */
    outd(0xCF8, address);

    /* Read and return data from the CONFIG_DATA register */
    return ind(0xCFC) & 0xFFFF;
}

/**********************************************************************
 *  @details Gets basic device details from the PCI MCFG table
 *  @param segGroupBaseAddr - Base address of the segment group in the PCI configuration space
 *  @param bus - Bus number of the device
 *  @param device - Device number of the PCI bus
 *  @param func - Function number of the device
 *********************************************************************/
static PCI_DEVICE_BASE_HEADER *kePciGetDeviceHeader (UINT64 segGroupBaseAddr, UINT8 bus, UINT8 device, UINT8 func)
{
    /* Calculate the offset of the device's MMIO configuration space */
    UINT64 devConfigOffset = segGroupBaseAddr + (func + device*8 + bus*256)*4096;
    return (PCI_DEVICE_BASE_HEADER*)devConfigOffset;
}

/**********************************************************************
 *  @details Populates the keSysDescriptor PCIe structures
 *********************************************************************/
 static void keInitializePci()
 {
     static char *deviceClasses[] = {
             "Unclassified",
             "Mass Storage Controller",
             "Network Controller",
             "Display Controller",
             "Multimedia Controller",
             "Memory Controller",
             "Bridge",
             "Communications Controller",
             "Base System Peripheral",
             "Input Device Controller",
             "Docking Station",
             "Processor",
             "Serial Bus Controller",
             "Wireless Controller",
             "Intelligent Controller",
             "Sattelite Communication Controller",
             "Encryption Controller"
     };

     /* Get the MCFG table */
     auto mcfgTable = (MCFG_TABLE*)FindSDT((EXTENDED_SYSTEM_DESCRIPTOR_TABLE *)keSysDescriptor->pXSDT, "MCFG");
     FaultLogAssertSerial(mcfgTable, "[ PCI ] Failed to locate MCFG table\n");

     /* Get number of segment groups (usually 1, so assuming only 1 TODO: change one day) */
     UINT64 numEntries = (mcfgTable->header.Length - sizeof(SDT_HEADER) - 8)/sizeof(mcfgTable->segmentGroups[0]);
     SerialOut("Number of PCI segment groups: %u\n", numEntries);

     /* Enumerate the buses */
     for (UINT64 bus = 0; bus < 256; bus++)
     {
         /* Enumerate devices on the bus */
         for (UINT64 dev = 0; dev < 32; dev++)
         {
             PCI_DEVICE_BASE_HEADER *header = kePciGetDeviceHeader(mcfgTable->segmentGroups[0].baseConfigAddress, bus, dev, 0);

             /* Device doesn't exist */
             if (header->vendorId == 0xFFFF) continue;

             const char *deviceClass = (header->classCode > 0x10)? "Unknown Device" : deviceClasses[header->classCode];
             SerialOut("%u:%u.%u: %s\n", bus, dev, 0, deviceClass);

             /* If header type is multifunction device, enumerate functions */
             if (header->headerType & 0x80)
             {
                 for (UINT64 func = 1; func < 8; func++)
                 {
                     header = kePciGetDeviceHeader(mcfgTable->segmentGroups[0].baseConfigAddress, bus, dev, func);
                     deviceClass = (header->classCode > 0x10)? "Unknown Device" : deviceClasses[header->classCode];

                     SerialOut("%u:%u.%u: %s\n", bus, dev, func, deviceClass);
                 }
             }
         }
     }
 }

typedef struct SYS_MEM_NODE
{
    UINT64 flags; /* 0 = Free, 1 = Occupied */
    UINT64 physAddress;
    //UINT64 virtAddress;
    UINT64 numPages;
    SYS_MEM_NODE *pNext;
} SYS_MEM_NODE;

/* Keeps track of free blocks in system memory */
static SYS_MEM_NODE *sysMemList = nullptr;

/* Keeps track of occupied pages, using their physical address */
static AvlTree *sysMemAllocatedTree;

void keMapMemory(PAGE_TABLE *pageTable, UINT64 virtAddress, UINT64 physAddress, UINT64 numPages, UINT8 global);

/**********************************************************************
 *  @details Initializes internal datastructures for memory allocation
 *********************************************************************/
 #define EFI_MEMORY_RUNTIME          0x8000000000000000ULL
void keSysMemInit()
{
    /* Get UEFI memory map structure */
    EFI_MEM_DESCRIPTOR *memMap = keSysDescriptor->MemoryMap;
    UINT64 numMmapEntries = keSysDescriptor->MemoryMapSize / sizeof(EFI_MEM_DESCRIPTOR);

    sysMemList = (SYS_MEM_NODE*) heap::malloc(sizeof(SYS_MEM_NODE));
    SYS_MEM_NODE *traverse = sysMemList;

    /* Generate linked list structure */
    for (UINT64 i = 0; i < numMmapEntries; i++)
    {
        if (memMap[i].Attribute == EFI_MEMORY_RUNTIME)
        {
            //FaultLogAssertSerial(memMap[i].PhysicalStart == memMap[i].VirtualStart, "Expected identity-mapped mmap\n");
            traverse->physAddress = memMap[i].PhysicalStart;

            //traverse->virtAddress = memMap[i].VirtualStart;
            traverse->numPages = memMap[i].NumberOfPages;
            traverse->flags = 0;

            traverse->pNext = (SYS_MEM_NODE*) heap::malloc(sizeof(SYS_MEM_NODE));
            traverse = traverse->pNext;
        }
    }

    /* Initialize tree that keeps track of allocated memory */
    sysMemAllocatedTree = new AvlTree;
}

/**********************************************************************
 *  @details Allocates one page from the available pool and returns its physical address
 *********************************************************************/
void *kePhysAlloc()
{
    /* Allocate one page from available pool */
    UINT64 retVal = sysMemList->physAddress;
    sysMemList->physAddress += 4096;
    sysMemList->numPages--;

    if (sysMemList->numPages == 0)
    {
        SYS_MEM_NODE *oldNode = sysMemList;
        sysMemList = sysMemList->pNext;
        heap::free(oldNode);

        FaultLogAssertSerial(sysMemList, "No more physical memory available\n");
    }

    /* Create a new entry in the allocated tree */
    auto newNode = (SYS_MEM_NODE*)heap::malloc(sizeof(SYS_MEM_NODE));
    newNode->pNext = nullptr;
    newNode->numPages = 1;
    newNode->physAddress = retVal;
    newNode->flags = 1;
    sysMemAllocatedTree->Insert(newNode, &newNode->physAddress);

    return (void*)retVal;
}

/**********************************************************************
 *  @details Allocates memory and maps it to a virtual address given a PML4 pointer
 *  @param size - Number of bytes to allocate
 *  @param virtAddress - Virtual address to map memory to
 *  @param pml4 - Pointer to PML4 paging structure to map virtual address in
 *********************************************************************/
void keVirtAlloc(UINT64 size, UINT64 virtAddress, void *pml4)
{
    FaultLogAssertSerial(size, "Size of 0 passed\n");

    /* Physical address of first physical page being allocated.
     * This value is used as the key when doing lookups in sysMemAllocatedTree */
    UINT64 startPhys = sysMemList->physAddress;

    /* Number of pages that will be reserved */
    UINT64 nPages = (size / 4096) + (!!(size%4096));

    while (nPages != 0)
    {
        /* Always allocate from the start of the list */
        SYS_MEM_NODE *front = sysMemList;
        FaultLogAssertSerial(front, "No more memory left\n");

        /* Only allocate a portion of the block */
        if (nPages < front->numPages)
        {
            /* Map the memory */
            keMapMemory((PAGE_TABLE*)pml4, virtAddress, front->physAddress, nPages, 0);

            /* Make new node to split the node in two */
            auto newNode = (SYS_MEM_NODE*)heap::malloc(sizeof(SYS_MEM_NODE));
            newNode->pNext = front->pNext;
            newNode->numPages = front->numPages - nPages;
            newNode->physAddress = front->physAddress + nPages*4096;
            newNode->flags = 0;

            sysMemList = newNode;
            front->numPages = nPages;
            nPages = 0;
        }
        /* Allocate whole block */
        else
        {
            /* Map the memory */
            keMapMemory((PAGE_TABLE*)pml4, virtAddress, front->physAddress, front->numPages, 0);

            sysMemList = front->pNext;
            virtAddress += front->numPages*4096;
            nPages -= front->numPages;
        }

        /* Add the removed node to the allocated tree.
         * A single node in the tree stores the start-node of a linked-list of occupied, contiguous
         * memory regions in physical memory */
        front->flags = 1;
        front->pNext = nullptr;

        auto treeNodeTraverse = (SYS_MEM_NODE*)sysMemAllocatedTree->Search(startPhys);
        /* If the parent node has not been added to the list */
        if (!treeNodeTraverse)
        {
            sysMemAllocatedTree->Insert(front, &front->physAddress);
        }
        /* Append to linked list */
        else
        {
            while (treeNodeTraverse->pNext != nullptr)
            {
                treeNodeTraverse->pNext = front;
            }
        }
    }
}

static bool keSysMemListAdjacentLeft(SYS_MEM_NODE *node, UINT64 ptr)
{
    return ptr == (node->physAddress + node->numPages*4096);
}

static bool keSysMemListAdjacentRight(SYS_MEM_NODE *node, UINT64 ptr)
{
    return false;
}

void kePhysFree(void *ptr)
{
    /* Get the allocated node from the allocated tree */
    auto allocNode = (SYS_MEM_NODE*)sysMemAllocatedTree->Search((UINT64)ptr);

    /* Remove the node */
    sysMemAllocatedTree->Remove((UINT64)ptr);

    /* Find a node to grow in the free memory list */
    SYS_MEM_NODE *growNode = sysMemList;

    /*TOOD: Finish*/
}

void keVirtFree(void *ptr)
{

}

/**********************************************************************
 *  @details Returns the physical address of a passed virtual address
 *  @param virtAddress - Virtual address to get physical address of
********************************************************************/
UINT64 kePhysicalAddressFromVirtual(UINT64 virtAddress)
{
    PAGE_TABLE *pageTable = (PAGE_TABLE *)GetCR3Value();

    /* Calculate indexes in the page tables to start */
    UINT64 ptIndices[] = {
            (virtAddress >> 39) & 0x1FF,
            (virtAddress >> 30) & 0x1FF,
            (virtAddress >> 21) & 0x1FF,
            (virtAddress >> 12) & 0x1FF
    };

    for (UINT64 j = 0; j < 3; j++)
    {
        SerialOut("PageTableEntries = %16x\n", pageTable->entries[ptIndices[j]]);

        /* Allocate new page table if needed */
        if (pageTable->entries[ptIndices[j]] == 0)
        {
            SerialOut("oops %u\n", j);
            return 0;
        }

        pageTable = (PAGE_TABLE*)(pageTable->entries[ptIndices[j]] & 0xFFFFFFFFFFFFF000);
    }

    return (UINT64)pageTable->entries[ptIndices[3]];
}

/**********************************************************************
 *  @details Maps a virtual address to a physical address, given a root page table (PML4).
 *  Physical and virtual addresses must be
 *  Max amount of memory that can be mapped in one call: 134217728 pages (0.5TiB)
 *  @param pageTable - Pointer to PML4 structure to modify
 *  @param virtAddress - Virtual address to map physical address to
 *  @param physAddress - Physical address being mapped
 *  @param numPages - Number of pages to map
 *  @param global - Whether to mark the mapped memory as 'Global', meaning TLB flushes should
 *  not occur for this region
 *  //Massive TODO: Implement global feature
********************************************************************/
void keMapMemory(PAGE_TABLE *pageTable, UINT64 virtAddress, UINT64 physAddress, UINT64 numPages, UINT8 global)
{
    UINT64 shifts[] = { 39, 30, 21, 12 };
    //char *names[] = { "pml4", "pdpt", "pd", "pt" };

    for (UINT64 k = 0; k < 3; k++)
    {
        /* Get PDPT/PD/PT pointer */
        if (pageTable->entries[(virtAddress >> shifts[k]) & 0x1FF] == 0)
        {
            //SerialOut("Create %s[%u]: %16x\n", names[k], (virtAddress >> shifts[k] & 0x1FF), &pageTable->entries[(virtAddress >> shifts[k]) & 0x1FF]);
            void *newTable = kePhysAlloc();
            memset64(newTable, 4096, 0);
            pageTable->entries[(virtAddress >> shifts[k]) & 0x1FF] = (UINT64)newTable | 3;
        }
        pageTable = (PAGE_TABLE*)(pageTable->entries[(virtAddress >> shifts[k]) & 0x1FF] & 0xFFFFFFFFFFFFF000);

        /* Allocate 1GiB/2MiB/4KiB pages (if any) */
        UINT64 numSizedPages = (numPages*4096) >> (shifts[k + 1]);
        numPages -= numSizedPages*(1 << shifts[k + 1])/4096;

        for (UINT64 i = 0; i < numSizedPages; i++)
        {
            //SerialOut("Modify %s[%u]: %16x\n", names[k + 1], (virtAddress >> shifts[k + 1] & 0x1FF), &pageTable->entries[(virtAddress >> shifts[k + 1]) & 0x1FF]);
            UINT8 psBit = (k != 2)? (1 << 7) : (global << 8);
            pageTable->entries[(virtAddress >> shifts[k + 1]) & 0x1FF] = physAddress | 3 | psBit;
            virtAddress += 1 << shifts[k + 1];
            physAddress += 1 << shifts[k + 1];
        }

        if (numPages == 0)
        {
            return;
        }
    }

    FaultLogAssertSerial(numPages == 0, "Didn't count the pages right\n");
}

void keUnmapMemory()
{

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
    keSysMemInit();

    /*
     * Setup IDT, PS/2 devices, HPET, and APIC timer
     * Interrupts enabled after here
     */
    keSysDescriptor->keSyscallTable = (UINT64)keSyscallTable;
    interrupt::KeInitIDT();
    KeInitPS2();
    KeInitPIC();
    KeInitACPI();
    sti();

    double sysMemSize = (double)keSysDescriptor->sysMemorySize / 0x40000000;
    printf("[ MEM ] System memory: %f GiB\n", sysMemSize);
    printf("HELLO 0x%04x\n", 0x4333);

    keInitializePci();

    /* Make our own root page table */
    auto pml4 = (PAGE_TABLE*)kePhysAlloc();
    memset64(pml4, 4096, 0);

    /* Identity map the first 8 GiB of memory */
    keMapMemory(pml4, 0, 0, 2097152, 0);

    /* Set the root page table pointer */
    SetCR3Value((UINT64)pml4);

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
    FaultLogAssertSerial(handle != 0, "Invalid task handle %u\n", handle);

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
    FaultLogAssertSerial(handle != 0, "Invalid task handle %u\n", handle);

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
    FaultLogAssertSerial(handle != 0, "Invalid task handle %u\n", handle);

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
