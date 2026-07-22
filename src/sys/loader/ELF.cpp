#include "ELF.h"
#include "../../tools/log.h"
#include "../vmem.h"

ELFLoader::ELFLoader() {
    this->elf = {};
    this->elf.runtime.header_read = false;
    this->elf.runtime.header_parsed = false;
    this->elf.program_headers.program_headers.clear();
    this->elf.file_offset = 0;
};

int ELFLoader::setPath(const char* path) {
    this->path = path;
    this->elf.file.file = fopen(path, "rb");
    if (!this->elf.file.file) {
        LOGE("ELF", "Failed to open file: %s", path);
        return -1;
    }
    return 0;
}

int ELFLoader::importFromSELF(self_t* self) {
    if (self->runtime.file_type != SELF_FILE) {
        LOGE("ELF", "Provided file is not a valid SELF file.");
        return -1;
    }
    LOGI("ELF", "Importing ELF from SELF");
    this->elf.file_offset = 0x1A0; // rn hardcoded, TODO Change it
    this->elf.file.file = self->file.file;
    this->elf.runtime.file_type = ELF_FILE;
    

    fseek(self->file.file,this->elf.file_offset , SEEK_SET);
    fread(&this->elf.ELF64_header, 1, sizeof(Elf64_Ehdr), self->file.file);
    this->elf.runtime.header_parsed = true;
    
    if (this->elf.ELF64_header.ident.EI_MAGIC[0] != 0x7F || this->elf.ELF64_header.ident.EI_MAGIC[1] != 'E' || this->elf.ELF64_header.ident.EI_MAGIC[2] != 'L' || this->elf.ELF64_header.ident.EI_MAGIC[3] != 'F') {
        LOGE("ELF", "Invalid ELF magic number in SELF file.");
        return -1;
    }
    // if (this->elf.ELF64_header.e_type != 2){
    //     LOGE("ELF", "Invalid ELF type in SELF file. Expected 2 (Executable), got %d", this->elf.ELF64_header.e_type);
    //     return -1;
    // }
    // TODO ADd more checks
    this->elf.runtime.header_read = true;
    
    return 0;
}

int ELFLoader::_loadHeader() { // only for when importing from file, not from SELF
    // fseek(this->elf.file.file, 0, SEEK_SET);
    // fread(&this->elf.ELF64_header, 1, sizeof(Elf64_Ehdr), this->elf.file.file);
    // return 0;
    // TODO
    return -1;
}

int ELFLoader::_loadProgramHeaders(){
    LOGD("ELF", "Loading Program Headers");
    for (size_t i = 0; i < this->elf.ELF64_header.e_phnum; i++)
    {
        Elf64_Phdr phdr;
        int offset = this->elf.ELF64_header.e_phoff + i * sizeof(Elf64_Phdr) + this->elf.file_offset;
        fseek(this->elf.file.file, offset, SEEK_SET);
        fread(&phdr, 1, sizeof(Elf64_Phdr), this->elf.file.file);
        if (this->debugEnabled == true) {
            LOGD("ELF", "Program Header %zu: Type: 0x%08x, Offset: 0x%016lx, VAddr: 0x%016lx, PAddr: 0x%016lx, FileSz: %10lu, MemSz: %08lu, Flags: 0x%08x, Align: %lu", i, phdr.p_type, phdr.p_offset, phdr.p_vaddr, phdr.p_paddr, phdr.p_filesz, phdr.p_memsz, phdr.p_flags, phdr.p_align);
        }
        this->elf.program_headers.program_headers.push_back(phdr);
    }
    return 0;
}

int ELFLoader::_loadSegments() {
    LOGD("ELF", "Loading Segments");
    for (size_t i = 0; i < this->elf.program_headers.program_headers.size(); i++)
    {
        Elf64_Phdr phdr = this->elf.program_headers.program_headers[i];
        elf_loaded_segment_t segment;
        segment.guest_vaddr = phdr.p_vaddr;
        segment.size = phdr.p_memsz;
        segment.host_ptr = g_vmem + segment.guest_vaddr;
        segment.flags = BSDFlagsToPOSIXFlags(phdr.p_flags);
        
        if (phdr.p_type == PT_LOAD) { // PT_LOAD
            if (this->debugEnabled == true) {
                LOGD("ELF", "Loading segment %zu: HAddr: 0x%016lx, Size: %lu, Flags: 0x%08x", i, segment.host_ptr, segment.size, segment.flags);
            }
            fseek(this->elf.file.file, phdr.p_offset + this->elf.file_offset, SEEK_SET);
            fread((void*)segment.host_ptr, 1, phdr.p_filesz, this->elf.file.file);
        } else if (phdr.p_type == PT_DYNAMIC) { // PT_DYNAMIC
            if (this->debugEnabled == true) {
                LOGD("ELF", "Dynamic segment found at index %zu, dylink is supported ish. but that for later..", i);
            }
        } 
    }
    return 0;
}

