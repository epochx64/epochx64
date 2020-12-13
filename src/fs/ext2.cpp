#include "ext2.h"

namespace ext2
{
    RAMDisk::RAMDisk(UINT64 pStart, UINT64 Size)
    {
        DiskSize = Size;
        pLBA0 = pStart;
        pSuperBlock = (SUPERBLOCK*)(pLBA0 + 1024);
        BlockGroups = (BLOCK_GROUP*)pSuperBlock;

        //  Zero out the disk
        for(auto i = (UINT64*)pStart; (UINT64)i < pStart + Size; i++) *i = 0;
    }

    BLOCK_GROUP *RAMDisk::GetBlockGroup(UINT64 ID)
    {
        return &(BlockGroups[ID]);
    }

    INODE *RAMDisk::GetINode(UINT64 ID)
    {
        UINT64 BlockGroupID = (ID - 1) / INODES_PER_BLOCK_GROUP;
        auto INodeTable = (INODE*)GetBlockGroup(BlockGroupID)->InodeTable;;

        UINT64 INodeIndex = (ID - 1) % INODES_PER_BLOCK_GROUP;
        return &(INodeTable[INodeIndex]);
    }

    BLOCK *RAMDisk::GetBlock(UINT64 ID)
    {
        return (BLOCK*)((UINT64)BlockGroups + (ID-1)*BLOCK_SIZE);
    }

    BLOCK_ID RAMDisk::AllocateBlock()
    {
        /*
         * Iterate through all the block groups
         */
        BLOCK_ID BlockNum = 0;
        for (auto iBlockGroup = (BLOCK_GROUP*)pSuperBlock;
             (UINT64)iBlockGroup < (UINT64)pSuperBlock + pSuperBlock->s_blocks_count * BLOCK_SIZE;
             iBlockGroup = (BLOCK_GROUP*)((UINT64)iBlockGroup + sizeof(BLOCK_GROUP)))
        {
            /*
             * Go through the block group's block bitmap
             *
             * For each byte, look at the bits
             * 1 = occupied block
             * 0 = unoccupied block
             */

            for (auto BitmapIterator = (UINT8 *) iBlockGroup->BlockBitMap;
                 BitmapIterator < (UINT8 *) iBlockGroup->BlockBitMap + BLOCKS_PER_BLOCK_GROUP / 8; BitmapIterator++)
            {
                for (UINT8 Mask = 0b00000001, i = 0; i < 8; Mask <<= 1, i++)
                {
                    BlockNum++;

                    //  If block is occupied
                    if(*BitmapIterator & Mask) continue;

                    //  Set the block as occupied
                    *BitmapIterator |= Mask;

                    return BlockNum;
                }
            }
        }

        return 0;
    }

    INODE_ID RAMDisk::AllocateINode()
    {
        /*
         * Iterate through all the block groups
         */
        INODE_ID INodeNum = 0;

        for(
                auto iBlockGroup = (BLOCK_GROUP*)pSuperBlock;
                (UINT64)iBlockGroup < (UINT64)pSuperBlock + pSuperBlock->s_blocks_count * BLOCK_SIZE;
                iBlockGroup = (BLOCK_GROUP*)((UINT64)iBlockGroup + sizeof(SUPERBLOCK)))
        {
            /*
             * Go through the block group's inode bitmap
             *
             * For each byte, look at the bits
             * 1 = occupied inode
             * 0 = unoccupied inode
             */
            for(
                    auto BitmapIterator = (UINT8*)iBlockGroup->InodeBitMap;
                    BitmapIterator < (UINT8*)iBlockGroup->InodeBitMap + INODES_PER_BLOCK_GROUP / 8;
                    BitmapIterator++)
            {
                for (UINT8 Mask = 0b00000001, i = 0; i < 8; Mask <<= 1, i++)
                {
                    INodeNum++;

                    //  If inode is occupied
                    if(*BitmapIterator & Mask)
                        continue;

                    //  Set the inode as occupied
                    *BitmapIterator |= Mask;

                    /*
                     * Write pointer to found block in INode
                     */
                    return INodeNum;
                }
            }
        }

        return 0;
    }

