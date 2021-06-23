#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>
#include <Guid/Acpi.h>
#include <stddef.h>
#include <elf.h>
#include <boot/shared_boot_defs.h>

typedef __attribute__((sysv_abi)) void (*KERNEL_ENTRY)(KE_SYS_DESCRIPTOR*);

EFI_HANDLE _ImageHandle;

EFI_GET_TIME GetTime;
EFI_SET_TIME SetTime;
EFI_FREE_POOL FreePool;
EFI_TEXT_STRING OutputString;
EFI_ALLOCATE_POOL AllocatePool;
EFI_GET_MEMORY_MAP GetMemoryMap;
EFI_ALLOCATE_PAGES AllocatePages;
EFI_HANDLE_PROTOCOL HandleProtocol;
EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;

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

UINT8 *ReadFile(UINT16* Path)
{
    EFI_STATUS Status;

    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_GUID EFILoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    HandleProtocol(_ImageHandle, &EFILoadedImageProtocolGUID, (void**)&LoadedImage);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Filesystem;
    EFI_GUID EFISimpleFilesystemProtocolGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    HandleProtocol(LoadedImage->DeviceHandle, &EFISimpleFilesystemProtocolGUID, (void**)&Filesystem);

    EFI_FILE_PROTOCOL *EFIRoot;
    Filesystem->OpenVolume(Filesystem, &EFIRoot);

    EFI_FILE_PROTOCOL *EFIFile;
    Status = EFIRoot->Open(EFIRoot, &EFIFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(Status))
    {
        OutputString(ConOut, L"Could not locate an EFI file\n\r");
        return 0;
    }

    //  GetInfo with NULL buffer sets FileInfoSize to expected size of EFI_FILE_INFO struct
    EFI_GUID EFIFileInfoGUID = EFI_FILE_INFO_ID;
    UINTN FileInfoSize = 0;
    EFIFile->GetInfo(EFIFile, &EFIFileInfoGUID, &FileInfoSize, NULL);

    //  This time, GetInfo will actually fetch the file's info
    EFI_FILE_INFO *FileInfo;
    UINT8 *FileBuf = 0;

    AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
    EFIFile->GetInfo(EFIFile, &EFIFileInfoGUID, &FileInfoSize, (void*)FileInfo);

    AllocatePool(EfiLoaderData, FileInfo->FileSize, (void**)&FileBuf);
    EFIFile->Read(EFIFile, &(FileInfo->FileSize), (void*)FileBuf);

    return FileBuf;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    KE_SYS_DESCRIPTOR KernelInfo = { 0 };

    _ImageHandle = ImageHandle;

    GetTime = SystemTable->RuntimeServices->GetTime;
    SetTime = SystemTable->RuntimeServices->SetTime;
    FreePool = SystemTable->BootServices->FreePool;
    OutputString =  SystemTable->ConOut->OutputString;
    AllocatePool = SystemTable->BootServices->AllocatePool;
    GetMemoryMap = SystemTable->BootServices->GetMemoryMap;
    AllocatePages = SystemTable->BootServices->AllocatePages;
    HandleProtocol = SystemTable->BootServices->HandleProtocol;
    SetVirtualAddressMap = SystemTable->RuntimeServices->SetVirtualAddressMap;

    ConOut = SystemTable->ConOut;

    OutputString(ConOut, L"In UEFI land\n\r");

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

        KernelInfo.gopInfo = FramebufferInfo;
    }

    /*  Get ACPI info and stuff into KERNEL_INFO */
    {
        EFI_GUID ACPIGUID = EFI_ACPI_TABLE_GUID;
        UINTN nTables = SystemTable->NumberOfTableEntries;

        for(UINTN i = 0; i < nTables; i++)
        {
            EFI_CONFIGURATION_TABLE *Config = &(SystemTable->ConfigurationTable[i]);
            if(!GuidCmp(Config->VendorGuid, ACPIGUID, sizeof(Config->VendorGuid))) continue;

            KernelInfo.pRSDP = (UINT64)Config->VendorTable;
            break;
        }
    }

    /*
     * Load kernel buffer into RAM
     * Load initrd into RAM
     */
    UINT8 *KernelBuffer = ReadFile(L"\\EFI\\epochx64.elf");
    KernelInfo.pRAMDisk = (UINT64)ReadFile(L"\\EFI\\initrd.ext2");

    /*
     * Next we parse the ELF header from the buffer,
     * Load all its sections into their corresponding memory locations.
     * We'll then grab function pointer to its entry point
     */
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
         * Zero out the BSS section
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

            for(UINT64 *i = (UINT64*)pSectionStart; (UINT64)i < pSectionStart + SectionSize; i++)
                *i = 0;

            MemSet((void*)pSectionStart, 0x00, SectionSize);
        }
    }

    /*  Get memory map, signal that we're done bootstrapping and jump to the kernel entry point */
    OutputString(SystemTable->ConOut, L"Kernel image loaded into RAM\n\r");

    UINTN *MapKey = &(KernelInfo.MapKey);
    {
        UINTN *MemoryMapSize = &(KernelInfo.MemoryMapSize);
        UINTN *DescriptorSize = &(KernelInfo.DescriptorSize);
        UINT32 *DescriptorVersion = &(KernelInfo.DescriptorVersion);

        while(1)
        {
            *MemoryMapSize = 0;
            //  GetMemoryMap with a NULL buffer will set MemoryMapSize
            GetMemoryMap(
                    MemoryMapSize,
                    NULL,
                    MapKey,
                    DescriptorSize,
                    DescriptorVersion
            );

            //  Now that we have the size, we allocate memory for it
            AllocatePool(EfiLoaderData,*MemoryMapSize + 2*(*DescriptorSize),(void**)&(KernelInfo.MemoryMap));

            //  This time it will actually give the memory map
            EFI_STATUS Result = GetMemoryMap(
                    MemoryMapSize,
                    (EFI_MEMORY_DESCRIPTOR*)(KernelInfo.MemoryMap),
                    MapKey,
                    DescriptorSize,
                    DescriptorVersion
            );

            //  Sometimes GetMemoryMap fails even with the "correct" size, so we have to loop until it works
            if(Result != EFI_BUFFER_TOO_SMALL) break;

            FreePool(KernelInfo.MemoryMap);
        }
    }

    Status = SystemTable->BootServices->ExitBootServices(ImageHandle, *MapKey);
    if(EFI_ERROR(Status)) while(1);

    /*
     * Find all free memory and map it to 0x000100000000, keeping everything else identity map
     * TODO: Find out a way to also identity map the free memory
     */
    {
        KernelInfo.pSysMemory = 0x000100000000;
        UINT64 SysMemoryIterator = KernelInfo.pSysMemory;

        for(EFI_MEM_DESCRIPTOR *MemDescriptor = KernelInfo.MemoryMap;
            (UINT64)MemDescriptor < (UINT64)KernelInfo.MemoryMap + KernelInfo.MemoryMapSize;
            MemDescriptor = (EFI_MEM_DESCRIPTOR*)((UINT64)MemDescriptor + KernelInfo.DescriptorSize))
        {
            // Memory address 0x10000 is occupied by multicore bootstrap code
            if  (MemDescriptor->Type == EfiConventionalMemory &&
                (0x10000 < SysMemoryIterator || SysMemoryIterator + MemDescriptor->NumberOfPages*0x1000 < 0x10000))
            {
                MemDescriptor->VirtualStart = SysMemoryIterator;
                MemDescriptor->Attribute = EFI_MEMORY_RUNTIME;
                MemDescriptor->Type = EfiRuntimeServicesData;
                SysMemoryIterator += MemDescriptor->NumberOfPages*0x1000;
                continue;
            }

            MemDescriptor->VirtualStart = MemDescriptor->PhysicalStart;
        }

        KernelInfo.sysMemorySize = SysMemoryIterator - KernelInfo.pSysMemory;
        KernelInfo.pSysMemoryBitMap = KernelInfo.pSysMemory;
        KernelInfo.sysMemoryBitMapSize = KernelInfo.sysMemorySize/0x1000/8 + 1;

        Status = SetVirtualAddressMap (
                KernelInfo.MemoryMapSize,
                KernelInfo.DescriptorSize,
                KernelInfo.DescriptorVersion,
                (EFI_MEMORY_DESCRIPTOR*)KernelInfo.MemoryMap
        );
        if (EFI_ERROR(Status)) while(1);
    }

    KernelEntry(&KernelInfo);

    return Status;
}