int ELFLoader::debugInfo(){
    if (this->elf.runtime.header_parsed && this->elf.runtime.header_read) {
        LOGD("ELF", "ELF Header Info:");
        LOGD("ELF", "  Magic: 0x%02x%02x%02x%02x", this->elf.ELF64_header.ident.EI_MAGIC[0], this->elf.ELF64_header.ident.EI_MAGIC[1], this->elf.ELF64_header.ident.EI_MAGIC[2], this->elf.ELF64_header.ident.EI_MAGIC[3]);
        LOGD("ELF", "  Entry Point: 0x%016lx", this->elf.ELF64_header.e_entry);
        LOGD("ELF", "  Program Header Offset: 0x%016lx", this->elf.ELF64_header.e_phoff);
        LOGD("ELF", "  Section Header Offset: 0x%016lx", this->elf.ELF64_header.e_shoff);
        LOGD("ELF", "  Flags: 0x%08x", this->elf.ELF64_header.e_flags);
        LOGD("ELF", "  ELF Header Size: %d bytes", this->elf.ELF64_header.e_ehsize);
        LOGD("ELF", "  Program Header Entry Size: %d bytes", this->elf.ELF64_header.e_phentsize);
        LOGD("ELF", "  Number of Program Headers: %d", this->elf.ELF64_header.e_phnum);
        LOGD("ELF", "  Section Header Entry Size: %d bytes", this->elf.ELF64_header.e_shentsize);
        LOGD("ELF", "  Number of Section Headers: %d", this->elf.ELF64_header.e_shnum);
        LOGD("ELF", "  Section Header String Table Index: %d", this->elf.ELF64_header.e_shstrndx);
    } else {
        LOGE("ELF", "Header not read or parsed. Call setPath() and ensure header is loaded first.");
        return -1;
    }
    return 0;
}

int ELFLoader::load() {
    if (!this->elf.runtime.header_read) {
        LOGE("ELF", "Header not read. Call setPath() first.");
        return -1;
    }
    if (!this->elf.runtime.header_parsed) {
        if (_loadHeader() != 0) {
            LOGE("ELF", "Failed to load header for file: %s", this->path);
            return -1;
        }
    }
    LOGI("ELF", "Loading ELF file");
    this->elf.program_headers.program_header_entry = this->elf.ELF64_header.e_phoff;
    this->elf.loaded_segments.clear();
    this->_loadProgramHeaders();
    this->_loadSegments();
    LOGI("ELF", "Loaded %zu program headers", this->elf.program_headers.program_headers.size());
    return 0;
}

int ELFLoader::BSDFlagsToPOSIXFlags(uint32_t bsd_flags) {
    int posix_flags = 0;
    if (bsd_flags & 0x1) posix_flags |= PROT_EXEC;
    if (bsd_flags & 0x2) posix_flags |= PROT_WRITE;
    if (bsd_flags & 0x4) posix_flags |= PROT_READ;
    return posix_flags;
}


SELFLoader::SELFLoader() {
    // this->self = {};
    this->self.runtime.header_read = false;
    this->self.runtime.header_parsed = false;
};

int SELFLoader::setPath(const char* path) {
    this->path = path;
    this->self.file.file = fopen(path, "rb");
    if (!this->self.file.file) {
        LOGE("SELF", "Failed to open file: %s", path);
        return -1;
    }
    this->self.runtime.header_read = true;
    return 0;
}

bool SELFLoader::isSELF() {
    if (!this->self.runtime.header_read){
        LOGE("SELF", "SELF header not read. Call setPath() first.");
        return false;
    }
    if (this->self.runtime.header_parsed){
        if (this->self.runtime.file_type == SELF_FILE) {
            return true;
        } else {
            return false;
        }
    } else {
        if (SELFLoader::_loadHeader() == 0) {
            return isSELF();
        } else {
            LOGE("SELF", "Failed to load header for file: %s", this->path);
            return false;
        }
    }
}

