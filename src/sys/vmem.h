#ifndef VMEM_H
#define VMEM_H
#include <cstddef>
#include <cstdint>
#include <vector>
#include <stdio.h>
#include <sys/mman.h>
#include "../tools/log.h"

#define GUEST_BASE_ADDR 0x20000000000ULL
#define PS5_RAM_SIZE    (16ULL * 1024 * 1024 * 1024) // 16GB

extern uint8_t *g_vmem;

uint8_t *vmem_init();
void vmem_free(void *base);

#endif // VMEM_H