#include "mem.h"

void* operator new ( size_t count ){ return heap::malloc(count); }
void* operator new[] ( size_t count ) { return heap::malloc(count); }

void operator delete (void* ptr, UINT64 len) { heap::free(ptr); }
void operator delete[] (void* ptr, UINT64 len) { heap::free(ptr); }

namespace heap {
    // 2MB page-aligned heap
    UINT8 __attribute__((aligned(4096))) pHeap[0x200000];

    // Size of heap in bytes
    UINT64 heapSize = 0x200000;

    // Number of occupied bytes in the heap
    UINT64 occupiedBytes = 0;

    // Pointer to first free BLOCK_HDR
    BLOCK_HDR *head = (BLOCK_HDR *) pHeap;

    /// Must be called only once, before using the malloc/free functions
    void init()
    {
        head->size = heapSize - sizeof(BLOCK_HDR);

        head->nextFree = nullptr;
        head->prevFree = nullptr;
    }

    /// Takes a free block and sets it as occupied by modifying the pointers in the linked-list structure.
    /// Will use ALL free space in the block
    /// \param block Pointer to the block
    void allocFull(BLOCK_HDR *block)
    {
        if(block == head) head = block->nextFree;

        // Correct the pointers on the previous & next free block
        if(block->prevFree) block->prevFree->nextFree = block->nextFree;
        if(block->nextFree) block->nextFree->prevFree = block->prevFree;
    }

    /// Takes a free block and allocates only some of its space, taking the rest and making a new free block
    /// \param block Pointer to the block
    /// \param size Number of bytes being allocated
    void allocPartial(BLOCK_HDR *block, UINT64 size)
    {
        auto newBlock = (BLOCK_HDR*)((UINT64)block + sizeof(BLOCK_HDR) + size);

        // Set the new block's pointers & attributes
        newBlock->prevFree = block->prevFree;
        newBlock->nextFree = block->nextFree;
        newBlock->size = block->size - size - sizeof(BLOCK_HDR);

        block->size -= newBlock->size + sizeof(BLOCK_HDR);

        if(block == head) head = newBlock;

        // Correct the pointers on the previous & next free block
        if(block->prevFree) block->prevFree->nextFree = newBlock;
        if(block->nextFree) block->nextFree->prevFree = newBlock;
    }

    /// Takes two blocks and merges them into one free block (preserving the LEFT block)
    /// \param left Left block in either free or occupied state
    /// \param right Right block in either free or occupied state
    /// \param rightState true if right is occupied, false if right is free
    void merge(BLOCK_HDR *left, BLOCK_HDR *right, bool rightState)
    {
        if(!rightState)
        {
            left->nextFree = right->nextFree;

            if(right->nextFree)
                right->nextFree->prevFree = left;

            if(left->prevFree)
                left->prevFree->nextFree = left;
        }

        left->size += right->size + sizeof(BLOCK_HDR);
    }

    /// Default heap memory allocator
    /// \param size Number of bytes to allocate
    /// \return
    void *malloc(UINT64 size)
    {
        // TODO: Track RAM usage
        auto heapIterator = head;

        // Drill down the linked-list until a large enough block is found
        while(heapIterator != nullptr && size > heapIterator->size)
            heapIterator = heapIterator->nextFree;

        // No more free blocks
        if(heapIterator == nullptr) return nullptr;

        // If the found block is perfectly sized, allocate the whole block
        // Otherwise, the extra free space should be converted into a new block
        if(heapIterator->size <= size + sizeof(BLOCK_HDR)) allocFull(heapIterator);
        else allocPartial(heapIterator, size);

        heapIterator->nextFree = nullptr;
        heapIterator->prevFree = nullptr;

        return (void*)((UINT64)heapIterator + sizeof(BLOCK_HDR));
    }

    /// Free function
    /// \param ptr
    void free(void *ptr)
    {
        auto block = (BLOCK_HDR*)((UINT64)ptr - sizeof(BLOCK_HDR));
        auto heapIterator = head;

        // Drill down the linked-list until we reach the next & previous free nodes to block
        while(heapIterator->nextFree != nullptr && (UINT64)heapIterator->nextFree < (UINT64)block)
            heapIterator = heapIterator->nextFree;

        BLOCK_HDR *left = heapIterator;
        BLOCK_HDR *right = heapIterator->nextFree;

        // If the block comes before the head node, special case
        if((UINT64)block < (UINT64)heapIterator)
        {
            left = nullptr;
            right = heapIterator;
            head = block;
        }

        block->prevFree = left;
        block->nextFree = right;

        // If no merging can be done
        if( (!left || !ADJACENT(left, block)) && (!right || !ADJACENT(block, right)) )
        {
            if(left) left->nextFree = block;
            if(right) right->prevFree = block;
            return;
        }

        // If adjacent to a free block on the left, combine the two blocks
        if (left && ADJACENT(left, block))
        {
            merge(left, block, true);
            block = left;
        }

        // If adjacent to a free block on the right, combine
        if(right && ADJACENT(block, right))
            merge(block, right, false);
    }

    void *MallocAligned(UINT64 Size, UINT64 Align)
    {
        Size += Align - 1;
        auto Return = (UINT64)malloc(Size);

        for(; (Return % Align) != 0; Return++);

        return (void *)Return;
    }
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
    /*
     * TODO: Locking should be implemented ASAP
     */
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