int SELFLoader::_loadHeader() {
    uint8_t tempBuffer[8];

    fseek(this->self.file.file, 0, SEEK_SET);
    fread(tempBuffer, 1, 8, this->self.file.file);

    // SELF magic number check
    if (
        (tempBuffer[0] == 0x4F && tempBuffer[1] == 0x15 && tempBuffer[2] == 0x3D && tempBuffer[3] == 0x1D ) || 
        (tempBuffer[0] == 0x54 && tempBuffer[1] == 0x14 && tempBuffer[2] == 0xF5 && tempBuffer[3] == 0xEE )
    ) {
        this->self.runtime.file_type = SELF_FILE;
        this->self.runtime.header_parsed = true;
        fseek(this->self.file.file, 0, SEEK_SET);
        fread(&this->self.SELF64_header, 1, sizeof(Self64_Ehdr), this->self.file.file);
        return 0;
    }

    LOGE("SELF", "SELF Header not found for file: %s", this->path);
    return -1;
}

int SELFLoader::debugInfo(){
    if (this->self.runtime.header_parsed && this->self.runtime.header_read) {
        if (this->self.runtime.file_type == SELF_FILE) {
            LOGD("SELF", "SELF Header Info:");
            LOGD("SELF", "  Magic: 0x%02x%02x%02x%02x", this->self.SELF64_header.magic[0], this->self.SELF64_header.magic[1], this->self.SELF64_header.magic[2], this->self.SELF64_header.magic[3]);
            LOGD("SELF", "  File Size: %lu bytes", this->self.SELF64_header.file_size);
            LOGD("SELF", "  Segment Count: %d", this->self.SELF64_header.segment_count);
            LOGD("SELF", "  Flags: 0x%04x", this->self.SELF64_header.flags);
    return 0;
        } else {
            LOGE("SELF", "Unknown file type for file: %s", this->path);
            return -1;
        }
    } else {
        LOGE("SELF", "Header not read or parsed. Call setPath() and ensure header is loaded first.");
        return -1;
    }
}

int SELFLoader::load() {
    if (!this->self.runtime.header_read) {
        LOGE("SELF", "Header not read. Call setPath() first.");
        return -1;
    }
    if (!this->self.runtime.header_parsed) {
        if (_loadHeader() != 0) {
            LOGE("SELF", "Failed to load header for file: %s", this->path);
            return -1;
        }
    }
    if (this->self.runtime.file_type == SELF_FILE) {
        LOGI("SELF", "Loading SELF file: %s", this->path);
        if (SELFLoader::_loadSELFsegments()) {
        }
        return -1;
    } else {
        LOGE("SELF", "Unknown file type for file: %s", this->path);
        return -1;
    }
}

int SELFLoader::_loadSELFsegments() {
    int segment_count = this->self.SELF64_header.segment_count;
    LOGI("SELF", "Loading %d SELF segments", segment_count);
    int loadsize = sizeof(Self64_segment_t) * segment_count;
    this->self.SELF64_segments.resize(segment_count);
    fseek(this->self.file.file, sizeof(Self64_Ehdr), SEEK_SET);
    fread(this->self.SELF64_segments.data(), 1, loadsize, this->self.file.file);
    bool compressed = false;
    for (int i = 0; i < segment_count; i++) {
        if (this->self.SELF64_segments[i].compressed_size != this->self.SELF64_segments[i].decompressed_size) {
            compressed = true;
        }
        if (this->debugEnabled == true) {
            LOGD("SELF", "Segment %d: Type: 0x%lx, Offset: 0x%lx, Compressed Size: %lu, Decompressed Size: %lu", i, this->self.SELF64_segments[i].type, this->self.SELF64_segments[i].offset, this->self.SELF64_segments[i].compressed_size, this->self.SELF64_segments[i].decompressed_size);    
        }
    }
    if (compressed) {
        LOGW("SELF", "Some segments are compressed. Decompression is not implemented yet.");
        // if i decide to implement it in future, it'll create a new file and dump the elf segments there, then pass it to elf loader 
        return -1;
    } else {
        LOGI("SELF", "All segments are uncompressed.");
    }
    

    // if there are multiple elfs in there then load all of them
    // rn the asumption is that there only 1 -_-    TODO: fix
    ELFLoader elfLoader;
    elfLoader.debugEnabled = this->debugEnabled;
    LOGI("SELF", "Importing ELF from SELF");
    if (elfLoader.importFromSELF(&this->self) != 0) {
        LOGE("SELF", "Failed to import ELF from SELF");
        return -1;
    }
    elfLoader.debugInfo();
    elfLoader.load();

    this->self._elfs.push_back(elfLoader.elf);
    return 0;
       
}


