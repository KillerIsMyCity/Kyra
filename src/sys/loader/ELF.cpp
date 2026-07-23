#include "ELF.h"
#include "../../tools/log.h"
#include "../vmem.h"
#include <string.h>
#include "dyload.h"

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
    this->elf.runtime.header_read = true;
    this->elf.file_offset = 0;
    return 0;
}

int ELFLoader::importFromSELF(self_t* self) {
    if (self->runtime.file_type != SELF_FILE) {
        LOGE("ELF", "Provided file is not a valid SELF file.");
        return -1;
    }
    LOGI("ELF", "Importing ELF from SELF");
    this->elf.file_offset = 0x1A0;// rn hardcoded, TODO Change it
    this->elf.file.file = self->file.file;
    this->elf.runtime.file_type = ELF_FILE;
    this->self_segments = self->SELF64_segments;

    fseek(self->file.file, this->elf.file_offset, SEEK_SET);
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

int ELFLoader::_loadHeader() {
    fseek(this->elf.file.file, this->elf.file_offset, SEEK_SET);
    fread(&this->elf.ELF64_header, 1, sizeof(Elf64_Ehdr), this->elf.file.file);

    if (this->elf.ELF64_header.ident.EI_MAGIC[0] != 0x7F || this->elf.ELF64_header.ident.EI_MAGIC[1] != 'E' || this->elf.ELF64_header.ident.EI_MAGIC[2] != 'L' || this->elf.ELF64_header.ident.EI_MAGIC[3] != 'F') {
        LOGE("ELF", "Invalid ELF magic number.");
        return -1;
    }

    this->elf.runtime.header_parsed = true;
    this->elf.runtime.file_type = ELF_FILE;
    return 0;
}

uint64_t ELFLoader::getEntryPoint() {
    return this->elf.ELF64_header.e_entry;
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
            LOGD("ELF", "PH[%zu] Type=0x%08x Offset=0x%016lx VAddr=0x%016lx FileSz=%lu MemSz=%lu Flags=0x%x Align=%lu",
                 i, phdr.p_type, phdr.p_offset, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz, phdr.p_flags, phdr.p_align);
        }
        this->elf.program_headers.program_headers.push_back(phdr);
    }
    return 0;
}

int ELFLoader::_loadSELFsegments() {
    LOGI("ELF", "Loading PT_LOAD segments from SELF segment table");

    std::vector<Self64_segment_t*> data_segments;
    for (auto& seg : this->self_segments) {
        if ((seg.type & 0xFF00) == 0x2800) {
            data_segments.push_back(&seg);
        }
    }
    LOGI("ELF", "Found %zu data segments in SELF", data_segments.size());

    for (size_t i = 0; i < this->elf.program_headers.program_headers.size(); i++) {
        Elf64_Phdr phdr = this->elf.program_headers.program_headers[i];

        if (phdr.p_type == PT_LOAD) {
            Self64_segment_t* self_seg = nullptr;
            for (auto* seg : data_segments) {
                if (seg->decompressed_size == phdr.p_filesz) {
                    self_seg = seg;
                    break;
                }
            }

            if (!self_seg) {
                LOGW("ELF", "No matching SELF segment for PT_LOAD %zu (filesz=%lu)", i, phdr.p_filesz);
                if (phdr.p_memsz > 0) {
                    elf_loaded_segment_t segment;
                    segment.guest_vaddr = phdr.p_vaddr;
                    segment.size = phdr.p_memsz;
                    segment.host_ptr = g_vmem + segment.guest_vaddr;
                    segment.flags = BSDFlagsToPOSIXFlags(phdr.p_flags);
                    if (phdr.p_memsz > 0) {
                        memset(segment.host_ptr, 0, phdr.p_memsz);
                    }
                    this->elf.loaded_segments.push_back(segment);
                }
                continue;
            }

            elf_loaded_segment_t segment;
            segment.guest_vaddr = phdr.p_vaddr;
            segment.size = phdr.p_memsz;
            segment.host_ptr = g_vmem + segment.guest_vaddr;
            segment.flags = BSDFlagsToPOSIXFlags(phdr.p_flags);

            LOGI("ELF", "PH[%zu] PT_LOAD: VAddr=0x%016lx -> Host=%p, FileSz=%lu, MemSz=%lu, SELF offset=0x%lx, Flags=0x%x",
                 i, segment.guest_vaddr, segment.host_ptr, phdr.p_filesz, phdr.p_memsz, self_seg->offset, segment.flags);

            fseek(this->elf.file.file, self_seg->offset, SEEK_SET);
            if (phdr.p_filesz > 0) {
                size_t read = fread((void*)segment.host_ptr, 1, phdr.p_filesz, this->elf.file.file);
                if (read != phdr.p_filesz) {
                    LOGW("ELF", "Short read: expected %lu bytes, got %lu", phdr.p_filesz, read);
                }
            }

            if (phdr.p_memsz > phdr.p_filesz) {
                size_t bss_size = phdr.p_memsz - phdr.p_filesz;
                uint8_t* bss_ptr = (uint8_t*)segment.host_ptr + phdr.p_filesz;
                memset(bss_ptr, 0, bss_size);
                LOGD("ELF", "  Zeroed BSS: %zu bytes at %p", bss_size, bss_ptr);
            }

            this->elf.loaded_segments.push_back(segment);
        }
    }
    return 0;
}

