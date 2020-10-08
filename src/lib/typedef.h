#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#include <defs/int.h>

namespace interrupt
{
    #define PIC1_CMD        0x20
    #define PIC1_DATA       0x21
    #define PIC2_CMD        0xA0
    #define PIC2_DATA       0xA1
    #define PIC_READ_IRR    0x0a    /* OCW3 irq ready next CMD read */
    #define PIC_READ_ISR    0x0b    /* OCW3 irq service next CMD read */

    #define PIC_FREQUENCY 1193182

    #define PIC1		    0x20		/* IO base address for master PIC */
    #define PIC2		    0xA0		/* IO base address for slave PIC */
    #define PIC1_COMMAND	PIC1
    #define PIC1_DATA	    (PIC1+1)
    #define PIC2_COMMAND	PIC2
    #define PIC2_DATA	    (PIC2+1)

    #define PS2_COMMAND_PORT 0x64

    #define ICW1_ICW4	    0x01		/* ICW4 (not) needed */
    #define ICW1_SINGLE	    0x02		/* Single (cascade) mode */
    #define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
    #define ICW1_LEVEL	    0x08		/* Level triggered (edge) mode */
    #define ICW1_INIT	    0x10		/* Initialization - required! */
    #define ICW4_8086	    0x01		/* 8086/88 (MCS-80/85) mode */
    #define ICW4_AUTO	    0x02		/* Auto (normal) EOI */
    #define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
    #define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
    #define ICW4_SFNM	    0x10		/* Special fully nested (not) */

    typedef struct
    {
        uint16_t    offset_lower;   //  Lower 16 bits of the interrupt service routine (ISR)
        uint16_t    css;            //  Code segment selector
        uint8_t     IST_offset;     //  Interrupt Stack Table offset
        uint8_t     attributes;     //  Type and attributes
        uint64_t    offset_higher;  //  First 48 bits store higher offset of addr, rest is null
        uint16_t    reserved;       //  Null, so that the last 32 bits of the IDT descriptor are null
    } __attribute__((packed))
    IDT_DESCRIPTOR;


}

namespace log
{
    /*
     * I chose some arbitrary control characters
     */
    #define HEX "\u0011"
    #define HEX_ID 0
    #define HEX_CHAR '\u0011'

    #define DEC "\u0012"
    #define DEC_ID 1
    #define DEC_CHAR '\u0012'

    #define STEPUP_CHAR '\u0016'
    #define STEPUP "\u0016"

    typedef struct
    {
        UINT64 Col;
        UINT64 Lin;
    } TTY_COORD;
}

namespace ACPI
{
    typedef struct
    {
        char Signature[8];
        UINT8 Checksum;
        char OEMID[6];
        UINT8 Revision;
        UINT32 RSDTAddress;

        UINT32 Length;
        UINT64 XSDTAddress;
        UINT8 ExtendedChecksum;
        UINT8 reserved[3];
    } __attribute__((packed)) RSDP_DESCRIPTOR;

    /*
     * Common header shared between all the SDTs (System Descriptor Tables)
     */
    typedef struct
    {
        char Signature[4];
        UINT32 Length;
        UINT8 Revision;
        UINT8 Checksum;
        char OEMID[6];
        char OEMTableID[8];
        UINT32 OEMRevision;
        UINT32 CreatorID;
        UINT32 CreatorRevision;
    } __attribute__((packed)) SDT_HEADER;

    typedef struct
    {
        SDT_HEADER SDTHeader;
        UINT64 pSDTArray[];
    } __attribute__((packed)) EXTENDED_SYSTEM_DESCRIPTOR_TABLE;

    typedef struct
    {
        SDT_HEADER Header;

    } __attribute__((packed)) MULTIPLE_APIC_DESCRIPTOR_TABLE;

    /*
     * Bundles up all the ACPI tables that we are
     * interested in
     */
    typedef struct
    {
        RSDP_DESCRIPTOR *RSDPDescriptor;
        EXTENDED_SYSTEM_DESCRIPTOR_TABLE *XSDT;
    } KERNEL_ACPI_TABLE;
}

namespace kernel
{
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

    typedef struct
    {
        KERNEL_BOOT_INFO KernelBootInfo;
    } KERNEL_DESCRIPTOR;

    extern KERNEL_DESCRIPTOR KernelDescriptor;
}

#endif