    UINT64 RAMDisk::GetTIBPEntry(INODE *INode, UINT64 Index)
    {
        UINT64 IBPIndex[3] =
                {
                        Index / DIBP_SPAN, //  TIBPIndex
                        (Index % DIBP_SPAN) / SIBP_SPAN, // DIBPIndex
                        (Index % SIBP_SPAN) / BLOCK_SPAN // SIBPIndex
                };

        auto IBP = &INode->i_block[14];
        for(auto i : IBPIndex)
        {
            if(*IBP == 0)
                *IBP = AllocateBlock();

            IBP = (UINT32*)((UINT64)GetBlock(*IBP) + i*4);
        }

        return *IBP;
    }

    void RAMDisk::SetTIBPEntry(INODE *INode, UINT64 Index, UINT64 Value)
    {
        UINT64 IBPIndex[3] =
                {
                        Index / DIBP_SPAN, //  TIBPIndex
                        (Index % DIBP_SPAN) / SIBP_SPAN, // DIBPIndex
                        (Index % SIBP_SPAN) / BLOCK_SPAN // SIBPIndex
                };

        auto IBP = &INode->i_block[14];
        for(auto i : IBPIndex)
        {
            if(*IBP == 0)
                *IBP = AllocateBlock();

            IBP = (UINT32*)((UINT64)GetBlock(*IBP) + i*4);
        }

        *IBP = Value;
    }

    /*
     * Uses an inode's Triply Indirect Block Pointer to locate
     * DIRECTORY ENTRY pointer
     */
    DIRECTORY_ENTRY *RAMDisk::GetINodeDirectoryEntry(INODE *INode, UINT64 EntryID)
    {
        UINT64 DirEntriesPerBlock = BLOCK_SIZE/sizeof(DIRECTORY_ENTRY);
        UINT64 BlockIndex = EntryID/DirEntriesPerBlock;

        if(BlockIndex < 12)
            return &((DIRECTORY_ENTRY*)(GetBlock(INode->i_block[BlockIndex])))[EntryID % DirEntriesPerBlock];

        UINT64 TIPBEntry = GetTIBPEntry(INode, EntryID);

        return &((DIRECTORY_ENTRY*)(GetBlock(TIPBEntry)))[EntryID % DirEntriesPerBlock];
    }

    void RAMDisk::SetINodeDirectoryEntry(INODE *INode, UINT64 EntryID, DIRECTORY_ENTRY *DirectoryEntry)
    {
        UINT64 DirEntriesPerBlock = BLOCK_SIZE/sizeof(DIRECTORY_ENTRY);
        UINT64 BlockIndex = EntryID/DirEntriesPerBlock;

        if(BlockIndex < 12)
        {
            auto pBlock = GetBlock(INode->i_block[BlockIndex]);
            ((DIRECTORY_ENTRY*)pBlock)[EntryID % DirEntriesPerBlock] = *DirectoryEntry;

            return;
        }

        UINT64 TIPBEntry = GetTIBPEntry(INode, EntryID);

        ((DIRECTORY_ENTRY*)(GetBlock(TIPBEntry)))[EntryID % DirEntriesPerBlock] = *DirectoryEntry;
    }

