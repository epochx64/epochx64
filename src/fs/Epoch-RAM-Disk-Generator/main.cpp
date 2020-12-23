#include <iostream>
#include <fs/ext2.h>

int main() {
    auto RAMDiskBuffer = new UINT8[0x1000000];
    std::cout << "Hello, World!" << std::endl;

    ext2::RAMDisk InitRAMDisk((UINT64)RAMDiskBuffer, 0x1000000);
    InitRAMDisk.MakeDir((UINT8*)"/boot");

    auto File = InitRAMDisk.GetFile((UINT8*)"/boot");
    std::cout << "Init RAMDisk Zeroed " << File->Name << std::endl;

    return 0;
}
