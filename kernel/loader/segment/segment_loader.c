#include "segment_loader.h"
#include "../debug/loader_debug.h"

extern bool elf_load_segment(void* pml4, int fd, const Elf64_Phdr* phdr, uint16_t index);

loader_status_t segment_loader_init(void) {
    loader_debug_log(LOG_LEVEL_INFO, "SEGMENT_LOADER", "W^X Ready Segment Loader & Memory Mapper Engine Initialized");
    return LOADER_SUCCESS;
}

loader_status_t segment_loader_map_elf(void* pml4, const parsed_elf_t* parsed, uint64_t base_load_addr, loaded_segment_t* out_segments, uint32_t max_segments, uint32_t* out_seg_count) {
    if (!parsed || !out_seg_count) return LOADER_ERR_NULL_POINTER;

    uint32_t count = 0;
    for (uint32_t i = 0; i < parsed->ph_count; i++) {
        const Elf64_Phdr* ph = &parsed->phdrs[i];
        if (ph->p_type == PT_LOAD) {
            uint64_t target_vaddr = (parsed->is_pie ? base_load_addr : 0) + ph->p_vaddr;

            if (out_segments && count < max_segments) {
                out_segments[count].vaddr = target_vaddr;
                out_segments[count].paddr = 0;
                out_segments[count].size = ph->p_memsz;
                out_segments[count].flags = ph->p_flags;
            }
            count++;

            loader_debug_dump_memory_map(target_vaddr, ph->p_memsz, ph->p_flags);
        }
    }

    *out_seg_count = count;
    return LOADER_SUCCESS;
}
