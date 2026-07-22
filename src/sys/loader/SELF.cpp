#include "SELF.h"

void SELFLoader::loadSELF(const char* path) {
    if (std::filesystem::exists(path)) {
        FILE* fd = fopen(path, "rb");
        if (fd) {
            self_t self;
            self.file.file = fd;
            if (SELFLoader::parseSELFHeader(&self) != 1) {
                LOGE("SELF", "Failed to parse SELF header: %s", path);
                return; // todo error handling
            }
            LOGD("SELF", "Ready to load SELF file: %s", std::filesystem::path(path).filename().string().c_str());
            SELFLoader::Load(&self);
            // todo append the self struct to a struct of loaded selfs
        }
    } else {
        LOGE("SELF", "Failed to open SELF file: %s", std::filesystem::path(path).filename().string().c_str());
    }
}

int SELFLoader::parseSELFHeader(self_t* self) {
    fread(&self->header, 1, sizeof(SELFHeader), self->file.file);
    if (!checkSELFmagic(self)) {
        LOGE("SELF", "Invalid SELF header");
        return -1;
    }
    return 1;
}

bool SELFLoader::checkSELFmagic(self_t* self) {
    if (self->header.magic[0] != 0x4F || self->header.magic[1] != 0x15 || self->header.magic[2] != 0x3D || self->header.magic[3] != 0x1D){
        return false;
    }
    return true;
}
void SELFLoader::selfDebugInfo(self_t* self) {
    LOGD("SELF", "SELF Header Info:");
    LOGD("SELF", "  Magic: %02x %02x %02x %02x", self->header.magic[0], self->header.magic[1], self->header.magic[2], self->header.magic[3]);
    LOGD("SELF", "  Size: %u %u", self->header.size[0], self->header.size[1]);
    LOGD("SELF", "  File Size: %lu bytes", self->header.file_size);
    LOGD("SELF", "  Segment Count: %u", self->header.segment_count);
    LOGD("SELF", "  Unk1: %u", self->header.unk1);
}

int SELFLoader::Load(self_t* self) {
    selfDebugInfo(self);
    return 1;
}