#include "reloc_engine.h"
#include "../debug/loader_debug.h"

static loader_status_t reloc_handle_64(uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr) {
    (void)base_addr;
    uint64_t* location = (uint64_t*)target_addr;
    *location = symbol_val + addend;
    loader_debug_trace_reloc(R_X86_64_64, target_addr, *location);
    return LOADER_SUCCESS;
}

static loader_status_t reloc_handle_glob_dat(uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr) {
    (void)base_addr;
    uint64_t* location = (uint64_t*)target_addr;
    *location = symbol_val + addend;
    loader_debug_trace_reloc(R_X86_64_GLOB_DAT, target_addr, *location);
    return LOADER_SUCCESS;
}

static loader_status_t reloc_handle_jump_slot(uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr) {
    (void)base_addr;
    uint64_t* location = (uint64_t*)target_addr;
    *location = symbol_val + addend;
    loader_debug_trace_reloc(R_X86_64_JUMP_SLOT, target_addr, *location);
    return LOADER_SUCCESS;
}

static loader_status_t reloc_handle_relative(uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr) {
    (void)symbol_val;
    uint64_t* location = (uint64_t*)target_addr;
    *location = base_addr + addend;
    loader_debug_trace_reloc(R_X86_64_RELATIVE, target_addr, *location);
    return LOADER_SUCCESS;
}

// Table-Driven Relocation Dispatch Table
#define MAX_RELOC_TYPES 16
static reloc_handler_fn g_reloc_dispatch_table[MAX_RELOC_TYPES];

void reloc_engine_init(void) {
    for (uint32_t i = 0; i < MAX_RELOC_TYPES; i++) {
        g_reloc_dispatch_table[i] = NULL;
    }

    g_reloc_dispatch_table[R_X86_64_64]        = reloc_handle_64;
    g_reloc_dispatch_table[R_X86_64_GLOB_DAT]  = reloc_handle_glob_dat;
    g_reloc_dispatch_table[R_X86_64_JUMP_SLOT] = reloc_handle_jump_slot;
    g_reloc_dispatch_table[R_X86_64_RELATIVE]  = reloc_handle_relative;

    loader_debug_log(LOG_LEVEL_INFO, "RELOC_ENGINE", "Table-Driven Relocation Engine Initialized");
}

loader_status_t reloc_engine_process_entry(uint32_t reloc_type, uint64_t target_addr, uint64_t symbol_val, int64_t addend, uint64_t base_addr) {
    if (reloc_type >= MAX_RELOC_TYPES || !g_reloc_dispatch_table[reloc_type]) {
        loader_debug_log(LOG_LEVEL_ERROR, "RELOC_ENGINE", "Unsupported Relocation Type");
        return LOADER_ERR_UNSUPPORTED_RELOC;
    }

    return g_reloc_dispatch_table[reloc_type](target_addr, symbol_val, addend, base_addr);
}
