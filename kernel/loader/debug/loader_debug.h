#ifndef BOS_LOADER_DEBUG_H
#define BOS_LOADER_DEBUG_H

#include "../include/loader_types.h"

typedef enum {
    LOG_LEVEL_NONE    = 0,
    LOG_LEVEL_ERROR   = 1,
    LOG_LEVEL_WARN    = 2,
    LOG_LEVEL_INFO    = 3,
    LOG_LEVEL_VERBOSE = 4
} loader_log_level_t;

void loader_debug_init(loader_log_level_t level);
void loader_debug_set_level(loader_log_level_t level);
void loader_debug_log(loader_log_level_t level, const char* category, const char* message);
void loader_debug_trace_symbol(const char* symbol_name, void* address, bool resolved);
void loader_debug_trace_reloc(uint32_t reloc_type, uint64_t target_addr, uint64_t val);
void loader_debug_dump_memory_map(uint64_t vaddr, uint64_t size, uint32_t flags);

#endif // BOS_LOADER_DEBUG_H
