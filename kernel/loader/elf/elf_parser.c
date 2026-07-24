#include "elf_parser.h"
#include "../debug/loader_debug.h"

loader_status_t elf_parser_verify(const void* file_buffer, uint64_t file_size) {
    if (!file_buffer || file_size < sizeof(Elf64_Ehdr)) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "File buffer null or too small for ELF Header");
        return LOADER_ERR_NULL_POINTER;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)file_buffer;

    // 1. Verify Magic Number
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "Invalid ELF Magic Number");
        return LOADER_ERR_INVALID_MAGIC;
    }

    // 2. Verify Architecture (Must be 64-bit ELF)
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "Non-64-bit ELF class unsupported");
        return LOADER_ERR_INVALID_ARCH;
    }

    // 3. Verify Endianness (Must be Little-Endian)
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "Big-Endian ELF unsupported");
        return LOADER_ERR_INVALID_ENDIAN;
    }

    // 4. Verify ELF Machine (X86_64)
    if (ehdr->e_machine != EM_X86_64) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "Invalid ELF Machine target (expected X86_64)");
        return LOADER_ERR_INVALID_ARCH;
    }

    // 5. Verify Program Headers Offset & Overflow
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0 ||
        (ehdr->e_phoff + (uint64_t)ehdr->e_phnum * sizeof(Elf64_Phdr) > file_size)) {
        loader_debug_log(LOG_LEVEL_ERROR, "ELF_PARSER", "Program headers exceed file size or corrupted");
        return LOADER_ERR_CORRUPT_SEGMENT;
    }

    return LOADER_SUCCESS;
}

loader_status_t elf_parser_parse(const void* file_buffer, uint64_t file_size, parsed_elf_t* out_parsed) {
    if (!out_parsed) return LOADER_ERR_NULL_POINTER;

    loader_status_t status = elf_parser_verify(file_buffer, file_size);
    if (status != LOADER_SUCCESS) return status;

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)file_buffer;
    out_parsed->ehdr = ehdr;
    out_parsed->phdrs = (const Elf64_Phdr*)((const uint8_t*)file_buffer + ehdr->e_phoff);
    out_parsed->ph_count = ehdr->e_phnum;
    out_parsed->entry_point = ehdr->e_entry;
    out_parsed->is_pie = (ehdr->e_type == ET_DYN);
    out_parsed->has_dynamic = false;
    out_parsed->dynamic_phdr = NULL;

    for (uint32_t i = 0; i < out_parsed->ph_count; i++) {
        if (out_parsed->phdrs[i].p_type == PT_DYNAMIC) {
            out_parsed->has_dynamic = true;
            out_parsed->dynamic_phdr = &out_parsed->phdrs[i];
            break;
        }
    }

    loader_debug_log(LOG_LEVEL_INFO, "ELF_PARSER", "ELF Header successfully verified and parsed");
    return LOADER_SUCCESS;
}
