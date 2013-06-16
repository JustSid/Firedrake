//
//  elf.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef _ELF_H_
#define _ELF_H_

#include <prefix.h>

typedef uint16_t elf32_half_t;

typedef uint32_t elf32_word_t;
typedef int32_t elf32_sword_t;

typedef uint64_t elf32_xword_t;
typedef int64_t elf32_sxword_t;

typedef uint32_t elf32_address_t;
typedef uint32_t elf32_offset_t;

typedef uint16_t elf32_index_t;
typedef uint16_t elf32_version_t;


typedef struct
{
	unsigned char   e_ident[16];    /* Magic number and other info */
	elf32_half_t    e_type;         /* Object file type */
	elf32_half_t    e_machine;      /* Architecture */
	elf32_word_t    e_version;      /* Object file version */
	elf32_address_t e_entry;        /* Entry point virtual address */
	elf32_offset_t  e_phoff;        /* Program header table file offset */
	elf32_offset_t  e_shoff;        /* Section header table file offset */
	elf32_word_t    e_flags;        /* Processor-specific flags */
	elf32_half_t    e_ehsize;       /* ELF header size in bytes */
	elf32_half_t    e_phentsize;    /* Program header table entry size */
	elf32_half_t    e_phnum;        /* Program header table entry count */
	elf32_half_t    e_shentsize;    /* Section header table entry size */
	elf32_half_t    e_shnum;        /* Section header table entry count */
	elf32_half_t    e_shstrndx;     /* Section header string table index */
} elf_header_t;

#define ELF_MAGIC "\177ELF"

/*  byte 7 of e_ident */
#define ELFOSABI_NONE       0
#define ELFOSABI_HPUX       1
#define ELFOSABI_NETBSD     2
#define ELFOSABI_LINUX      3
#define ELFOSABI_UNKNOWN4   4
#define ELFOSABI_UNKNOWN5   5
#define ELFOSABI_SOLARIS    6
#define ELFOSABI_AIX        7
#define ELFOSABI_IRIX       8
#define ELFOSABI_FREEBSD    9
#define ELFOSABI_TRU64      10
#define ELFOSABI_MODESTO    11
#define ELFOSABI_OPENBSD    12
#define ELFOSABI_OPENVMS    13
#define ELFOSABI_NSK        14
#define ELFOSABI_AROS       15
#define ELFOSABI_ARM        97
#define ELFOSABI_STANDALONE 255

/* e_type values */
#define ET_NONE     0       /* No file type */
#define ET_REL      1       /* Relocatable file */
#define ET_EXEC     2       /* Executable file */
#define ET_DYN      3       /* Shared object file */
#define ET_CORE     4       /* Core file */
#define ET_NUM      5       /* Number of defined types */
#define ET_LOOS     0xfe00      /* OS-specific range start */
#define ET_HIOS     0xfeff      /* OS-specific range end */
#define ET_LOPROC   0xff00      /* Processor-specific range start */
#define ET_HIPROC   0xffff      /* Processor-specific range end */


// Section header

typedef struct
{
	elf32_word_t    sh_name;        /* Section name (string tbl index) */
	elf32_word_t    sh_type;        /* Section type */
	elf32_word_t    sh_flags;       /* Section flags */
	elf32_address_t sh_addr;        /* Section virtual addr at execution */
	elf32_offset_t  sh_offset;      /* Section file offset */
	elf32_word_t    sh_size;        /* Section size in bytes */
	elf32_word_t    sh_link;        /* Link to another section */
	elf32_word_t    sh_info;        /* Additional section information */
	elf32_word_t    sh_addralign;   /* Section alignment */
	elf32_word_t    sh_entsize;     /* Entry size if section holds table */
} elf_section_header_t;

#define SHN_UNDEF       0       /* Undefined section */
#define SHN_LORESERVE   0xff00      /* Start of reserved indices */
#define SHN_LOPROC      0xff00      /* Start of processor-specific */
#define SHN_HIPROC      0xff1f      /* End of processor-specific */
#define SHN_LOOS        0xff20      /* Start of OS-specific */
#define SHN_HIOS        0xff3f      /* End of OS-specific */
#define SHN_ABS         0xfff1      /* Associated symbol is absolute */
#define SHN_COMMON      0xfff2      /* Associated symbol is common */
#define SHN_XINDEX      0xffff      /* Index is in extra table.  */
#define SHN_HIRESERVE   0xffff      /* End of reserved indices */

