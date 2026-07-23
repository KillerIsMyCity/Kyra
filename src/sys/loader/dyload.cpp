#include "dyload.h"
#include "../vmem.h"

// AI was used during the writing of this

int DynamicLoader::parseDynamicSection(elf_t* elf, Elf64_Phdr dynamic_header) {


    uintptr_t host_dynamic_segment_addr = (uintptr_t)(g_vmem + dynamic_header.p_vaddr);
    Elf64_Dyn* dyn_table = reinterpret_cast<Elf64_Dyn*>(host_dynamic_segment_addr);

    uintptr_t strtab_vaddr = 0;
    size_t strtab_size = 0;
    std::vector<uint64_t> needed_lib_offsets;
    LOGD("DYLINK", "Parsing dynamic section at virtual address: 0x%lx", host_dynamic_segment_addr);

    size_t count = dynamic_header.p_filesz / sizeof(Elf64_Dyn);
    LOGD("DYLINK", "Dynamic Section contains %zu entries", count);

    bool found_null = false;
    for (size_t i = 0; i < count; i++)
    {
        Elf64_Dyn dyn = dyn_table[i];
        LOGD("DYLINK", "  Dynamic Entry %zu: Tag: 0x%lx, Value: 0x%lx", i, dyn.d_tag, dyn.d_un.d_val);
        if (dyn.d_tag == DT_NULL) {
            found_null = true;
            break;
        }
        switch (dyn.d_tag) {
            case DT_NEEDED:
                needed_lib_offsets.push_back(dyn.d_un.d_val);
                break;
            case DT_STRTAB:
                strtab_vaddr = dyn.d_un.d_ptr;
                break;
            case DT_STRSZ:
                strtab_size = dyn.d_un.d_val;
                break;
            case DT_SYMTAB:
                LOGD("DYLINK", "  Symbol Table Virtual Address: 0x%lx", dyn.d_un.d_ptr);
                break;
            case DT_PLTGOT:
                LOGD("DYLINK", "  Global Offset Table Address: 0x%lx", dyn.d_un.d_ptr);
                break;
            default:
                LOGW("DYLINK", "  Unhandled Dynamic Tag: 0x%lx", dyn.d_tag);
                break;
        }
    }
    if (!found_null) {
        LOGW("DYLINK", "  Warning: DT_NULL not found in dynamic section, potential malformed ELF.");
    }



    // Second Pass: Resolve shared dependencies from the string table
    if (strtab_vaddr != 0 && !needed_lib_offsets.empty()) {
        
        uintptr_t host_strtab_addr = GUEST_BASE_ADDR + strtab_vaddr;
        const char* strtab = reinterpret_cast<const char*>(host_strtab_addr);

        LOGI("DYLINK", "================= DEPENDENCY RESOLUTION =================");
        for (uint64_t offset : needed_lib_offsets) {
            if (offset < strtab_size) {
                std::string lib_name = &strtab[offset];
                LOGI("DYLINK", "  Requires Shared Library (SPRX): %s", lib_name.c_str());
                elf->dependencies.push_back(lib_name);
            } else {
                LOGE("DYLINK", "  Encountered invalid DT_NEEDED offset: %lu", offset);
            }
        }
        LOGI("DYLINK", "=========================================================");   
    } else {
        LOGW("DYLINK", "No string table or needed libraries found in the dynamic section.");
    }
    return 1;
}