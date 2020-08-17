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
{
    #define COLOR_WHITE graphics::Color(255, 255, 255, 0)
    #define COLOR_BLACK graphics::Color(0,0,0,0)

    typedef struct
    {
        uint64_t size;
    } mem_info_t;

    //  Goes out to COM1
    class dbgout
    {
    public:
        dbgout();

        dbgout &operator<<(char *str);
    };

    class kout
    {
    public:
        kout();

        kout &operator<<(char *str);
    };

    void
    process_multiboot();

    void
    init_vesa();
}

#endif