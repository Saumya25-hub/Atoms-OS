#ifndef BOS_ELF_PARSER_H
#define BOS_ELF_PARSER_H

#include "../include/loader_types.h"
#include "kernel/core/loader/elf/include/elf_types.h"

// Parsed ELF Descriptor
typedef struct {
    const Elf64_Ehdr* ehdr;
    const Elf64_Phdr* phdrs;
    uint32_t          ph_count;
    Elf64_Addr        entry_point;
    bool              is_pie;
    bool              has_dynamic;
    const Elf64_Phdr* dynamic_phdr;
} parsed_elf_t;

// Validates ELF Magic, Architecture, Endianness, Version, Alignment & Overflow
loader_status_t elf_parser_verify(const void* file_buffer, uint64_t file_size);

// Parses headers into structured parsed_elf_t descriptor
loader_status_t elf_parser_parse(const void* file_buffer, uint64_t file_size, parsed_elf_t* out_parsed);

#endif // BOS_ELF_PARSER_H
