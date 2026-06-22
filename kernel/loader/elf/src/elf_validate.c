#include "kernel/loader/elf/include/elf.h"
#include "kernel/display/display.h"
#include "kernel/config/build_config.h"

bool elf_verify_header(const Elf64_Ehdr* hdr) {
    if (!hdr) {
        return false;
    }

#ifdef BOS_DEBUG
    // Silenced for cleaner output in Sprint 5
#endif

    // Check Magic
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
        hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 ||
        hdr->e_ident[EI_MAG3] != ELFMAG3) {
        display_print("[FAIL] Magic Number\n");
        return false;
    }


    // Check Class
    if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
        display_print("[FAIL] ELF64 Class\n");
        return false;
    }


    // Check Endianness
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        display_print("[FAIL] Little Endian\n");
        return false;
    }


    // Check Version
    if (hdr->e_ident[EI_VERSION] != EV_CURRENT || hdr->e_version != EV_CURRENT) {
        display_print("[FAIL] Invalid ELF version\n");
        return false;
    }

    // Check Executable
    if (hdr->e_type != ET_EXEC) {
        display_print("[FAIL] Not an executable file\n");
        return false;
    }

    // Check Machine
    if (hdr->e_machine != EM_X86_64) {
        display_print("[FAIL] Unsupported Architecture\n");
        return false;
    }

    // Check Entry point
    if (hdr->e_entry == 0) {
        display_print("[FAIL] Null entry point\n");
        return false;
    }

    return true;
}
