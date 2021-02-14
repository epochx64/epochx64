#include <iostream>
#include <fs/ext2.h>
#include <windows.h>
#include <tchar.h>

ext2::STATUS ReadFileToRAMDisk(TCHAR *WinPath, UINT8 *RAMPath, ext2::RAMDisk *RAMDisk)
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

    auto ReadFileBuf = new UINT8[FileSize];
    ReadFileEx(hFile, (LPVOID)ReadFileBuf, FileSize, &ol, nullptr);

    /*
     * Now write buffer to file in RAM Disk
     */
    ext2::FILE File;

    File.Type = FILETYPE_REG;
    File.Size = FileSize;
    ::string::strncpy(RAMPath, File.Path, MAX_PATH);

    RAMDisk->WriteFile(&File, ReadFileBuf);

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

    auto RAMDiskBuffer = new UINT8[INITRD_SIZE_BYTES];
    ext2::RAMDisk InitRAMDisk((UINT64)RAMDiskBuffer, INITRD_SIZE_BYTES);
    InitRAMDisk.MakeDir((UINT8*)"/boot");

    ReadFileToRAMDisk((TCHAR*)"index", (UINT8*)"/boot/index", &InitRAMDisk);
    ReadFileToRAMDisk((TCHAR*)"test.elf", (UINT8*)"/boot/test.elf", &InitRAMDisk);

    ext2::FILE TestFile;
    {
        TestFile.Type = FILETYPE_REG;
        ::string::strncpy((UINT8*)"/boot/index", TestFile.Path, MAX_PATH);

        auto RDFileSz = InitRAMDisk.GetFileSize((UINT8*)"/boot/index");
        auto ReadFileBuf = new UINT8[RDFileSz];
        InitRAMDisk.ReadFile(&TestFile, ReadFileBuf);

        WriteBufferToFile((TCHAR*)"ext2index", ReadFileBuf, TestFile.Size);
    }

    /*
     * Write the init RAM Disk to a file
     */
    WriteBufferToFile("initrd.ext2", RAMDiskBuffer, INITRD_SIZE_BYTES);

    return 0;
}
