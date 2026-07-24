#ifndef BOS_RELOC_ENGINE_H
#define BOS_RELOC_ENGINE_H

#include "../include/loader_types.h"

// X86_64 Relocation Types
#define R_X86_64_NONE      0
#define R_X86_64_64        1
#define R_X86_64_PC32      2
#define R_X86_64_GOT32     3
#define R_X86_64_PLT32     4
#define R_X86_64_COPY      5
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE  8
#define R_X86_64_GOTPCREL  9

// Standalone Table-Driven Relocation Handler Signature
typedef loader_status_t (*reloc_handler_fn)(uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr);

// Reloc Engine Subsystem Interface
void            reloc_engine_init(void);
loader_status_t reloc_engine_process_entry(uint32_t reloc_type, uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr);

#endif // BOS_RELOC_ENGINE_H
