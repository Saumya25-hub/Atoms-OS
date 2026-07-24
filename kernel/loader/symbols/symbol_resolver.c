#include "symbol_resolver.h"
#include "../debug/loader_debug.h"
#include "kernel/core/lib/include/string.h"

static symbol_table_t g_global_symbol_table;

void symbol_resolver_init(void) {
    memset(&g_global_symbol_table, 0, sizeof(symbol_table_t));
    loader_debug_log(LOG_LEVEL_INFO, "SYM_RESOLVER", "Symbol Resolver Subsystem Initialized");
}

loader_status_t symbol_resolver_register(const char* name, void* address, symbol_bind_t bind, symbol_type_t type) {
    if (!name || !address) return LOADER_ERR_NULL_POINTER;
    if (g_global_symbol_table.count >= MAX_EXPORTED_SYMBOLS) return LOADER_ERR_MAX_LIBRARIES_REACHED;

    loader_symbol_t* sym = &g_global_symbol_table.symbols[g_global_symbol_table.count++];
    sym->name = name;
    sym->address = address;
    sym->size = 0;
    sym->binding = bind;
    sym->type = type;

    loader_debug_trace_symbol(name, address, true);
    return LOADER_SUCCESS;
}

void* symbol_resolver_lookup(const char* name) {
    if (!name) return NULL;

    for (uint32_t i = 0; i < g_global_symbol_table.count; i++) {
        if (strcmp(g_global_symbol_table.symbols[i].name, name) == 0) {
            loader_debug_trace_symbol(name, g_global_symbol_table.symbols[i].address, true);
            return g_global_symbol_table.symbols[i].address;
        }
    }

    loader_debug_trace_symbol(name, NULL, false);
    return NULL;
}

loader_status_t symbol_resolver_lookup_descriptor(const char* name, loader_symbol_t* out_sym) {
    if (!name || !out_sym) return LOADER_ERR_NULL_POINTER;

    for (uint32_t i = 0; i < g_global_symbol_table.count; i++) {
        if (strcmp(g_global_symbol_table.symbols[i].name, name) == 0) {
            *out_sym = g_global_symbol_table.symbols[i];
            return LOADER_SUCCESS;
        }
    }

    return LOADER_ERR_SYMBOL_NOT_FOUND;
}
