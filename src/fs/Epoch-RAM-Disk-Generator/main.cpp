#include <iostream>
#include <fs/ext2.h>

int main() {
    auto RAMDiskBuffer = new UINT8[0x1000000];
    ext2::RAMDisk InitRAMDisk((UINT64)RAMDiskBuffer, 0x1000000);

    InitRAMDisk.MakeDir((UINT8*)"/boot");
    auto File = InitRAMDisk.GetFile((UINT8*)"/boot");

    std::cout << "Created directory: /" << File->Name << std::endl;

    /*
     * Create file /boot/testfile.t and write "TEST FILE" to it
     * Then read it back
     */
    ext2::FILE TestFile;
    TestFile.Type = FILETYPE_REG;
    TestFile.Size = 250;
    string::strncpy((UINT8*)"/boot/testfile.t", TestFile.Path, MAX_PATH);

    auto TestFileBuf = new UINT8[250];
    string::strncpy((UINT8*)"TEST FILE", TestFileBuf, 10);

    InitRAMDisk.WriteFile(&TestFile, TestFileBuf);
    std::cout << "Function WriteFile wrote \"" << TestFileBuf << "\" to /boot/testfile.t" << std::endl;

    auto ReadFileBuf = new UINT8[250];
    InitRAMDisk.ReadFile(&TestFile, ReadFileBuf);

    std::cout << "ReadFile data from /boot/testfile.t: " << ReadFileBuf << std::endl;

    return 0;
}
