#include "vmem.h"

uint8_t *g_vmem = nullptr;
    
uint8_t *vmem_init()
{
    g_vmem = reinterpret_cast<uint8_t *>(mmap((void *)GUEST_BASE_ADDR,
                  PS5_RAM_SIZE,
                  PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                  -1,
                  0));

    if (g_vmem == MAP_FAILED) {
        perror("mmap");
        LOGE("VMEM", "Failed to allocate virtual memory");
        g_vmem = nullptr;
        // return nullptr;
        exit(EXIT_FAILURE);
    }

    LOGI("VMEM", "Allocated virtual memory at %p", g_vmem);
    return g_vmem;
}

void vmem_free(void *base)
{
    if (munmap(base, PS5_RAM_SIZE) < 0) {
        perror("munmap");
        LOGE("VMEM", "Failed to free virtual memory");
    } else {
        LOGI("VMEM", "Freed virtual memory at %p", base);
    }

    if (base == g_vmem)
        g_vmem = nullptr;
}