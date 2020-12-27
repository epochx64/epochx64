#include "ext2.h"

namespace ext2
{
    RAMDisk::RAMDisk(UINT64 pStart, UINT64 Size, bool New)
    {
        /*
         * If we are creating a new RAM Disk, zero out the disk
         */
        if(New)
        {
            for(auto i = (UINT64*)pStart; (UINT64)i < pStart + Size; i++) *i = 0;
        }

        DiskSize = Size;
        pLBA0 = pStart;

        pSuperBlock = (SUPERBLOCK*)(pLBA0 + 1024);
        pSuperBlock->s_blocks_count = Size/BLOCK_SIZE;

        BlockGroups = (BLOCK_GROUP*)pSuperBlock;
    }

    BLOCK_GROUP *RAMDisk::GetBlockGroup(UINT64 ID)
    {
        return &(BlockGroups[ID]);
    }

    INODE *RAMDisk::GetINode(INODE_ID ID)
    {
        UINT64 BlockGroupID = (ID - 1) / INODES_PER_BLOCK_GROUP;
        auto INodeTable = (INODE*)GetBlockGroup(BlockGroupID)->InodeTable;;

        UINT64 INodeIndex = (ID - 1) % INODES_PER_BLOCK_GROUP;
        return &(INodeTable[INodeIndex]);
    }

    BLOCK *RAMDisk::GetBlock(BLOCK_ID ID)
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
            BlockNum += BLOCKS_PER_BLOCK_GROUP - DATA_BLOCKS_PER_GROUP;

            /*
             * Go through the block group's block bitmap
             *
             * For each byte, look at the bits
             * 1 = occupied block
             * 0 = unoccupied block
             */

            for (auto BitmapIterator = (UINT8 *) iBlockGroup->BlockBitMap;
                 BitmapIterator < (UINT8 *) iBlockGroup->BlockBitMap + DATA_BLOCKS_PER_GROUP / 8; BitmapIterator++)
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

    STATUS RAMDisk::OccupyBlock(BLOCK_ID ID)
    {
        UINT64 BlockGroupID = (ID - 1) / BLOCKS_PER_BLOCK_GROUP;
        auto BlockGroup = GetBlockGroup(BlockGroupID);

        UINT64 BitMapIndex = (ID - 1) % BLOCKS_PER_BLOCK_GROUP;
        UINT64 ByteIndex = BitMapIndex / 8;

        //  Mark the block as occupied in the bitmap
        *(UINT8*)((UINT64)BlockGroup->BlockBitMap + ByteIndex) = 0b00000001 << (BitMapIndex  % 8);

        return STATUS_OK;
    }

