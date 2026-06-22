#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#include <stdint.h>

typedef struct ProcessImage {
    uint64_t entry_point;

    uint64_t image_base;
    uint64_t image_end;
    uint64_t image_size;

    uint64_t heap_start;
    uint64_t heap_end;

    uint64_t stack_bottom;
    uint64_t stack_top;

    uint64_t user_cr3;

    uint32_t segments_loaded;
    uint32_t page_count;
    uint32_t flags;

    void* argument_block;
    void* environment_block;
} ProcessImage;

#endif // PROCESS_IMAGE_H
