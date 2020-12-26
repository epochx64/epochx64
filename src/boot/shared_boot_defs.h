#ifndef SHARED_BOOT_DEFS_H
#define SHARED_BOOT_DEFS_H

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

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

typedef struct
{
    UINTN MemoryMapSize;
    UINTN DescriptorSize;
    UINTN MapKey;
    UINT32 DescriptorVersion;
    EFI_MEM_DESCRIPTOR  *MemoryMap;
    EFI_TIME_DESCRIPTOR TimeDescriptor;

    UINT64 pRAMDisk;

    FRAMEBUFFER_INFO GOPInfo;
    FRAMEBUFFER_INFO KernelLog;

    UINT64 pSysMemory;
    UINT64 SysMemorySize;

    UINT64 pRSDP;
    UINT64 pXSDT;
    UINT32 APICBase;
    UINT32 APICInitCount;
} KERNEL_DESCRIPTOR;

#endif