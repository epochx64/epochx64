#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>

typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint64_t	Elf64_Off;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Xword;

#define EI_NIDENT  16
#define PT_LOAD    1

#define	SHN_UNDEF   0
#define	SHN_ABS	    0xfff1		/* Absolute values. */
#define	SHT_SYMTAB  2	/* symbol table section */
#define SHT_NOBITS  0x8
#define	SHT_REL	    9           /* relocation section - no addends */
#define	SHT_RELA	4	        /* relocation section with addends */

#define ELF64_R_SYM(INFO)	((INFO) >> 32)
#define ELF64_R_TYPE(INFO)	((INFO) & 0xFFFFFFFFL)

#define X86_64_64 1
#define X86_64_PC32 2
#define X86_64_32S 0x0B
#define X86_64_RELATIVE 0x08

#define R_X86_64_64(S, A) ((S) + (A))
#define R_X86_64_PC32(S, A, P) ((S) + (A) - (P))
#define R_X86_64_32S(S, A) ((S) + (A))
#define R_X86_64_RELATIVE(B, A) ((B) + (A))

#define R_386_NONE 0 // No relocation
#define R_386_32   1 // Symbol + Offset
#define R_386_PC32 2 //Symbol + Offset - Section Offset

typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint64_t	Elf64_Off;
typedef int32_t		Elf64_Sword;
typedef int64_t		Elf64_Sxword;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Lword;
typedef uint64_t	Elf64_Xword;

/*
 * The ELF64 header at the beginning of ELF kernel
 */
typedef struct elf64_hdr {
    unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;		/* Entry point virtual address */
    Elf64_Off e_phoff;		/* Program header table file offset */
    Elf64_Off e_shoff;		/* Section header table file offset */
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/*
 * Program header
 */
typedef struct elf64_phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;		/* Segment file offset */
    Elf64_Addr p_vaddr;		/* Segment virtual address */
    Elf64_Addr p_paddr;		/* Segment physical address */
    Elf64_Xword p_filesz;		/* Segment size in file */
    Elf64_Xword p_memsz;		/* Segment size in memory */
    Elf64_Xword p_align;		/* Segment alignment, file & memory */
} Elf64_Phdr;


typedef struct elf64_shdr {
    Elf64_Word sh_name;		/* Section name, index in string tbl */
    Elf64_Word sh_type;		/* Type of section */
    Elf64_Xword sh_flags;		/* Miscellaneous section attributes */
    Elf64_Addr sh_addr;		/* Section virtual addr at execution */
    Elf64_Off sh_offset;		/* Section file offset */
    Elf64_Xword sh_size;		/* Size of section in bytes */
    Elf64_Word sh_link;		/* Index of another section */
    Elf64_Word sh_info;		/* Additional section information */
    Elf64_Xword sh_addralign;	/* Section alignment */
    Elf64_Xword sh_entsize;	/* Entry size if section holds table */
} Elf64_Shdr;

/* Relocations that don't need an addend field. */
typedef struct {
    Elf64_Addr	r_offset;	/* Location to be relocated. */
    Elf64_Xword	r_info;		/* Relocation type and symbol index. */
} Elf64_Rel;

/* Relocations that need an addend field. */
typedef struct {
    Elf64_Addr	r_offset;	/* Location to be relocated. */
    Elf64_Xword	r_info;		/* Relocation type and symbol index. */
    Elf64_Sxword	r_addend;	/* Addend. */
} Elf64_Rela;

typedef struct {
    Elf64_Word	st_name;	/* String table index of name. */
    unsigned char	st_info;	/* Type and binding information. */
    unsigned char	st_other;	/* Reserved (not used). */
    Elf64_Half	st_shndx;	/* Section index of symbol. */
    Elf64_Addr	st_value;	/* Symbol value. */
    Elf64_Xword	st_size;	/* Size of associated object. */
} Elf64_Sym;


#endif