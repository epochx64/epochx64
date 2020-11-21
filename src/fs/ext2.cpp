#include "ext2.h"

namespace ext2
{
    RAMDisk::RAMDisk(UINT64 pStart, UINT64 Size)
    {
        DiskSize = Size;
        pLBA0 = pStart;
        pSuperBlock = (SUPERBLOCK*)(pLBA0 + 1024);
    }
}