#ifndef BOS_SEGMENT_LOADER_H
#define BOS_SEGMENT_LOADER_H

#include "../include/loader_types.h"
#include "../elf/elf_parser.h"

// Loaded Segment Mapping Descriptor
typedef struct {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t size;
    uint32_t flags; // PF_R, PF_W, PF_X
} loaded_segment_t;

loader_status_t segment_loader_init(void);
loader_status_t segment_loader_map_elf(void* pml4, const parsed_elf_t* parsed, uint64_t base_load_addr, loaded_segment_t* out_segments, uint32_t max_segments, uint32_t* out_seg_count);

#endif // BOS_SEGMENT_LOADER_H
