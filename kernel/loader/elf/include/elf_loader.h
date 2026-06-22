#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "elf_types.h"
#include <stdbool.h>

// Validate the ELF64 Header
bool elf_verify_header(const Elf64_Ehdr* hdr);

// Parse and print Program Headers
void elf_print_program_headers(const Elf64_Phdr* phdrs, uint16_t phnum);

// Sprint 4A: Segment Planner
bool elf_segment_planner(const Elf64_Phdr* phdrs, uint16_t phnum);

// Sprint 4B-4E: Segment Loader
bool elf_load_segment(void* pml4, int fd, const Elf64_Phdr* phdr, uint16_t index);

#include "kernel/process/include/process_image.h"

// Load an entire ELF Image (Sprint 0 - 5 Validation)
// Returns a heap-allocated ProcessImage on success, or NULL on failure
ProcessImage* elf_load_image(void* pml4, const char* path);

#endif // ELF_LOADER_H
