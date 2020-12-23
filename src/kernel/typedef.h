#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#include <lib/defs/int.h>
#include <boot/boot.h>
#include <boot/shared_boot_defs.h>

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

    //  Floating point version
    #define DOUBLE_DEC "\u0013"
    #define DOUBLE_DEC_ID 2
    #define DOUBLE_DEC_CHAR '\u0013'

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
    #define MADT_LAPIC 0
    #define X2APIC_BIT (1 << 21)
    #define APIC_REGISTER_ICR 0x300
    #define APBOOSTRAP_INFO_ENTRY_SIZE 0x10

    typedef struct
    {
        UINT64 pStack;
    } AP_BOOTSTRAP_INFO;

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
    } KERNEL_ACPI_INFO;
}

namespace kernel
{
    #define FRAMEBUFFER_BYTES_PER_PIXEL 4

    extern KERNEL_DESCRIPTOR *KernelDescriptor;
}

namespace scheduler
{
    typedef UINT64 TASK_ARG;

    typedef struct
    {
        //  For SSE/MMX floating point data
        //  (relevant whenever float or double is used)
        UINT8 FPUState[512];

        //  General purpose registers
        UINT64 r15;
        UINT64 r14;
        UINT64 r13;
        UINT64 r12;
        UINT64 r11;
        UINT64 r10;
        UINT64 r9;
        UINT64 r8;
        UINT64 rbp;
        UINT64 rsp;
        UINT64 rsi;
        UINT64 rdi;
        UINT64 rdx;
        UINT64 rcx;
        UINT64 rbx;
        UINT64 rax;

        /*
         * The RIP value of the task
         * The APIC timer ISR will have to push this value to the stack before iretq instruction
         *
         * This should only ever be 0 for the kernel thread/task (when the kernel initializes the scheduler)
         * It will be changed by the scheduler (apic timer) when its called for the first time
         */
        UINT64 IRETQRIP;
        UINT64 IRETQCS;
        UINT64 IRETQRFLAGS;
    } __attribute__((packed, aligned(16))) TASK_INFO;
}

namespace heap
{
#define BLOCK_OCCUPIED 0b00000001

    typedef struct
    {
        /*
         * Bit 0 : Occupied
         */
        uint8_t block_flags;

        /*
         * Size of 0 means the rest
         * of the heap is unoccupied
         */
        uint64_t data_size;
    } __attribute((packed))
    block_info_t;

    void* malloc(uint64_t size);
}

#endif