#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <stddef.h>
#include <elf.h>
#include <defs/boot.h>

typedef __attribute__((sysv_abi)) void (*KERNEL_ENTRY)(KERNEL_BOOT_INFO);

UINT64 StrLen(char *str)
{
    UINT64 result = 0;

    //  This will break when it gets to the null terminator
    while(str[result++]);

    //  Because it will increment one too many times
    return result - 1;
}

void MemNCopy(void *src, void *dst, UINT64 count)
{
    for(UINT64 i = 0; i < count; i++) ((UINT8*)dst)[i] = ((UINT8*)src)[i];
}

void MemSet(void *dst, UINT8 val, UINT64 count)
{
    for(UINT64 i = 0; i < count; i++) ((UINT8*)dst)[i] = val;
}

int GuidCmp(EFI_GUID GuidA, EFI_GUID GuidB, UINT64 len)
{
    UINT8 *A = (UINT8*)&GuidA;
    UINT8 *B = (UINT8*)&GuidB;

    for(UINT64 i = 0; i < len; i++)
    {
        if(A[i] != B[i]) return 0;
    }

    return 1;
}

void to_hex(UINT64 num, char* buf)
{
    UINT8 hex_size = sizeof(num) * 2;

    for ( uint8_t i = 0; i < hex_size; i++ )
    {
        UINT8 c = (UINT8)(num >> (4*i)) & 0x0F;

        //  ASCII conversion; capital
        if ( c > 9 ) c += 7;
        c += '0';

        buf[hex_size - (i+1)] = c;
    }

    //  Null terminate
    buf[hex_size] = 0;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;

    EFI_TEXT_STRING OutputString =  SystemTable->ConOut->OutputString;
    EFI_ALLOCATE_POOL AllocatePool = SystemTable->BootServices->AllocatePool;
    EFI_GET_MEMORY_MAP GetMemoryMap = SystemTable->BootServices->GetMemoryMap;
    EFI_ALLOCATE_PAGES AllocatePages = SystemTable->BootServices->AllocatePages;
    EFI_HANDLE_PROTOCOL HandleProtocol = SystemTable->BootServices->HandleProtocol;

    OutputString(SystemTable->ConOut, L"IN UEFI LAND\n\r");

    /*  Get memory map information  */
    KERNEL_BOOT_INFO KernelInfo;
    {
        UINTN *MapKey = &(KernelInfo.MapKey);
        UINTN *MemoryMapSize = &(KernelInfo.MemoryMapSize);
        UINTN *DescriptorSize = &(KernelInfo.DescriptorSize);
        UINT32 *DescriptorVersion = &(KernelInfo.DescriptorVersion);

        //  GetMemoryMap without a NULL buffer will set MemoryMapSize
        GetMemoryMap(MemoryMapSize, NULL, MapKey, DescriptorSize, DescriptorVersion);
        AllocatePool(EfiLoaderData, *MemoryMapSize, (void**)&(KernelInfo.MemoryMap));

        GetMemoryMap(MemoryMapSize, (EFI_MEMORY_DESCRIPTOR*)(KernelInfo.MemoryMap),
                MapKey, DescriptorSize, DescriptorVersion);
    }

    /*  Get framebuffer info and set video mode */
    FRAMEBUFFER_INFO FramebufferInfo;
    {
        EFI_GUID GOP_GUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        UINTN nHandles = 0;
        EFI_HANDLE *HandleBuffer;
        SystemTable->BootServices->LocateHandleBuffer(ByProtocol, &GOP_GUID, NULL, &nHandles, &HandleBuffer);

        EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPProtocol;
        HandleProtocol(HandleBuffer[0], &GOP_GUID, (void**)&GOPProtocol);

        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GraphicsOutputInfo;
        UINTN InfoSize;
        Status = GOPProtocol->QueryMode
                (
                GOPProtocol,
                GOPProtocol->Mode==NULL?0:GOPProtocol->Mode->Mode,
                &InfoSize,
                &GraphicsOutputInfo
                );
        if(Status == EFI_NOT_STARTED) GOPProtocol->SetMode(GOPProtocol, 0);

        UINT32 NativeMode = GOPProtocol->Mode->Mode;
        UINT32 nModes = GOPProtocol->Mode->MaxMode;

        UINT32 ChosenOne = 0;
        UINT32 HighestX = 0, HighestY = 0;
        for(UINT32 i = 0; i < nModes; i++)
        {
            GOPProtocol->QueryMode(GOPProtocol, i, &InfoSize, &GraphicsOutputInfo);

            if(GraphicsOutputInfo->HorizontalResolution > HighestX || GraphicsOutputInfo->VerticalResolution > HighestY)
            {
                HighestX = GraphicsOutputInfo->HorizontalResolution;
                HighestY = GraphicsOutputInfo->VerticalResolution;
                ChosenOne = i;
            }
        }

        //  On newer hardware, this doesn't need to execute
        if(NativeMode != ChosenOne) GOPProtocol->SetMode(GOPProtocol, ChosenOne);

        FramebufferInfo.pFrameBuffer    = GOPProtocol->Mode->FrameBufferBase;
        FramebufferInfo.Bits            = FRAMEBUFFER_BYTES_PER_PIXEL;
        FramebufferInfo.Height          = GOPProtocol->Mode->Info->VerticalResolution;
        FramebufferInfo.Width           = GOPProtocol->Mode->Info->HorizontalResolution;
        FramebufferInfo.Pitch           = GOPProtocol->Mode->Info->PixelsPerScanLine*4;

        KernelInfo.FramebufferInfo = FramebufferInfo;
    }

    /*  Get ACPI info and stuff into KERNEL_INFO */
    {
        EFI_GUID ACPIGUID = EFI_ACPI_TABLE_GUID;
        UINTN nTables = SystemTable->NumberOfTableEntries;

        for(UINTN i = 0; i < nTables; i++)
        {
            EFI_CONFIGURATION_TABLE *Config = &(SystemTable->ConfigurationTable[i]);
            if(!GuidCmp(Config->VendorGuid, ACPIGUID, sizeof(Config->VendorGuid))) continue;

            KernelInfo.RSDP = (UINT64)Config->VendorTable;
            break;
        }
    }

    /*
     * Get a handle to the kernel bin and load the
     * whole thing into a buffer
     */
    UINT8 *KernelBuffer;
    {
        EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
        EFI_GUID EFILoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        HandleProtocol(ImageHandle, &EFILoadedImageProtocolGUID, (void**)&LoadedImage);

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Filesystem;
        EFI_GUID EFISimpleFilesystemProtocolGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
        HandleProtocol(LoadedImage->DeviceHandle, &EFISimpleFilesystemProtocolGUID, (void**)&Filesystem);

        EFI_FILE_PROTOCOL *EFIRoot;
        Filesystem->OpenVolume(Filesystem, &EFIRoot);

        EFI_FILE_PROTOCOL *KernelBin;
        Status = EFIRoot->Open(EFIRoot, &KernelBin, L"\\EFI\\epochx64.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
        if (EFI_ERROR(Status))
        {
            OutputString(SystemTable->ConOut, L"Could not locate kernel ELF: \\EFI\\epochx64.bin\n\r");
            return Status;
        }

        //  GetInfo with NULL buffer sets FileInfoSize to expected size of EFI_FILE_INFO struct
        EFI_GUID EFIFileInfoGUID = EFI_FILE_INFO_ID;
        UINTN FileInfoSize = 0;
        KernelBin->GetInfo(KernelBin, &EFIFileInfoGUID, &FileInfoSize, NULL);

        //  This time, GetInfo will actually fetch the file's info
        EFI_FILE_INFO *FileInfo;
        AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
        KernelBin->GetInfo(KernelBin, &EFIFileInfoGUID, &FileInfoSize, (void*)FileInfo);

        AllocatePool(EfiLoaderData, FileInfo->FileSize, (void**)&KernelBuffer);
        KernelBin->Read(KernelBin, &(FileInfo->FileSize), (void*)KernelBuffer);
    }

    /*
     * - Next we parse the ELF header from the buffer,
     * - Load all its sections into their corresponding memory locations.
     * - We'll then grab function pointer to its entry point
     */
    OutputString(SystemTable->ConOut, L"KERNEL IMAGE LOADED\n\r");

    KERNEL_ENTRY KernelEntry;
    {
        Elf64_Ehdr *EHdr = (Elf64_Ehdr *)KernelBuffer;
        KernelEntry = (KERNEL_ENTRY)(EHdr->e_entry);

        UINT64 pProgramHeaders = (UINT64)EHdr + EHdr->e_phoff;
        for
                (
                Elf64_Phdr *iProgramHeader = (Elf64_Phdr *)pProgramHeaders;
                (UINT64)iProgramHeader < pProgramHeaders + EHdr->e_phnum*EHdr->e_phentsize;
                iProgramHeader = (Elf64_Phdr *)((UINT64)iProgramHeader + EHdr->e_phentsize)
                )
        {
            UINT64 SegmentSize = iProgramHeader->p_memsz;
            UINT64 pSegmentStart = iProgramHeader->p_paddr;
            UINT64 nPages = 1 + (SegmentSize / 0x1000);

            AllocatePages(AllocateAddress, EfiLoaderData, nPages, &pSegmentStart);
            if(iProgramHeader->p_type != PT_LOAD) continue;

            UINT64 pSegmentData = (UINT64)KernelBuffer + iProgramHeader->p_offset;
            MemNCopy ((void *)pSegmentData, (void *)pSegmentStart, SegmentSize);
        }

        /*
         * If BSS section location in memory is not zeroed, static and global
         * variables do not initialize properly. If they are initialized with 0,
         * for some reason they are filled with garbage data. Any nonzero value works
         * which is bizarre
         */
        UINT64 pSectionHeaders = (UINT64)EHdr + EHdr->e_shoff;
        for
                (
                Elf64_Shdr *iSectionHeader = (Elf64_Shdr *)pSectionHeaders;
                (UINT64)iSectionHeader < pSectionHeaders + EHdr->e_shnum*EHdr->e_shentsize;
                iSectionHeader = (Elf64_Shdr *)((UINT64)iSectionHeader + EHdr->e_shentsize)
                )
        {
            if(iSectionHeader->sh_type != SHT_NOBITS) continue;

            UINT64 SectionSize = iSectionHeader->sh_size;
            UINT64 pSectionStart = iSectionHeader->sh_addr;

            UINT64 nPages = 1 + (SectionSize / 0x1000);
            AllocatePages(AllocateAddress, EfiLoaderData, nPages, &pSectionStart);

            MemSet((void*)pSectionStart, 0x00, SectionSize);
        }
    }

    OutputString(SystemTable->ConOut, L"KERNEL SECTIONS IN RAM\n\r");

    /*  Signal that we're done bootstrapping and jump to the kernel entry point */
    OutputString(SystemTable->ConOut, L"CALLING KERNEL\n\r");
    SystemTable->BootServices->ExitBootServices(ImageHandle, KernelInfo.MapKey);
    KernelEntry(KernelInfo);

    return Status;
}
