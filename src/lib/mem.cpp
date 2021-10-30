#include "mem.h"
#include <io.h>
#include <fault.h>

void* operator new ( size_t count ){ return heap::malloc(count); }
void* operator new[] ( size_t count ) { return heap::malloc(count); }

void operator delete (void* ptr, UINT64 len) { heap::free(ptr); }
void operator delete[] (void* ptr) { heap::free(ptr); }

namespace heap
{
    // 32MiB page-aligned heap
    UINT8 __attribute__((aligned(4096))) pHeap[0x2000000];

    // Size of heap in bytes
    UINT64 heapSize = 0x2000000;

    // Number of occupied bytes in the heap
    UINT64 occupiedBytes = 0;

    // Pointer to first free BLOCK_HDR
    BLOCK_HDR *head;

    LOCK heapLock = LOCK_STATUS_FREE;

    /// Must be called only once, before using the malloc/free functions
    void init()
    {
        head = (BLOCK_HDR*) pHeap;
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

        block->size = size;

        if(block == head) head = newBlock;

        // Correct the pointers on the previous & next free block
        if(block->prevFree) block->prevFree->nextFree = newBlock;
        if(block->nextFree) block->nextFree->prevFree = newBlock;
    }

    /// Default heap memory allocator
    /// \param size Number of bytes to allocate
    /// \return
    void *malloc(UINT64 size)
    {
        AcquireLock(&heapLock);

        // TODO: Track RAM usage
        auto heapIterator = head;

        FaultLogAssertSerial(heapIterator, "heap head is NULL\n");

        // Drill down the linked-list until a large enough block is found
        while(heapIterator != nullptr && heapIterator->size < size)
            heapIterator = heapIterator->nextFree;

        // No more free blocks
        if(heapIterator == nullptr)
        {
            ReleaseLock(&heapLock);
            FaultLogAssertSerial(0, "No more free blocks for size %u. head=%16x\n", size, head);
            return nullptr;
        }

        // If the found block is perfectly sized, allocate the whole block
        // Otherwise, the extra free space should be converted into a new block
        if(heapIterator->size <= (size + sizeof(BLOCK_HDR))) allocFull(heapIterator);
        else allocPartial(heapIterator, size);

        ReleaseLock(&heapLock);

        FaultLogAssertSerial(((UINT64)heapIterator >= (UINT64)pHeap) && ((UINT64)heapIterator < ((UINT64)pHeap + heapSize)),
                             "heapIterator(0x%16x) is invalid\n", (UINT64)heapIterator);
        auto retVal = (void*)((UINT64)heapIterator + sizeof(BLOCK_HDR));

        return retVal;
    }

    /// Free function
    /// \param ptr
    void free(void *ptr)
    {
        AcquireLock(&heapLock);

        FaultLogAssertSerial(ptr, "ptr is NULL\n");

        auto block = (BLOCK_HDR*)((UINT64)ptr - sizeof(BLOCK_HDR));
        auto heapIterator = head;

        //SerialOut("DEallocating 0x%16x with size %u\n", (UINT64)ptr, block->size);

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

            ReleaseLock(&heapLock);
            return;
        }

        // If adjacent to a free block on the left, combine the two blocks
        if (left && ADJACENT(left, block))
        {
            // Enlarge left block, nothing else required
            left->size += block->size + sizeof(BLOCK_HDR);

            // Another merge is possible
            if (right && ADJACENT(left, right))
            {
                left->nextFree = right->nextFree;
                if (right->nextFree) right->nextFree->prevFree = left;

                left->size += right->size + sizeof(BLOCK_HDR);
            }
        }
        // If adjacent to a free block on the right
        else if (right && ADJACENT(block, right))
        {
            // Set pointers
            block->prevFree = right->prevFree;
            block->nextFree = right->nextFree;
            if (right->prevFree) right->prevFree->nextFree = block;
            if (right->nextFree) right->nextFree->prevFree = block;

            // Increase size
            block->size += right->size + sizeof(BLOCK_HDR);
        }

        ReleaseLock(&heapLock);
    }
}

void KeSysMemBitmapSet(UINT64 BlockID, UINT64 nBlocks)
{
    UINT64 SetBlocks = 0;

    for(auto BitMapIter = (UINT64*)(keSysDescriptor->pSysMemoryBitMap + 8*(BlockID/64));
        (UINT64)BitMapIter < (UINT64)(keSysDescriptor->pSysMemoryBitMap + keSysDescriptor->sysMemoryBitMapSize);
        BitMapIter++)
    {
        for (UINT64 Mask = 1, i = 0; i < 64; Mask <<= 1, i++)
        {
            if(SetBlocks == 0)
            {
                Mask <<= (BlockID % 64);
                i += (BlockID % 64);
            }

            *BitMapIter |= Mask;

            if(++SetBlocks >= nBlocks) return;
        }
    }
}

#define SYS_MEM_BLOCK_SIZE 0x100000

void *KeSysMalloc(UINT64 Size)
{
    /*
     * TODO: Locking should be implemented ASAP
     */
    UINT64 nBlocks = Size/SYS_MEM_BLOCK_SIZE + 1;

    UINT64 BlockID = 0;
    UINT64 nFoundBlocks = 0;

    /*
     * Run through the bitmap to find a contiguous chunk of memory
     */
    for(auto BitMapIter = (UINT64*)(keSysDescriptor->pSysMemoryBitMap);
        (UINT64)BitMapIter < (UINT64)(keSysDescriptor->pSysMemoryBitMap + keSysDescriptor->sysMemoryBitMapSize);
        BitMapIter++)
    {
        UINT64 bitmapChunk = *BitMapIter;

        for (UINT64 Mask = 1, i = 0; i < 64; Mask <<= 1, i++)
        {
            BlockID++;

            if(bitmapChunk & Mask)
            {
                nFoundBlocks = 0;
                continue;
            }

            if(++nFoundBlocks >= nBlocks)
            {
                KeSysMemBitmapSet(BlockID - nBlocks, nBlocks);
                return (void*)(keSysDescriptor->pSysMemory + (BlockID - nBlocks)*SYS_MEM_BLOCK_SIZE);
            }
        }
    }

    return nullptr;
}