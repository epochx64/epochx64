#ifndef _BOOT_H
#define _BOOT_H

#include <defs/int.h>

/*
 * These must be identical to those found in lib/typedef.h
 */

#define FRAMEBUFFER_BYTES_PER_PIXEL 4

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
    //  Memory Info
    UINTN MapKey;
    UINTN MemoryMapSize;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    EFI_MEM_DESCRIPTOR *MemoryMap;

    FRAMEBUFFER_INFO FramebufferInfo;

    UINT64 RSDP;
} KERNEL_BOOT_INFO;

#endif