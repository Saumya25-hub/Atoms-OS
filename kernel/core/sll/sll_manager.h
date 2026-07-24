#ifndef ATOMS_SLL_MANAGER_H
#define ATOMS_SLL_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Shared Link Library (SLL) Engine (Phase 10)
// ============================================================

#define ATOMS_MAX_SLL_LIBRARIES 32
#define ATOMS_MAX_SLL_EXPORTS   128

typedef struct {
    char     symbol_name[64];
    uint64_t function_address;
} ATOMS_SLL_Export;

typedef struct {
    char             name[64];          // "window.sll", "filesystem.sll"
    char             version[16];       // "1.0.0"
    uint32_t         ref_count;         // Reference Counting
    bool             loaded;            // Single Instance Flag
    
    ATOMS_SLL_Export exports[ATOMS_MAX_SLL_EXPORTS];
    uint32_t         export_count;
    
    char             dependencies[8][64]; // Dependency Tree
    uint32_t         dependency_count;
} ATOMS_SLL_Library;

void              ATOMS_SLL_Init(void);
ATOMS_SLL_Library* ATOMS_SLL_Register(const char* name, const char* version);
bool              ATOMS_SLL_AddExport(const char* lib_name, const char* symbol, uint64_t func_ptr);
uint64_t          ATOMS_SLL_ResolveSymbol(const char* lib_name, const char* symbol);
bool              ATOMS_SLL_Load(const char* lib_name);
void              ATOMS_SLL_Unload(const char* lib_name);
bool              ATOMS_SLL_CheckCircularDependency(const char* lib_name, const char* target_dep);
uint32_t          ATOMS_SLL_GetCount(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SLL_MANAGER_H
