#ifndef ELFV2_H
#define ELFV2_H
#include <filesystem>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"

class ELFV2Loader {
public:
    elf_t elf;
    
    ELFV2Loader();
    int setPath(const char* path);
    bool isELF();
    bool isSELF();
    int load();
    int debugInfo();

private:
    int _elfDebugInfo();
    int _selfDebugInfo();
    int _loadHeader();
    int _loadELF();
    int _loadSELF();
    int _loadSELFsegments();
    const char* path;
    FILE* fd;
    int BSDFlagsToPOSIXFlags(uint32_t bsd_flags);
};

#endif // ELFV2_H