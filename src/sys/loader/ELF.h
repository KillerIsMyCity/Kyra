#ifndef SELF_H
#define SELF_H
#include <filesystem>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"

class SELFLoader {
public:
    self_t self;
    bool debugEnabled = false;
    
    SELFLoader();
    int setPath(const char* path);
    bool isSELF();
    int load();
    int debugInfo();

private:
    int _loadHeader();
    int _loadSELFsegments();
    const char* path;
    int BSDFlagsToPOSIXFlags(uint32_t bsd_flags);
};


class ELFLoader {
public:
    elf_t elf;
    bool debugEnabled = false;
    ELFLoader();
    int setPath(const char* path);
    int importFromSELF(self_t* self);
    int load();
private:
    int _loadHeader();
    int _loadProgramHeaders();
    int _loadSegments();
    const char* path;
};

#endif // SELF_H