#define SHT_NULL          0     /* Section header table entry unused */
#define SHT_PROGBITS      1     /* Program data */
#define SHT_SYMTAB        2     /* Symbol table */
#define SHT_STRTAB        3     /* String table */
#define SHT_RELA          4     /* Relocation entries with addends */
#define SHT_HASH          5     /* Symbol hash table */
#define SHT_DYNAMIC       6     /* Dynamic linking information */
#define SHT_NOTE          7     /* Notes */
#define SHT_NOBITS        8     /* Program space with no data (bss) */
#define SHT_REL           9     /* Relocation entries, no addends */
#define SHT_SHLIB         10        /* Reserved */
#define SHT_DYNSYM        11        /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY    14        /* Array of constructors */
#define SHT_FINI_ARRAY    15        /* Array of destructors */
#define SHT_PREINIT_ARRAY 16        /* Array of pre-constructors */
#define SHT_GROUP         17        /* Section group */
#define SHT_SYMTAB_SHNDX  18        /* Extended section indeces */
#define SHT_NUM           19        /* Number of defined types.  */
#define SHT_LOOS          0x60000000    /* Start OS-specific */
#define SHT_GNU_LIBLIST   0x6ffffff7    /* Prelink library list */
#define SHT_CHECKSUM      0x6ffffff8    /* Checksum for DSO content.  */
#define SHT_LOSUNW        0x6ffffffa    /* Sun-specific low bound.  */
#define SHT_SUNW_move     0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef    0x6ffffffd    /* Version definition section.  */
#define SHT_GNU_verneed   0x6ffffffe    /* Version needs section.  */
#define SHT_GNU_versym    0x6fffffff    /* Version symbol table.  */
#define SHT_HISUNW        0x6fffffff    /* Sun-specific high bound.  */
#define SHT_HIOS          0x6fffffff    /* End OS-specific type */
#define SHT_LOPROC        0x70000000    /* Start of processor-specific */
#define SHT_HIPROC        0x7fffffff    /* End of processor-specific */
#define SHT_LOUSER        0x80000000    /* Start of application-specific */
#define SHT_HIUSER        0x8fffffff    /* End of application-specific */

typedef struct
{
	elf32_word_t    st_name;        /* Symbol name (string tbl index) */
	elf32_address_t st_value;       /* Symbol value */
	elf32_word_t    st_size;        /* Symbol size */
	unsigned char   st_info;        /* Symbol type and binding */
	unsigned char   st_other;       /* Symbol visibility */
	elf32_index_t   st_shndx;       /* Section index */
} elf_sym_t;


#define ELF32_ST_BIND(val)          (((uint32_t)(val)) >> 4)
#define ELF32_ST_TYPE(val)          ((val) & 0xF)
#define ELF32_ST_INFO(bind,type)    (((bind) << 4) + ((type) & 0xF))

#define STN_UNDEF   0       /* Undefined symbol index */

#define STB_LOCAL       0       /* Symbol not visible outside obj */
#define STB_GLOBAL      1       /* Symbol visible outside obj */
#define STB_WEAK        2       /* Like globals, lower precedence */
#define STB_LOOS        10      /* OS-specific semantics */
#define STB_GNU_UNIQUE  10      /* Symbol is unique in namespace */
#define STB_HIOS        12      /* OS-specific semantics */
#define STB_LOPROC      13      /* Processor-specific semantics */
#define STB_HIPROC      15      /* Processor-specific semantics */

#define STT_NOTYPE      0       /* Symbol type is unspecified */
#define STT_OBJECT      1       /* Symbol is a data object */
#define STT_FUNC        2       /* Symbol is a code object */
#define STT_SECTION     3       /* Symbol associated with a section */
#define STT_FILE        4       /* Symbol gives a file name */
#define STT_COMMON      5       /* An uninitialised common block */
#define STT_TLS         6       /* Thread local data object */
#define STT_RELC        8       /* Complex relocation expression */
#define STT_SRELC       9       /* Signed Complex relocation expression */
#define STT_LOOS        10      /* OS-specific semantics */
#define STT_GNU_IFUNC   10      /* Symbol is an indirect code object */
#define STT_HIOS        12      /* OS-specific semantics */
#define STT_LOPROC      13      /* Processor-specific semantics */
#define STT_HIPROC      15      /* Processor-specific semantics */

typedef struct
{
	elf32_address_t r_offset;       /* Address */
	elf32_word_t    r_info;         /* Relocation type and symbol index */
} elf_rel_t;

typedef struct
{
	elf32_address_t r_offset;       /* Address */
	elf32_address_t r_info;         /* Relocation type and symbol index */
	elf32_word_t    r_addend;       /* Addend */
} elf_rela_t;

#define ELF32_R_SYM(val)            ((val) >> 8)
#define ELF32_R_TYPE(val)           ((val) & 0xff)
#define ELF32_R_INFO(sym, type)     (((sym) << 8) + ((type) & 0xff))

#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC     10

// Program header

typedef struct
{
	elf32_word_t    p_type;     /* entry type */
	elf32_offset_t  p_offset;   /* file offset */
	elf32_address_t p_vaddr;    /* virtual address */
	elf32_address_t p_paddr;    /* physical address */
	elf32_word_t    p_filesz;   /* file size */
	elf32_word_t    p_memsz;    /* memory size */
	elf32_word_t    p_flags;    /* entry flags */
	elf32_word_t    p_align;    /* memory/file alignment */
} elf_program_header_t;