int ELFLoader::_parseDynamic() {
    for (size_t i = 0; i < this->elf.program_headers.program_headers.size(); i++) {
        Elf64_Phdr phdr = this->elf.program_headers.program_headers[i];
        if (phdr.p_type == PT_DYNAMIC) {
            LOGI("ELF", "Parsing PT_DYNAMIC at VAddr=0x%016lx, Size=%lu", phdr.p_vaddr, phdr.p_memsz);
            DynamicLoader::parseDynamicSection(&this->elf, phdr);
            return 0;
        }
    }
    LOGW("ELF", "No PT_DYNAMIC segment found");
    return 0;
}

int ELFLoader::_loadELFSegments() {
    LOGI("ELF", "Loading PT_LOAD segments from ELF file");

    for (size_t i = 0; i < this->elf.program_headers.program_headers.size(); i++) {
        Elf64_Phdr phdr = this->elf.program_headers.program_headers[i];

        if (phdr.p_type == PT_LOAD) {
            elf_loaded_segment_t segment;
            segment.guest_vaddr = phdr.p_vaddr;
            segment.size = phdr.p_memsz;
            segment.host_ptr = g_vmem + segment.guest_vaddr;
            segment.flags = BSDFlagsToPOSIXFlags(phdr.p_flags);

            LOGI("ELF", "PH[%zu] PT_LOAD: VAddr=0x%016lx -> Host=%p, FileSz=%lu, MemSz=%lu, FileOffset=0x%lx, Flags=0x%x",
                 i, segment.guest_vaddr, segment.host_ptr, phdr.p_filesz, phdr.p_memsz, phdr.p_offset, segment.flags);

            if (phdr.p_filesz > 0) {
                fseek(this->elf.file.file, phdr.p_offset + this->elf.file_offset, SEEK_SET);
                size_t read = fread((void*)segment.host_ptr, 1, phdr.p_filesz, this->elf.file.file);
                if (read != phdr.p_filesz) {
                    LOGW("ELF", "Short read: expected %lu bytes, got %lu", phdr.p_filesz, read);
                }
            }

            if (phdr.p_memsz > phdr.p_filesz) {
                size_t bss_size = phdr.p_memsz - phdr.p_filesz;
                uint8_t* bss_ptr = (uint8_t*)segment.host_ptr + phdr.p_filesz;
                memset(bss_ptr, 0, bss_size);
                LOGD("ELF", "  Zeroed BSS: %zu bytes at %p", bss_size, bss_ptr);
            }

            this->elf.loaded_segments.push_back(segment);
        }
    }
    return 0;
}

int ELFLoader::_loadSegments() {
    if (!this->self_segments.empty()) {
        return this->_loadSELFsegments();
    }
    return this->_loadELFSegments();
}

