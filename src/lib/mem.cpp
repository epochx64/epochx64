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

}

using namespace kernel;

void SysMemBitMapSet(UINT64 BlockID, UINT64 nBlocks)
{
    UINT64 SetBlocks = 0;

    for(auto BitMapIter = (UINT8*)(KernelDescriptor->pSysMemoryBitMap + (BlockID/8));
        (UINT64)BitMapIter < (UINT64)(KernelDescriptor->pSysMemoryBitMap + KernelDescriptor->SysMemoryBitMapSize);
        BitMapIter++)
    {
        for (UINT8 Mask = 1, i = 0; i < 8; Mask <<= 1, i++)
        {
            if(SetBlocks == 0)
            {
                Mask <<= (BlockID % 8);
                i += (BlockID % 8);
            }

            *BitMapIter |= Mask;

            if(++SetBlocks >= nBlocks) return;
        }
    }
}

void *SysMalloc(UINT64 Size)
{
    UINT64 nBlocks = Size/4096 + 1;

    UINT64 BlockID = 0;
    UINT64 nFoundBlocks = 0;

    /*
     * Run through the bitmap to find a contiguous chunk of memory
     */
    for(auto BitMapIter = (UINT8*)(KernelDescriptor->pSysMemoryBitMap);
        (UINT64)BitMapIter < (UINT64)(KernelDescriptor->pSysMemoryBitMap + KernelDescriptor->SysMemoryBitMapSize);
        BitMapIter++)
    {
        for (UINT8 Mask = 1, i = 0; i < 8; Mask <<= 1, i++)
        {
            BlockID++;

            if(*BitMapIter & Mask)
            {
                nFoundBlocks = 0;
                continue;
            }

            if(++nFoundBlocks >= nBlocks)
            {
                SysMemBitMapSet(BlockID - nBlocks, nBlocks);
                return (void*)(KernelDescriptor->pSysMemory + (BlockID - nBlocks)*4096);
            }
        }
    }

    return nullptr;
}