    void RAMDisk::AllocateBlocks(INODE* INode, UINT64 Size)
    {
        /*
         * 3 nested for loops gross
         */
        UINT64 BlocksAllocated = 0;

        /*
         * Iterate through all the block groups
         */
        for (auto iBlockGroup = (BLOCK_GROUP*)pSuperBlock;
             (UINT64)iBlockGroup < (UINT64)pSuperBlock + pSuperBlock->s_blocks_count * BLOCK_SIZE;
             iBlockGroup = (BLOCK_GROUP*)((UINT64)iBlockGroup + sizeof(SUPERBLOCK)))
        {
            UINT64 BlockNum = 0;

            /*
             * Go through the block group's block bitmap
             *
             * For each byte, look at the bits
             * 1 = occupied block
             * 0 = unoccupied block
             */
            for(auto BitmapIterator = (UINT8*)iBlockGroup->BlockBitMap;
            BitmapIterator < (UINT8*)iBlockGroup->BlockBitMap + BLOCKS_PER_BLOCK_GROUP / 8; BitmapIterator++)
            {
                for (UINT8 Mask = 0b00000001, i = 0; i < 8; Mask <<= 1, i++)
                {
                    BlockNum++;

                    //  If block is occupied
                    if(*BitmapIterator & Mask)
                        continue;

                    //  Set the block as occupied
                    *BitmapIterator |= Mask;

                    /*
                     * Write pointer to found block in INode
                     */
                    auto BlockID = ((UINT64)iBlockGroup - (UINT64)pSuperBlock)/BLOCK_SIZE + BlockNum;
                    if(BlocksAllocated < 12)
                    {
                        INode->i_block[BlocksAllocated] = BlockID;
                    } else {
                        SetTIBPEntry(INode, BlocksAllocated - 12, BlockID);
                    }

                    if(++BlocksAllocated >= Size) return;
                }
            }
        }
    }

    DIRECTORY_ENTRY *RAMDisk::GetFile(UINT8 *Path)
    {
        /*
         * The first three directory entries in a directory are occupied
         * 0: IndexFile
         * 1: .
         * 2: ..
         */
        using namespace string;

        /*
         * Start off with root dir
         */
        auto INodeIter = GetINode(2);
        DIRECTORY_ENTRY *DirEntryIter = nullptr;

        /*
         * TODO: Change this to use binary search and index
         *
         * Iterate through directories in Path string
         */
        for(UINT16 PathIndex = 0, NameLen; PathIndex < MAX_PATH && Path[PathIndex] != 0; PathIndex += NameLen + 1)
        {
            if(Path[PathIndex] == '/') PathIndex++;

            NameLen = strlen(&Path[PathIndex], '/', MAX_PATH - PathIndex);

            for(ENTRY_ID DirEntryID = 0; ; DirEntryID++)
            {
                DirEntryIter = GetINodeDirectoryEntry(INodeIter, DirEntryID);

                if(strlen(DirEntryIter->Name) == 0) return nullptr;
                if(strncmp(&Path[PathIndex], DirEntryIter->Name, NameLen))
                {
                    INodeIter = GetINode(DirEntryIter->INodeID);
                    break;
                }
            }
        }

        return DirEntryIter;;
    }

    STATUS RAMDisk::CreateFile(FILE *File)
    {
        /*
         * The first three directory entries in a directory are occupied
         * 0: IndexFile
         * 1: .
         * 2: ..
         */
        using namespace string;

        auto Path = File->Path;
        if(GetFile(File->Path) != nullptr) return STATUS_FAIL;

        /*
         * Start off with root dir
         */
        auto INodeIter = GetINode(2);
        DIRECTORY_ENTRY *DirEntryIter = nullptr;

        /*
         * TODO: Change this to use binary search and index
         *
         * Iterate through directories in Path string
         */
        for(UINT16 PathIndex = 0, NameLen; PathIndex < MAX_PATH && Path[PathIndex] != 0; PathIndex += NameLen + 1)
        {
            if(Path[PathIndex] == '/') PathIndex++;

            NameLen = strlen(&Path[PathIndex], '/', MAX_PATH - PathIndex);

            for(ENTRY_ID DirEntryID = 0; ; DirEntryID++)
            {
                DirEntryIter = GetINodeDirectoryEntry(INodeIter, DirEntryID);

                /*
                 * Found a free directory entry
                 */
                if(strlen(DirEntryIter->Name) == 0)
                {
                    strncpy(&Path[PathIndex], DirEntryIter->Name, NameLen);
                    DirEntryIter->Type = File->Type;
                    DirEntryIter->INodeID = AllocateINode();

                    return STATUS_OK;
                }

                if(strncmp(&Path[PathIndex], DirEntryIter->Name, NameLen))
                {
                    INodeIter = GetINode(DirEntryIter->INodeID);
                    break;
                }
            }
        }

        return STATUS_FAIL;
    }
}
