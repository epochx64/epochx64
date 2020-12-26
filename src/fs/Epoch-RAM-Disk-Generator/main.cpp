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

int main()
{
    using namespace std;

    auto RAMDiskBuffer = new UINT8[INITRD_SIZE_BYTES];
    ext2::RAMDisk InitRAMDisk((UINT64)RAMDiskBuffer, INITRD_SIZE_BYTES);
    {
        InitRAMDisk.MakeDir((UINT8*)"/boot");

        auto File = InitRAMDisk.GetFile((UINT8*)"/boot");
        cout << "Created directory: /" << File->Name << endl;
    }

    ReadFileToRAMDisk((TCHAR*)"Makefile", (UINT8*)"/boot/Makefile", &InitRAMDisk);

    /*
     * Create file /boot/testfile.t and write "TEST FILE" to it
     * Then read it back
     * Temporary
     */
    ext2::FILE TestFile;
    {
        TestFile.Type = FILETYPE_REG;
        ::string::strncpy((UINT8*)"/boot/Makefile", TestFile.Path, MAX_PATH);

        auto ReadFileBuf = new UINT8[InitRAMDisk.GetFileSize((UINT8*)"/boot/Makefile")];
        InitRAMDisk.ReadFile(&TestFile, ReadFileBuf);

        cout << "ReadFile data from /boot/Makefile (" << TestFile.Size << " bytes): " << ReadFileBuf << endl;
    }

    HANDLE hFile = CreateFile((TCHAR*)"initrd.ext2",
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

    if(hFile == INVALID_HANDLE_VALUE) return -1;

    DWORD BytesWritten;
    WriteFile(hFile,
            RAMDiskBuffer,
            INITRD_SIZE_BYTES,
            &BytesWritten,
            nullptr);

    cout << "Wrote initrd.ext2 (" << BytesWritten << " bytes) to disk";

    return 0;
}
