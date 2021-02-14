#ifndef RELOCATION_H
#define RELOCATION_H

#include "elf.h"
#include <kernel/typedef.h>
#include <kernel/log.h>
#include <lib/mem.h>

namespace elf
{
    inline Elf64_Shdr *GetSectionHeader(Elf64_Ehdr *ELFHeader, UINT64 Index)
    {
        return &((Elf64_Shdr*)((UINT64)ELFHeader + ELFHeader->e_shoff))[Index];
    }

    inline char *GetString(Elf64_Ehdr *ELFHeader, UINT64 Offset)
    {
        if(ELFHeader->e_shstrndx == SHN_UNDEF) return nullptr;

        auto StringTableOffset = GetSectionHeader(ELFHeader, ELFHeader->e_shstrndx)->sh_offset;
        auto StringTable = (char*)((UINT64)ELFHeader + StringTableOffset);

        return StringTable + Offset;
    }

    UINT64 GetSymbolValue(Elf64_Ehdr *ELFHeader, int Table, UINT64 Index);

    UINT64 GetProgramSize(Elf64_Shdr *ELFHeader);

    void ProcessRELA(Elf64_Ehdr *ELFHeader, Elf64_Shdr *RELASection, UINT64 pProgramStart);

    //  Returns far pointer to entry point
    void *LoadELF64(Elf64_Ehdr *ELFHeader);
}

#endif