#ifndef SELF_H
#define SELF_H

#include <filesystem>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
#include <filesystem>
#include "../../tools/log.h"

typedef struct {
    uint8_t  magic[4];         
    uint8_t  unk0[8];          
    uint16_t size[2];
    uint64_t file_size;
    uint16_t segment_count;
    uint16_t unk1;
    uint32_t padding;
} SELFHeader;

typedef struct {
    FILE* file;
    struct stat st;
} self_file_t;

typedef struct {
    SELFHeader header;
    self_file_t file;
} self_t;

class SELFLoader {
public:
    void loadSELF(const char* path);
private:
    int parseSELFHeader(self_t* self);
    bool checkSELFmagic(self_t* self);
    void selfDebugInfo(self_t* self);
    int Load(self_t* self);
};
#endif // SELF_H