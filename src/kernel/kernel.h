#ifndef KERNEL_H
#define KERNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../include/multiboot2.h"
#include "asm.h"
#include "log.h"
#include "heap.h"
#include "font_rom.h"
#include "graphics.h"
#include "paging.h"

extern "C"
{
    void
    kernel_main (
            uint64_t    u64multiboot2_info_addr,
            uint64_t    u64pml4_addr,
            uint64_t    u64heap_size,
            uint64_t    u64heap_addr
    );
};

namespace kernel

namespace kernel
{
    void
    process_multiboot();

    void
    init_vesa();
}

#endif