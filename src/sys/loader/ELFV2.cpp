#include "ELFV2.h"
#include "../../tools/log.h"

ELFV2Loader::ELFV2Loader() {
    this->elf = {};
    this->elf.runtime.header_read = false;
    this->elf.runtime.header_parsed = false;
};

int ELFV2Loader::setPath(const char* path) {
    this->path = path;
    this->elf.file.file = fopen(path, "rb");
    if (!this->elf.file.file) {
        LOGE("ELF", "Failed to open file: %s", path);
        return -1;
    }
    this->elf.runtime.header_read = true;
    return 0;
}

bool ELFV2Loader::isELF() {
    if (!this->elf.runtime.header_read){
        LOGE("ELF", "ELF header not read. Call setPath() first.");
        return false;
    }
    if (this->elf.runtime.header_parsed){
        if (this->elf.runtime.file_type == ELF_FILE) {
            return true;
        } else {
            return false;
        }
    } else {
        if (ELFV2Loader::_loadHeader() == 0) {
            return isELF();
        } else {
            LOGE("ELF", "Failed to load header for file: %s", this->path);
            return false;
        }
    }
}

bool ELFV2Loader::isSELF() {
    if (!this->elf.runtime.header_read){
        LOGE("ELF", "ELF header not read. Call setPath() first.");
        return false;
    }
    if (this->elf.runtime.header_parsed){
        if (this->elf.runtime.file_type == SELF_FILE) {
            return true;
        } else {
            return false;
        }
    } else {
        if (ELFV2Loader::_loadHeader() == 0) {
            return isSELF();
        } else {
            LOGE("ELF", "Failed to load header for file: %s", this->path);
            return false;
        }
    }
}

int ELFV2Loader::_loadHeader() {
    uint8_t tempBuffer[8];

    fseek(this->elf.file.file, 0, SEEK_SET);
    fread(tempBuffer, 1, 8, this->elf.file.file);

    // ELF magic number check
    if (tempBuffer[0] == 0x7F && tempBuffer[1] == 'E' && tempBuffer[2] == 'L' && tempBuffer[3] == 'F') {
        this->elf.runtime.file_type = ELF_FILE;
        this->elf.runtime.header_parsed = true;
        fseek(this->elf.file.file, 0, SEEK_SET);
        fread(&this->elf.ELF64_header, 1, sizeof(Elf64_Ehdr), this->elf.file.file);
        return 0;
    }

    // SELF magic number check
    if (tempBuffer[0] == 0x4F && tempBuffer[1] == 0x15 && tempBuffer[2] == 0x3D && tempBuffer[3] == 0x1D) {
        this->elf.runtime.file_type = SELF_FILE;
        this->elf.runtime.header_parsed = true;
        fseek(this->elf.file.file, 0, SEEK_SET);
        fread(&this->elf.SELF64_header, 1, sizeof(Self64_Ehdr), this->elf.file.file);
        return 0;
    }

    LOGE("ELF", "Unknown file type for file: %s", this->path);
    return -1;
}

int ELFV2Loader::debugInfo(){
    if (this->elf.runtime.header_parsed && this->elf.runtime.header_read) {
        if (this->elf.runtime.file_type == ELF_FILE) {
            return _elfDebugInfo();
        } else if (this->elf.runtime.file_type == SELF_FILE) {
            return _selfDebugInfo();
        } else {
            LOGE("ELF", "Unknown file type for file: %s", this->path);
            return -1;
        }
    } else {
        LOGE("ELF", "Header not read or parsed. Call setPath() and ensure header is loaded first.");
        return -1;
    }
}

int ELFV2Loader::_elfDebugInfo() {
    LOGD("ELF", "ELF Header Info:");
    LOGD("ELF", "  Entry Point: 0x%lx", this->elf.ELF64_header.e_entry);
    LOGD("ELF", "  Program Header Offset: 0x%lx", this->elf.ELF64_header.e_phoff);
    LOGD("ELF", "  Section Header Offset: 0x%lx", this->elf.ELF64_header.e_shoff);
    LOGD("ELF", "  Number of Program Headers: %d", this->elf.ELF64_header.e_phnum);
    LOGD("ELF", "  Number of Section Headers: %d", this->elf.ELF64_header.e_shnum);
    return 0;
}

int ELFV2Loader::_selfDebugInfo() {
    LOGD("ELF", "SELF Header Info:");
    LOGD("ELF", "  Magic: 0x%02x%02x%02x%02x", this->elf.SELF64_header.magic[0], this->elf.SELF64_header.magic[1], this->elf.SELF64_header.magic[2], this->elf.SELF64_header.magic[3]);
    LOGD("ELF", "  File Size: %lu bytes", this->elf.SELF64_header.file_size);
    LOGD("ELF", "  Segment Count: %d", this->elf.SELF64_header.segment_count);
    LOGD("ELF", "  Flags: 0x%04x", this->elf.SELF64_header.flags);
    return 0;
}

int ELFV2Loader::load() {
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

    if (this->elf.runtime.file_type == ELF_FILE) {
        LOGI("ELF", "Loading ELF file: %s", this->path);
        return -1;
        // return ELFV2Loader::_loadELF();
    } else if (this->elf.runtime.file_type == SELF_FILE) {
        LOGI("ELF", "Loading SELF file: %s", this->path);
        // return ELFV2Loader::_loadSELF();
        if (ELFV2Loader::_loadSELFsegments()) {
            
        }
        return -1;
    } else {
        LOGE("ELF", "Unknown file type for file: %s", this->path);
        return -1;
    }
}

int ELFV2Loader::_loadSELFsegments() {
    int segment_count = this->elf.SELF64_header.segment_count;
    LOGI("ELF", "Loading %d SELF segments", segment_count);
    int loadsize = sizeof(Self64_segment_t) * segment_count;
    fseek(this->elf.file.file, sizeof(Self64_Ehdr), SEEK_SET);
    fread(this->elf.SELF64_segments.data(), 1, loadsize, this->elf.file.file);
    if (this->debugEnabled == true) {
        for (int i = 0; i < segment_count; i++) {
            LOGD("ELF", "Segment %d: Type: 0x%lx, Offset: 0x%lx, Compressed Size: %lu, Decompressed Size: %lu", i, this->elf.SELF64_segments[i].type, this->elf.SELF64_segments[i].offset, this->elf.SELF64_segments[i].compressed_size, this->elf.SELF64_segments[i].decompressed_size);    
       }
    }
    return 0;
       
}