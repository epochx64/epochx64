#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void* operator new   ( size_t count );
void* operator new[] ( size_t count );

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

    void*
    malloc(uint64_t size);
}

#endif