int ELFLoader::_protectSegments() {
    LOGD("ELF", "Setting segment protection flags");
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_mask = ~(page_size - 1);

    for (size_t i = 0; i < this->elf.loaded_segments.size(); i++)
    {
        elf_loaded_segment_t segment = this->elf.loaded_segments[i];
        if (segment.flags == 0) continue;
        uintptr_t aligned_addr = (uintptr_t)segment.host_ptr & page_mask;
        size_t aligned_size = (((segment.size + page_size - 1) / page_size) * page_size);

        LOGD("ELF", "mprotect(%p, 0x%lx, 0x%x)", (void*)aligned_addr, aligned_size, segment.flags);
        if (mprotect((void*)aligned_addr, aligned_size, segment.flags) != 0) {
            LOGE("ELF", "Failed to set protection flags for segment %zu: %s", i, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int ELFLoader::debugInfo(){
    if (this->elf.runtime.header_parsed && this->elf.runtime.header_read) {
        LOGI("ELF", "=== ELF Header ===");
        LOGI("ELF", "  Magic:      0x%02x%02x%02x%02x", this->elf.ELF64_header.ident.EI_MAGIC[0], this->elf.ELF64_header.ident.EI_MAGIC[1], this->elf.ELF64_header.ident.EI_MAGIC[2], this->elf.ELF64_header.ident.EI_MAGIC[3]);
        LOGI("ELF", "  Type:       0x%04x", this->elf.ELF64_header.e_type);
        LOGI("ELF", "  Machine:    0x%04x", this->elf.ELF64_header.e_machine);
        LOGI("ELF", "  Entry:      0x%016lx", this->elf.ELF64_header.e_entry);
        LOGI("ELF", "  PH Offset:  0x%016lx", this->elf.ELF64_header.e_phoff);
        LOGI("ELF", "  PH Count:   %d", this->elf.ELF64_header.e_phnum);
        LOGI("ELF", "  PH EntrySz: %d", this->elf.ELF64_header.e_phentsize);
    } else {
        LOGE("ELF", "Header not read or parsed.");
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

    if (this->_loadProgramHeaders() != 0) {
        LOGE("ELF", "Failed to load program headers");
        return -1;
    }

    if (this->_loadSegments() != 0) {
        LOGE("ELF", "Failed to load segments");
        return -1;
    }

    this->_parseDynamic();

    this->_protectSegments();

    LOGI("ELF", "Loaded %zu segments, %zu program headers",
         this->elf.loaded_segments.size(), this->elf.program_headers.program_headers.size());
    return 0;
}

int ELFLoader::BSDFlagsToPOSIXFlags(uint32_t bsd_flags) {
    int posix_flags = 0;
    if (bsd_flags & PF_X) posix_flags |= PROT_EXEC;
    if (bsd_flags & PF_W) posix_flags |= PROT_WRITE;
    if (bsd_flags & PF_R) posix_flags |= PROT_READ;
    return posix_flags;
}


SELFLoader::SELFLoader() {
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
            LOGI("SELF", "=== SELF Header ===");
            LOGI("SELF", "  Magic:         0x%02x%02x%02x%02x", this->self.SELF64_header.magic[0], this->self.SELF64_header.magic[1], this->self.SELF64_header.magic[2], this->self.SELF64_header.magic[3]);
            LOGI("SELF", "  File Size:     %lu bytes", this->self.SELF64_header.file_size);
            LOGI("SELF", "  Segment Count: %d", this->self.SELF64_header.segment_count);
            LOGI("SELF", "  Flags:         0x%04x", this->self.SELF64_header.flags);
    return 0;
        } else {
            LOGE("SELF", "Unknown file type for file: %s", this->path);
            return -1;
        }
    } else {
        LOGE("SELF", "Header not read or parsed.");
        return -1;
    }
}

int SELFLoader::load() {
    if (!this->self.runtime.header_read) {
        LOGE("SELF", "Header not read. Call setPath() first.");
        return -1;
    }
    if (!this->self.runtime.header_parsed) {
        if (this->_loadHeader() != 0) {
            LOGE("SELF", "Failed to load header for file: %s", this->path);
            return -1;
        }
    }
    if (this->self.runtime.file_type == SELF_FILE) {
        LOGI("SELF", "Loading SELF file: %s", this->path);
        if (this->_loadSELFsegments() != 0) {
            LOGE("SELF", "Failed to load SELF segments");
            return -1;
        }
        return 0;
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
        LOGD("SELF", "  Seg[%d] Type=0x%lx Offset=0x%lx Comp=%lu Decomp=%lu %s",
             i, this->self.SELF64_segments[i].type, this->self.SELF64_segments[i].offset,
             this->self.SELF64_segments[i].compressed_size, this->self.SELF64_segments[i].decompressed_size,
             ((this->self.SELF64_segments[i].type & 0xFF00) == 0x2800) ? "[DATA]" : "[META]");
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
    if (elfLoader.load() != 0) {
        LOGE("SELF", "Failed to load ELF");
        return -1;
    }

    this->self._elfs.push_back(elfLoader.elf);
    return 0;
}
