#ifndef ELF_COMMON_H
#define ELF_COMMON_H

#include <cstdint>
#include <vector>
#include <string>
#include <sys/stat.h>

// ----- GNU ELF Header structure ------
typedef enum {
    EI_CLASS_INVALID = 0,
    EI_CLASS_32 = 1,
    EI_CLASS_64 = 2
} EI_CLASS_t;

typedef enum {
    EI_DATA_INVALID = 0,
    EI_DATA_LSB = 1,
    EI_DATA_MSB = 2
} EI_DATA_t;

typedef enum {
    PT_LOAD          = 1,
    PT_DYNAMIC       = 2,
    PT_TLS           = 7,
    PT_OS_DYNLIBDATA = 0x61000000, // Sony bullshit up ahead (ithink)
    PT_OS_PROCPARAMS = 0x61000001,
    PT_OS_RELRO      = 0x61000002,
    PT_UNK1          = 0x6474E550,
    PT_UNK2          = 0x6474E552,
    PT_SCE_RELA         = 0x70000000, 
    PT_SCE_DYNSYM       = 0x70000001,
    PT_SCE_PROPARAM     = 0x70000002,
    PT_SCE_MODULE_PARAM = 0x70000003,
    PT_SCE_COMMENT      = 0x70000004
} Elf64_PType; // BSD Program block(section? range? idk) type

typedef struct {
    uint8_t EI_MAGIC[4];
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSION;
    uint8_t EI_OSABI;
    uint8_t EI_ABIVERSION;
    uint8_t EI_PAD[7]; // 
} GNU_ELF_t;

// typedef struct {
#define PF_X 0x1 // Execute
#define PF_W 0x2 // Write
#define PF_R 0x4 // Read
// } GNU_ELF_PFlags_t;

typedef struct {
    GNU_ELF_t ident;
    uint16_t  e_type; 
    uint16_t  e_machine;
    uint32_t  e_version;
    uint64_t  e_entry; // Entry point address
    uint64_t   e_phoff; // Program header table offset
    uint64_t   e_shoff; // Section header table offset
    uint32_t  e_flags; 
    uint16_t  e_ehsize; // ELF header size
    uint16_t  e_phentsize; // Program header entry size
    uint16_t  e_phnum; // Number of program headers
    uint16_t  e_shentsize; // Section header entry size
    uint16_t  e_shnum; // Number of section headers
    uint16_t  e_shstrndx; // Section header string table index
} Elf64_Ehdr; // Full BSD ELF Header structure

typedef struct {
    uint32_t   p_type;
    uint32_t   p_flags;
    uint64_t   p_offset;
    uint64_t   p_vaddr;
    uint64_t   p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
} Elf64_Phdr; // BSD Program header structure

// ----- SELF Header structure -----
typedef struct {
    uint8_t  magic[4];         
    uint8_t  unk0[8];          
    uint16_t size[2];
    uint64_t file_size;
    uint16_t segment_count;
    uint16_t flags;
    uint32_t padding;
} Self64_Ehdr; // Sony ELF Header structure


typedef struct {
	uint64_t type;
	uint64_t offset;
	uint64_t compressed_size;
	uint64_t decompressed_size;
} Self64_segment_t; 

// ----- ELF Loader -----

typedef enum {
    ELF_SUCCESS = 0,
    ELF_ERROR_INVALID_HEADER = -1,
    ELF_ERROR_FILE_NOT_FOUND = -2,
    ELF_ERROR_FILE_OPEN_FAILED = -3,
    ELF_ERROR_FILE_STAT_FAILED = -4,
    ELF_ERROR_MMAP_FAILED = -5
} elf_error_t;

typedef enum {
    ELF_GNU = 0
} elf_type_t;

typedef struct {
    FILE* file;
    struct stat st;
} elf_file_t;

typedef struct {
    void* mmap_base;
    size_t size;
} elf_ptr_t;

typedef enum {
    ELF_FILE,
    SELF_FILE,
    UNKNOWN_FILE
} elf_file_type_t;
    

typedef struct {
    elf_type_t type;
    int uuid;
    bool header_read;
    bool header_parsed;
    elf_file_type_t file_type;
} elf_runtime_t;

typedef struct {
    uint64_t program_header_entry;
    Elf64_Phdr* program_headers;
} elf_program_header_t;

typedef struct {
    uintptr_t guest_vaddr;  // The PS5 virtual address
    void* host_ptr;         // Pointer to the mapped memory in the host process
    size_t size;            // Size in memory
    int flags;              // POSIX protection flags (convertor exists in ELFLoader)
} elf_loaded_segment_t;

typedef struct {
    Elf64_Ehdr ELF64_header; // The ELF header
    Self64_Ehdr SELF64_header; // The SELF header (Fucking Sony)
    std::vector<Self64_segment_t> SELF64_segments; 
    elf_program_header_t program_headers;
    elf_file_t file; // ELF file descriptor 
    elf_ptr_t elf_ptr; // Pointer to the mapped ELF file in memory
    elf_runtime_t runtime;  // Runtime information about the ELF (not used byt the translation layer)
    std::vector<elf_loaded_segment_t> loaded_segments; 
    std::vector<std::string> dependencies; // List of shared library dependencies
} elf_t;

typedef struct {
    int64_t d_tag;     // Dynamic entry type
    union {
        uint64_t d_val; // Integer value
        uint64_t d_ptr; // Address value
    } d_un;
} Elf64_Dyn;


#endif // ELF_COMMON_H