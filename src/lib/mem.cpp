#include "mem.h"

void* operator new ( size_t count ){ return heap::malloc(count); }
void* operator new[] ( size_t count ) { return heap::malloc(count); }

namespace heap
{
    //  2MB page-aligned heap
    UINT8 __attribute__((aligned(4096))) pHeap[0x200000];
    UINT64 HeapSize = 0x200000;

    void*
    malloc(UINT64 size)
    {
        //TODO: turn this into smaller easier functions, it very ugly function, also more documentation
        //      Maybe also use a better algorithm
        //TODO: Track RAM usage
        void            *found          = nullptr;
        block_info_t    *p_block_info   = nullptr;
        UINT64        i_heap_index    = 0;

        while(true)
        {
            p_block_info = (block_info_t*)( (UINT64)pHeap + i_heap_index );

            //  This block has never been touched
            //  we will assume all memory after it
            //  is free
            if ( p_block_info->data_size == 0 )
            {
                p_block_info->data_size     = size;
                p_block_info->block_flags   = BLOCK_OCCUPIED;

                found = (void*)( (uint64_t)p_block_info + sizeof(block_info_t) );
                return found;
            }

            //  Break if large enough and not occupied
            if ( !(p_block_info->block_flags & BLOCK_OCCUPIED) && p_block_info->data_size >= size )
                break;

            i_heap_index += sizeof(block_info_t) + p_block_info->data_size;
        }

        found                       = (void*)( (uint64_t)p_block_info + sizeof(block_info_t) );
        p_block_info->block_flags   = BLOCK_OCCUPIED;

        //  If the size of the section we just took is large enough
        //  to cut off and fit another block_info_t + at least 1 byte
        //  we will do that to save space. If it's not we'll just waste
        //  a few bytes
        if ( p_block_info->data_size >= 1 + sizeof(block_info_t) + size )
        {
            //  Size of the block before trim
            uint64_t old_size = p_block_info->data_size;

            //  Trim it and now look at the new block we're making after it
            p_block_info->data_size     = size;
            p_block_info                = (block_info_t*)( (uint64_t)p_block_info + sizeof(block_info_t) + size );

            p_block_info->data_size     = old_size - sizeof(block_info_t) - size;
            p_block_info->block_flags   = 0b00000000;
        }

        //  If for whatever reason the for loop doesn't end up retu
        return found;
    }

    void *MallocAligned(UINT64 Size, UINT64 Align)
    {
        Size += Align - 1;
        auto Return = (UINT64)malloc(Size);

        for(; (Return % Align) != 0; Return++);

        return (void *)Return;
    }
}

namespace mem
{
    void set(byte *dst, byte val, uint64_t count)
    {
        for ( uint64_t i = 0; i < count; i++ )
        {
            dst[i] = val;
        }
    }

    void copy(byte *src, byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = src[i];
        }
    }

    void zero(byte *dst, size_t count)
    {
        for(size_t i = 0; i < count; i++)
        {
            dst[i] = 0x00;
        }
    }
}