/* p_type */
#define PT_NULL         0       /* Program header table entry unused */
#define PT_LOAD         1       /* Loadable program segment */
#define PT_DYNAMIC      2       /* Dynamic linking information */
#define PT_INTERP       3       /* Program interpreter */
#define PT_NOTE         4       /* Auxiliary information */
#define PT_SHLIB        5       /* Reserved */
#define PT_PHDR         6       /* Entry for header table itself */
#define PT_TLS          7       /* Thread-local storage segment */
#define PT_NUM          8       /* Number of defined types */
#define PT_LOOS         0x60000000  /* Start of OS-specific */
#define PT_GNU_EH_FRAME 0x6474e550  /* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK    0x6474e551  /* Indicates stack executability */
#define PT_LOSUNW       0x6ffffffa
#define PT_SUNWBSS      0x6ffffffa  /* Sun Specific segment */
#define PT_SUNWSTACK    0x6ffffffb  /* Stack segment */
#define PT_HISUNW       0x6fffffff
#define PT_HIOS         0x6fffffff  /* End of OS-specific */
#define PT_LOPROC       0x70000000  /* Start of processor-specific */
#define PT_HIPROC       0x7fffffff  /* End of processor-specific */

/* p_flags */
#define PF_R        0x4     /* read */
#define PF_W        0x2     /* write */
#define PF_X        0x1     /* execute */



typedef struct
{
	elf32_sword_t   d_tag;          /* Dynamic entry type */
	union
	{
		elf32_word_t d_val;         /* Integer value */
		elf32_address_t d_ptr;          /* Address value */
	} d_un;
} elf_dyn_t;

#define DT_NULL             0       /* Marks end of dynamic section */
#define DT_NEEDED           1       /* Name of needed library */
#define DT_PLTRELSZ         2       /* Size in bytes of PLT relocs */
#define DT_PLTGOT           3       /* Processor defined value */
#define DT_HASH             4       /* Address of symbol hash table */
#define DT_STRTAB           5       /* Address of string table */
#define DT_SYMTAB           6       /* Address of symbol table */
#define DT_RELA             7       /* Address of Rela relocs */
#define DT_RELASZ           8       /* Total size of Rela relocs */
#define DT_RELAENT          9       /* Size of one Rela reloc */
#define DT_STRSZ            10      /* Size of string table */
#define DT_SYMENT           11      /* Size of one symbol table entry */
#define DT_INIT             12      /* Address of init function */
#define DT_FINI             13      /* Address of termination function */
#define DT_SONAME           14      /* Name of shared object */
#define DT_RPATH            15      /* Library search path (deprecated) */
#define DT_SYMBOLIC         16      /* Start symbol search here */
#define DT_REL              17      /* Address of Rel relocs */
#define DT_RELSZ            18      /* Total size of Rel relocs */
#define DT_RELENT           19      /* Size of one Rel reloc */
#define DT_PLTREL           20      /* Type of reloc in PLT */
#define DT_DEBUG            21      /* For debugging; unspecified */
#define DT_TEXTREL          22      /* Reloc might modify .text */
#define DT_JMPREL           23      /* Address of PLT relocs */
#define DT_BIND_NOW         24      /* Process relocations of object */
#define DT_INIT_ARRAY       25      /* Array with addresses of init fct */
#define DT_FINI_ARRAY       26      /* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ     27      /* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ     28      /* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH          29      /* Library search path */
#define DT_FLAGS            30      /* Flags for the object being loaded */
#define DT_ENCODING         32      /* Start of encoded range */
#define DT_PREINIT_ARRAY    32     /* Array with addresses of preinit fct*/
#define DT_PREINIT_ARRAYSZ  33       /* size in bytes of DT_PREINIT_ARRAY */
#define DT_NUM              34      /* Number used */
#define DT_LOOS             0x6000000d  /* Start of OS-specific */
#define DT_HIOS             0x6ffff000  /* End of OS-specific */
#define DT_LOPROC           0x70000000  /* Start of processor-specific */
#define DT_HIPROC           0x7fffffff  /* End of processor-specific */
#define DT_PROCNUM          DT_MIPS_NUM /* Most used by any processor */

static inline uint32_t elf_hash(const char *name)
{
	uint8_t *buffer = (uint8_t *)name;
	uint32_t h = 0;
	uint32_t g;

	while(*buffer != '\0')
	{
		uint32_t c = *buffer;
		h = (h << 4) + c;

		if((g = h & 0xf0000000) != 0) 
		{
			h ^= g;
			h ^= g >> 24;
		}

		buffer ++;
	}

	return h;
}

#endif /* _ELF_H_ */
