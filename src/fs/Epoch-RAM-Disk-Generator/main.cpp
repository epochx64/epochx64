#include <iostream>
#include <windows.h>
#include <fs/ext2.h>

ext2::STATUS ReadFileToRamDisk(TCHAR *WinPath, UINT8 *RAMPath, ext2::RAMDisk *RAMDisk)
{
    OVERLAPPED ol = {0};

    HANDLE hFile = CreateFile((LPCSTR)WinPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if(hFile == INVALID_HANDLE_VALUE) return STATUS_FAIL;

    UINT64 FileSize;
    DWORD Low, High;

    Low = GetFileSize(hFile, &High);
    FileSize = Low | ((UINT64)High >> 32);

    auto readFileBuf = new UINT8[FileSize];
    ReadFileEx(hFile, (LPVOID)readFileBuf, FileSize, &ol, nullptr);

    /*
     * Now write buffer to file in RAM Disk
     */
    ext2::FILE File;

    File.Type = FILETYPE_REG;
    File.Size = FileSize;
    ::string::strncpy(RAMPath, File.Path, MAX_PATH);

    RAMDisk->WriteFile(&File, readFileBuf);

    return STATUS_OK;
}

DWORD WriteBufferToFile(TCHAR *WinPath, UINT8* Buffer, UINT64 Size)
{
    HANDLE hFile = CreateFile(WinPath,
                              GENERIC_WRITE,
                              0,
                              nullptr,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);

    if(hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD BytesWritten;
    WriteFile(hFile,
              Buffer,
              Size,
              &BytesWritten,
              nullptr);

    return BytesWritten;
}

int main()
{
    using namespace std;

    auto ramDiskBuffer = new UINT8[INITRD_SIZE_BYTES];
    ext2::RAMDisk initRamDisk((UINT64)ramDiskBuffer, INITRD_SIZE_BYTES);
    initRamDisk.MakeDir((UINT8*)"/boot");

    ReadFileToRamDisk((TCHAR*)"index", (UINT8*)"/boot/index", &initRamDisk);
    ReadFileToRamDisk((TCHAR*)"test.elf", (UINT8*)"/boot/test.elf", &initRamDisk);

    ext2::FILE testFile;
    {
        testFile.Type = FILETYPE_REG;
        ::string::strncpy((UINT8*)"/boot/index", testFile.Path, MAX_PATH);

        auto readdFileSz = initRamDisk.GetFileSize((UINT8*)"/boot/index");
        auto readFileBuf = new UINT8[readdFileSz];
        initRamDisk.ReadFile(&testFile, readFileBuf);

        WriteBufferToFile((TCHAR*)"ext2index", readFileBuf, testFile.Size);
    }

    /*
     * Write the init RAM Disk to a file
     */
    WriteBufferToFile("initrd.ext2", ramDiskBuffer, INITRD_SIZE_BYTES);

    return 0;
}
