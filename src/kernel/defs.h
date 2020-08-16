#ifndef DEFS_H
#define DEFS_H

#define BITS 32
#define __x86       1
#define __x86_64    0
#define KERNEL_MEM  0xC0100000

#define N_GDT_ENTRIES       5
#define PAGE_DIR_SIZE       1024
#define PAGE_TABLE_SIZE     1024
#define PAGE_SIZE           4096
#define PAGE_ALIGN_MASK     0xFFFFF000
#define PAGEDIR_ALIGN_MASK  0xFFC00000

#define BLOCK_OCCUPIED              0b00000001

typedef void* PTR;


#endif
