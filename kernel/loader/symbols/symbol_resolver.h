#ifndef BOS_SYMBOL_RESOLVER_H
#define BOS_SYMBOL_RESOLVER_H

#include "../include/loader_types.h"

#define MAX_EXPORTED_SYMBOLS 1024

// Table of Exported Symbols
typedef struct {
    loader_symbol_t symbols[MAX_EXPORTED_SYMBOLS];
    uint32_t        count;
} symbol_table_t;

void            symbol_resolver_init(void);
loader_status_t symbol_resolver_register(const char* name, void* address, symbol_bind_t bind, symbol_type_t type);
void*           symbol_resolver_lookup(const char* name);
loader_status_t symbol_resolver_lookup_descriptor(const char* name, loader_symbol_t* out_sym);

#endif // BOS_SYMBOL_RESOLVER_H