    INODE_ID RAMDisk::AllocateINode()
    {
        /*
         * We start at inode ID 2 (which later gets incremented to 3 so we actually start at 3)
         * because the root dir is inode 2
         */
        INODE_ID INodeNum = 2;

        /*
         * Iterate through all the block groups
         */
        for(
                auto iBlockGroup = (BLOCK_GROUP*)pSuperBlock;
                (UINT64)iBlockGroup < (UINT64)pSuperBlock + pSuperBlock->s_blocks_count * BLOCK_SIZE;
                iBlockGroup = (BLOCK_GROUP*)((UINT64)iBlockGroup + sizeof(BLOCK_GROUP)))
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

    BLOCK_ID RAMDisk::GetTIBPEntry(INODE *INode, UINT64 Index)
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

    void RAMDisk::SetTIBPEntry(INODE *INode, UINT64 Index, BLOCK_ID Value)
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

    DIRECTORY_ENTRY *RAMDisk::GetINodeDirectoryEntry(INODE *INode, UINT64 EntryID)
    {
        UINT64 DirEntriesPerBlock = BLOCK_SIZE/sizeof(DIRECTORY_ENTRY);
        UINT64 BlockIndex = EntryID/DirEntriesPerBlock;

        if(BlockIndex < 12)
        {
            auto BlockID = INode->i_block[BlockIndex];

            if(BlockID == 0)
            {
                BlockID = AllocateBlock();
                INode->i_block[BlockIndex] = BlockID;
            }

            return &((DIRECTORY_ENTRY*)(GetBlock(BlockID)))[EntryID % DirEntriesPerBlock];
        }

        UINT64 TIPBEntry = GetTIBPEntry(INode, EntryID);

        if(TIPBEntry == 0)
        {
            TIPBEntry = AllocateBlock();
            SetTIBPEntry(INode, EntryID, TIPBEntry);
        }

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

    void RAMDisk::AllocateBlocks(INODE *INode, UINT64 Size)
    {
        UINT64 BlocksAllocated = 0;
        UINT64 BlocksRequired = Size/BLOCK_SIZE + 1;

        /*
         * Iterate through all the block groups
         */
        UINT64 BlockNum = 0;
        for (auto iBlockGroup = (BLOCK_GROUP*)pSuperBlock;
             (UINT64)iBlockGroup < (UINT64)pSuperBlock + pSuperBlock->s_blocks_count * BLOCK_SIZE;
             iBlockGroup = (BLOCK_GROUP*)((UINT64)iBlockGroup + sizeof(BLOCK_GROUP)))
        {
            BlockNum += BLOCKS_PER_BLOCK_GROUP - DATA_BLOCKS_PER_GROUP;
            /*
             * Go through the block group's block bitmap
             *
             * For each byte, look at the bits
             * 1 = occupied block
             * 0 = unoccupied block
             */
            for (auto BitmapIterator = (UINT8 *) iBlockGroup->BlockBitMap;
                 BitmapIterator < (UINT8 *) iBlockGroup->BlockBitMap + DATA_BLOCKS_PER_GROUP / 8; BitmapIterator++)
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
                    auto BlockID = BlockNum;
                    if(BlocksAllocated < 12)
                    {
                        INode->i_block[BlocksAllocated] = BlockID;
                    } else {
                        SetTIBPEntry(INode, BlocksAllocated - 12, BlockID);
                    }

                    if(++BlocksAllocated >= BlocksRequired) return;
                }
            }
        }
    }

