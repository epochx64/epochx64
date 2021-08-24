#ifndef SHARED_BOOT_DEFS_H
#define SHARED_BOOT_DEFS_H

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

#define FRAMEBUFFER_BYTES_PER_PIXEL 4
#define INITRD_SIZE_BYTES 0x800000

/// From UEFISpec.h
typedef struct {
    ///
    /// Type of the memory region.
    /// Type EFI_MEMORY_TYPE is defined in the
    /// AllocatePages() function description.
    ///
    UINT32                Type;
    ///
    /// Physical address of the first byte in the memory region. PhysicalStart must be
    /// aligned on a 4 KiB boundary, and must not be above 0xfffffffffffff000. Type
    /// EFI_PHYSICAL_ADDRESS is defined in the AllocatePages() function description
    ///
    EFI_PHYSICAL_ADDRESS  PhysicalStart;
    ///
    /// Virtual address of the first byte in the memory region.
    /// VirtualStart must be aligned on a 4 KiB boundary,
    /// and must not be above 0xfffffffffffff000.
    ///
    EFI_VIRTUAL_ADDRESS   VirtualStart;
    ///
    /// NumberOfPagesNumber of 4 KiB pages in the memory region.
    /// NumberOfPages must not be 0, and must not be any value
    /// that would represent a memory page with a start address,
    /// either physical or virtual, above 0xfffffffffffff000.
    ///
    UINT64                NumberOfPages;
    ///
    /// Attributes of the memory region that describe the bit mask of capabilities
    /// for that memory region, and not necessarily the current settings for that
    /// memory region.
    ///
    UINT64                Attribute;
} EFI_MEM_DESCRIPTOR;

typedef struct
{
    UINT64 pFrameBuffer;
    UINT64 Width;
    UINT64 Height;
    UINT64 Pitch;
    UINT8 Bits;
} FRAMEBUFFER_INFO;

/// From UEFISpec.h
typedef struct
{
    UINT16  Year;           //  1900-9999
    UINT8   Month;          //  1-12
    UINT8   Day;            //  1-31
    UINT8   Hour;           //  0-23
    UINT8   Minute;         //  0-59
    UINT8   Second;         //  0-59
    UINT8   Pad1;
    UINT32  NanoSecond;     //  0-999,999,999
    INT16   TimeZone;       //  -1440 - 1440 or 2047
    UINT8   Daylight;
    UINT8   Pad2;
} EFI_TIME_DESCRIPTOR;

typedef UINT64 KE_TASK_ARG;
typedef UINT64 KE_HANDLE;

#ifndef WINDOWS_H
typedef UINT64 HANDLE;
#endif

#define INVALID_HANDLE 0

/* Number of nanoseconds since system boot */
typedef UINT64 KE_TIME;

typedef struct
{
    const char *path;
    KE_TIME startTime;
    UINT8 reschedule;
    KE_TIME periodNanoSeconds;
    void *pStdout;
    void *pStdin;
    UINT8 noWindow;
} PROCESS_PROPERTIES;

/* Kernel function list */
typedef KE_HANDLE (*KE_CREATE_PROCESS)(PROCESS_PROPERTIES *properties);
typedef KE_HANDLE (*KE_SCHEDULE_TASK)(UINT64 entry, KE_TIME startTime, UINT8 reschedule, KE_TIME periodNanoSeconds, UINT64 nArgs, ...);
typedef KE_HANDLE (*KE_GET_CURRENT_TASK_HANDLE)();
typedef void (*KE_SUSPEND_TASK)(KE_HANDLE handle);
typedef void (*KE_SUSPEND_CURRENT_TASK)();
typedef void (*KE_RESUME_TASK)(KE_HANDLE handle, KE_TIME resumeTime);
typedef void *(*KE_QUERY_TASK)(KE_HANDLE handle);
typedef void *(*KE_QUERY_SCHEDULER)(KE_HANDLE handle);
typedef KE_TIME (*KE_GET_TIME)();

typedef struct
{
    UINTN MemoryMapSize;
    UINTN DescriptorSize;
    UINTN MapKey;
    UINT32 DescriptorVersion;
    EFI_MEM_DESCRIPTOR  *MemoryMap;
    EFI_TIME_DESCRIPTOR TimeDescriptor;

    UINT64 pRAMDisk;

    FRAMEBUFFER_INFO gopInfo;

    UINT64 pSysMemory;
    UINT64 sysMemorySize;

    UINT64 pSysMemoryBitMap;
    UINT64 sysMemoryBitMapSize;

    UINT64 pRSDP;
    UINT64 pXSDT;
    UINT32 apicBase;
    UINT32 apicInitCount;

    UINT64 pHPET;                       //  Pointer to the HPET registers
    UINT64 hpetPeriod;                  //  Period in femptoseconds

    UINT64 pkeSchedulers;                 //  Pointer to array of Scheduler pointers
    UINT64 pTaskInfos;                  //  Pointer to array of KE_TASK_DESCRIPTOR pointers

    UINT64 nCores;                      //  Number of cores detected

    UINT64 pDwmDescriptor;

    /* Kernel function list */
    KE_CREATE_PROCESS KeCreateProcess;
    KE_SCHEDULE_TASK KeScheduleTask;
    KE_GET_CURRENT_TASK_HANDLE KeGetCurrentTaskHandle;
    KE_RESUME_TASK KeResumeTask;
    KE_SUSPEND_TASK KeSuspendTask;
    KE_SUSPEND_CURRENT_TASK KeSuspendCurrentTask;
    KE_GET_TIME KeGetTime;
    KE_QUERY_TASK KeQueryTask;
    KE_QUERY_SCHEDULER KeQueryScheduler;

} KE_SYS_DESCRIPTOR;

#endif