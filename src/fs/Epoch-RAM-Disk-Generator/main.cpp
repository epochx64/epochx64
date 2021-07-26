#include <iostream>
#include <fs/ext2.h>
#include <stdlib.h>

ext2::STATUS ReadFileToRamDisk(const char *path, UINT8 *RAMPath, ext2::RAMDisk *RAMDisk)
{
    FILE *f = fopen(path, "rb");

    /* Get file size */
    fseek(f, 0, SEEK_END);
    UINT64 fileSize = ftell(f);
    rewind(f);

    auto readFileBuf = new UINT8[fileSize];
    fread(readFileBuf, sizeof(UINT8), fileSize, f);

    /*
     * Now write buffer to file in RAM Disk
     */
    ext2::FILE File;

    File.Type = FILETYPE_REG;
    File.Size = fileSize;
    ::string::strncpy(RAMPath, File.Path, MAX_PATH);

    RAMDisk->WriteFile(&File, readFileBuf);

    return STATUS_OK;
}

void WriteBufferToFile(const char *path, UINT8* buffer, UINT64 size)
{
    FILE *f = fopen(path, "w");
    fwrite(buffer, sizeof(UINT8), size, f);
    fclose(f);
}

int main()
{
    using namespace std;

    auto ramDiskBuffer = new UINT8[INITRD_SIZE_BYTES];
    ext2::RAMDisk initRamDisk((UINT64)ramDiskBuffer, INITRD_SIZE_BYTES);
    initRamDisk.MakeDir((UINT8*)"/boot");

    ReadFileToRamDisk("index", (UINT8*)"/boot/index", &initRamDisk);
    ReadFileToRamDisk("terminal.elf", (UINT8*)"/boot/terminal.elf", &initRamDisk);
    ReadFileToRamDisk("dwm.elf", (UINT8*)"/boot/dwm.elf", &initRamDisk);

    ext2::FILE testFile;
    {
        testFile.Type = FILETYPE_REG;
        ::string::strncpy((UINT8*)"/boot/index", testFile.Path, MAX_PATH);

        auto readdFileSz = initRamDisk.GetFileSize((UINT8*)"/boot/index");
        auto readFileBuf = new UINT8[readdFileSz];
        initRamDisk.ReadFile(&testFile, readFileBuf);

        WriteBufferToFile("ext2index", readFileBuf, testFile.Size);
    }

    /*
     * Write the init RAM Disk to a file
     */
    WriteBufferToFile("initrd.ext2", ramDiskBuffer, INITRD_SIZE_BYTES);

    return 0;
}
