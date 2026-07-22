#include "ELF.h"
#include "../../tools/log.h"

SELFLoader::SELFLoader() {
    this->self = {};
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
    
    return 0;
       
}