    DIRECTORY_ENTRY *RAMDisk::GetFile(UINT8 *Path)
    {
        using namespace string;

        /*
         * Must be an absolute path
         */
        if(Path[0] != '/') return STATUS_FAIL;

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
        for(UINT16 PathIndex = 1, NameLen; PathIndex < MAX_PATH && Path[PathIndex] != 0; PathIndex += NameLen)
        {
            if(Path[PathIndex] == '/') PathIndex++;

            NameLen = strlen(&Path[PathIndex], '/', MAX_PATH - PathIndex);

            for(ENTRY_ID DirEntryID = 0; ; DirEntryID++)
            {
                DirEntryIter = GetINodeDirectoryEntry(INodeIter, DirEntryID);

                /*
                 * TODO: This is a lazy way of checking if the file can't be found
                 */
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
        /* TODO:
         * The first three directory entries in a directory are occupied
         * 0: IndexFile
         * 1: .
         * 2: ..
         */
        using namespace string;

        auto Path = File->Path;
        if(GetFile(File->Path) != nullptr) return STATUS_FAIL;

        /*
         * Must be an absolute path
         */
        if(Path[0] != '/') return STATUS_FAIL;

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
        for(UINT16 PathIndex = 1, NameLen; PathIndex < MAX_PATH && Path[PathIndex] != 0; PathIndex += NameLen + 1)
        {
            NameLen = strlen(&Path[PathIndex], '/' | 0, MAX_PATH - PathIndex);

            for(ENTRY_ID DirEntryID = 0; ; DirEntryID++)
            {
                DirEntryIter = GetINodeDirectoryEntry(INodeIter, DirEntryID);

                /*
                 * Found a free directory entry
                 * By the time we reach null entry, DirEntryIter will
                 * be the name of the file at end of path
                 * TODO: This is janky and should be changed
                 */
                if(strlen(DirEntryIter->Name) == 0)
                {
                    strncpy(&Path[PathIndex], DirEntryIter->Name, NameLen);
                    DirEntryIter->Type = File->Type;
                    DirEntryIter->Size = File->Size;
                    DirEntryIter->INodeID = AllocateINode();

                    INodeIter = GetINode(DirEntryIter->INodeID);

                    if(File->Type == FILETYPE_REG)
                    {
                        AllocateBlocks(INodeIter, File->Size);
                    }

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

    STATUS RAMDisk::ReadFile(FILE *File, UINT8 *Buffer)
    {
        auto FileDirEntry = GetFile(File->Path);
        if(FileDirEntry == nullptr) return STATUS_FAIL;

        auto FileINode = GetINode(FileDirEntry->INodeID);

        File->Size = FileINode->i_size;
        UINT64 nBlocks = FileINode->i_size/BLOCK_SIZE + 1;

        for(UINT64 iBlock = 0; iBlock < nBlocks; iBlock++)
        {
            UINT32 BlockID;

            /*
             * If the block index within the target inode is larger than
             * 12 then use the triply indirect block pointer structure
             */
            if(iBlock < 12)
            {
                BlockID = FileINode->i_block[iBlock];
            }
            else {
                BlockID = GetTIBPEntry(FileINode, iBlock - 12);
            }

            if(BlockID == 0)
            {
                BlockID = AllocateBlock();

                if(iBlock < 12) FileINode->i_block[iBlock] =  BlockID;
                else SetTIBPEntry(FileINode, iBlock - 12, BlockID);
            }

            /*
             * Copy block from buffer to filesystem
             */
            auto BlockIter = (UINT64*)GetBlock(BlockID);
            for(UINT64 j = 0; j < BLOCK_SIZE && j + iBlock*BLOCK_SIZE < FileINode->i_size; j+=8)
                 *(UINT64*)(Buffer + iBlock*BLOCK_SIZE + j) = *(BlockIter++);
        }

        return STATUS_OK;
    }

    STATUS RAMDisk::WriteFile(FILE *File, UINT8 *Buffer)
    {
        File->Type = FILETYPE_REG;

        auto FileDirEntry = GetFile(File->Path);
        if(FileDirEntry == nullptr)
        {
            CreateFile(File);
            FileDirEntry = GetFile(File->Path);
        }

        auto FileINode = GetINode(FileDirEntry->INodeID);

        FileINode->i_size = File->Size;

        UINT64 nBlocks = File->Size/BLOCK_SIZE + 1;
        for(UINT64 iBlock = 0; iBlock < nBlocks; iBlock++)
        {
            UINT32 BlockID;

            /*
             * If the block index within the target inode is larger than
             * 12 then use the triply indirect block pointer structure
             */
            if(iBlock < 12)
            {
                BlockID = FileINode->i_block[iBlock];
            }
            else {
                BlockID = GetTIBPEntry(FileINode, iBlock - 12);
            }

            if(BlockID == 0)
            {
                BlockID = AllocateBlock();

                if(iBlock < 12) FileINode->i_block[iBlock] =  BlockID;
                else SetTIBPEntry(FileINode, iBlock - 12, BlockID);
            }

            /*
             * Copy block from buffer to filesystem
             */
            auto BlockIter = (UINT64*)GetBlock(BlockID);
            for(UINT64 j = 0; j < BLOCK_SIZE && j + iBlock*BLOCK_SIZE < FileINode->i_size; j+=8)
                *(BlockIter++) = *(UINT64*)(Buffer + iBlock*BLOCK_SIZE + j);
        }

        return STATUS_OK;
    }

    STATUS RAMDisk::MakeDir(UINT8 Path[MAX_PATH])
    {
        FILE dir, *Dir = &dir;

        Dir->Type = FILETYPE_DIR;
        Dir->Size = 0;

        string::strncpy(Path, Dir->Path, string::strlen(Path));

        CreateFile(Dir);

        return STATUS_OK;
    }

    UINT64 RAMDisk::GetFileSize(UINT8 Path[MAX_PATH])
    {
        auto FileDirEntry = GetFile(Path);
        auto FileINode = GetINode(FileDirEntry->INodeID);

        return FileINode->i_size;
    }

    STATUS GetFilenameFromPath(UINT8 Path[MAX_PATH], UINT8 *Filename)
    {
        using namespace string;

        UINT16 Index = MAX_PATH;
        while(Path[Index] != '/') Index--;

        strncpy((UINT8*)&Path[Index], Filename, strlen((UINT8*)&Path[Index], 0, MAX_PATH));

        return STATUS_OK;
    }
}
