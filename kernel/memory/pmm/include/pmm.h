#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/boot/include/boot_info.h"

#define PAGE_SIZE 4096

void pmm_init(boot_info_t* boot_info);
void* pmm_alloc_page();
void* pmm_alloc_pages(size_t count);
void pmm_free_page(void* phys_addr);
void pmm_free_pages(void* phys_addr, size_t count);

uint64_t pmm_get_total_memory();
uint64_t pmm_get_free_memory();
uint64_t pmm_get_used_memory();

void* pmm_get_bitmap_address();
uint64_t pmm_get_bitmap_size();
uint64_t pmm_get_total_frames();

#endif // PMM_H
