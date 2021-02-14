#include "relocation.h"

namespace elf
{
    UINT64 GetSymbolValue(Elf64_Ehdr *ELFHeader, int Table, UINT64 Index)
    {
        using namespace log;

        auto SymbolTableHeader = GetSectionHeader(ELFHeader, Table);
        Elf64_Sym *Symbol = &((Elf64_Sym*)((UINT64)ELFHeader + SymbolTableHeader->sh_offset))[Index];

        Elf64_Shdr *StringTable = GetSectionHeader(ELFHeader, SymbolTableHeader->sh_link);
        auto Name = (char*)((UINT64)ELFHeader + StringTable->sh_offset + Symbol->st_name);

        if(Symbol->st_shndx == SHN_UNDEF)
        {
            kout << "External symbol found: " << Name << "\n";
            return 0;
        }
        else if(Symbol->st_shndx == SHN_ABS)
        {
            kout << "Absolute symbol found: " << Name << " (value 0x"<<HEX<< Symbol->st_value << ")\n";
            return Symbol->st_value;
        }

        Elf64_Shdr *Target = GetSectionHeader(ELFHeader, Symbol->st_shndx);
        UINT64 Value = (UINT64)ELFHeader + Target->sh_offset + Symbol->st_value;

        kout << "Internally defined symbol found: " << Name << "\n";
        return Value;
    }

    UINT64 GetProgramSize(Elf64_Ehdr *ELFHeader)
    {
        /*
         * All ELFs must order sections first->last .text->.data->.bss
         */

        UINT64 Size = 0;
        auto SectionHeaders = (Elf64_Shdr *)((UINT64)ELFHeader + ELFHeader->e_shoff);

        for(UINT64 i = 1; i < ELFHeader->e_shnum; i++)
        {
            Size += SectionHeaders[i].sh_size;
            auto iSectionName = GetString(ELFHeader, SectionHeaders[i].sh_name);

            if(string::strncmp((unsigned char*)iSectionName, (unsigned char*)".bss", 4))
                return SectionHeaders[i].sh_addr + SectionHeaders[i].sh_size;
        }

        return 0;
    }

    void ProcessRELA(Elf64_Ehdr *ELFHeader, Elf64_Shdr *RELASection, UINT64 pProgramStart)
    {
        auto RELAEntries = (Elf64_Rela*)((UINT64)ELFHeader + RELASection->sh_offset);
        UINT64 nEntries = RELASection->sh_size / RELASection->sh_entsize;

        auto RELALinkedSection = GetSectionHeader(ELFHeader, RELASection->sh_info);

        for(UINT64 i = 0; i < nEntries; i++)
        {
            auto RELAType = ELF64_R_TYPE(RELAEntries[i].r_info);
            auto RELALocation = (UINT64*)((UINT64)ELFHeader + RELALinkedSection->sh_offset + RELAEntries[i].r_offset);
            auto SymVal = GetSymbolValue(ELFHeader,
                    RELASection->sh_link,
                    ELF64_R_SYM(RELAEntries[i].r_info));


            log::kout << "RELALocation 0x" <<HEX<< (UINT64)RELALocation << "\n";

            if(RELAType == X86_64_64)
                *RELALocation = R_X86_64_64(SymVal, RELAEntries[i].r_addend);

            if(RELAType == X86_64_PC32)
                *(UINT32*)RELALocation = R_X86_64_PC32(SymVal, RELAEntries[i].r_addend, (UINT64)RELALocation);

            if(RELAType == X86_64_32S)
                *(UINT32*)RELALocation = R_X86_64_32S(SymVal, RELAEntries[i].r_addend);

            if(RELAType == X86_64_RELATIVE)
                *(UINT64*)RELALocation = R_X86_64_RELATIVE(pProgramStart, RELAEntries[i].r_addend);

            log::kout << "Relocation (type 0x" <<HEX<< RELAType << ") applied: 0x" <<HEX<< *RELALocation << "\n";
        }
    }

    void *LoadELF64(Elf64_Ehdr *ELFHeader)
    {
        UINT64 ProgramSize = GetProgramSize(ELFHeader);

        auto SectionHeaders = (Elf64_Shdr *)((UINT64)ELFHeader + ELFHeader->e_shoff);
        auto pProgram = SysMalloc(ProgramSize);

        //  Zero out the memory we just allocated
        for(UINT64 i = 0; i < ProgramSize; i+=8) *(UINT64*)((UINT64)pProgram + i) = 0;

        log::kout << "ELF size: 0x"<<HEX<<ProgramSize << " loaded\n";

        for(UINT64 i = 1; i < ELFHeader->e_shnum; i++)
        {
            auto iSectionName = GetString(ELFHeader, SectionHeaders[i].sh_name);

            /*
             * linker script must place .bss after all PT_LOAD segments
             */
            if(string::strncmp((unsigned char*)iSectionName, (unsigned char*)".bss", 4)) break;

            for(UINT64 j = 0; j < SectionHeaders[i].sh_size; j+=8)
            {
                *(UINT64*)((UINT64)pProgram + SectionHeaders[i].sh_addr + j)
                        = *(UINT64*)((UINT64)ELFHeader + SectionHeaders[i].sh_offset + j);
            }

            /*
             * Might be useful sometime in the future
             *
            auto iSectionType = SectionHeaders[i].sh_type;
            if(iSectionType == SHT_RELA)
            {
                ProcessRELA(ELFHeader, &SectionHeaders[i], (UINT64)pProgram);
                log::kout << "\n";
            }*/
        }

        return (void*)((UINT64)pProgram + ELFHeader->e_entry);

        /*
         * This is an alternate way of loading the elf to memory but I opted to use the janky method
         * no surprise there
         *
         * But will keep here in case of unforeseen issues caused by the jank method

        auto ProgramHeaders = (Elf64_Phdr *)((UINT64)ELFHeader + ELFHeader->e_phoff);
        for(Elf64_Phdr *iProgramHeader = ProgramHeaders;
            (UINT64)iProgramHeader < (UINT64)ProgramHeaders + ELFHeader->e_phnum*ELFHeader->e_phentsize;
            iProgramHeader = (Elf64_Phdr*)((UINT64)iProgramHeader + ELFHeader->e_phentsize))
        {
            if(iProgramHeader->p_type != PT_LOAD) continue;

            log::kout << "PROGRAMHEADER size 0x" <<HEX<< iProgramHeader->p_filesz << "\n";
            for(UINT64 j = 0; j < iProgramHeader->p_filesz; j+=8)
                *(UINT64*)((UINT64)pProgram + iProgramHeader->p_vaddr + j) = *(UINT64*)((UINT64)ELFHeader + iProgramHeader->p_offset + j);
        }
         */
    }
}