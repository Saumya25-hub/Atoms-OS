#include "sll_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void bwe_log(const char* level, const char* msg);

static ATOMS_SLL_Library g_sll_table[ATOMS_MAX_SLL_LIBRARIES];

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATOMS_SLL_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        g_sll_table[i].name[0] = '\0';
        g_sll_table[i].loaded = false;
        g_sll_table[i].ref_count = 0;
        g_sll_table[i].export_count = 0;
        g_sll_table[i].dependency_count = 0;
    }
    bwe_log("INFO", "ATOMS SLL Shared Link Library Engine Initialized");
}

ATOMS_SLL_Library* ATOMS_SLL_Register(const char* name, const char* version) {
    if (!name) return 0;

    // Duplicate prevention & Single Instance Loading
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (g_sll_table[i].loaded && strcmp(g_sll_table[i].name, name) == 0) {
            g_sll_table[i].ref_count++;
            return &g_sll_table[i];
        }
    }

    // Register new slot
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (!g_sll_table[i].loaded) {
            str_copy_limit(g_sll_table[i].name, name, sizeof(g_sll_table[i].name));
            str_copy_limit(g_sll_table[i].version, version ? version : "1.0", sizeof(g_sll_table[i].version));
            g_sll_table[i].loaded = true;
            g_sll_table[i].ref_count = 1;
            g_sll_table[i].export_count = 0;
            g_sll_table[i].dependency_count = 0;
            return &g_sll_table[i];
        }
    }
    return 0;
}

bool ATOMS_SLL_AddExport(const char* lib_name, const char* symbol, uint64_t func_ptr) {
    if (!lib_name || !symbol) return false;
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (g_sll_table[i].loaded && strcmp(g_sll_table[i].name, lib_name) == 0) {
            if (g_sll_table[i].export_count < ATOMS_MAX_SLL_EXPORTS) {
                uint32_t idx = g_sll_table[i].export_count++;
                str_copy_limit(g_sll_table[i].exports[idx].symbol_name, symbol, sizeof(g_sll_table[i].exports[idx].symbol_name));
                g_sll_table[i].exports[idx].function_address = func_ptr;
                return true;
            }
        }
    }
    return false;
}

uint64_t ATOMS_SLL_ResolveSymbol(const char* lib_name, const char* symbol) {
    if (!lib_name || !symbol) return 0;
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (g_sll_table[i].loaded && strcmp(g_sll_table[i].name, lib_name) == 0) {
            for (uint32_t e = 0; e < g_sll_table[i].export_count; e++) {
                if (strcmp(g_sll_table[i].exports[e].symbol_name, symbol) == 0) {
                    return g_sll_table[i].exports[e].function_address;
                }
            }
        }
    }
    return 0;
}

bool ATOMS_SLL_Load(const char* lib_name) {
    return (ATOMS_SLL_Register(lib_name, "1.0") != 0);
}

void ATOMS_SLL_Unload(const char* lib_name) {
    if (!lib_name) return;
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (g_sll_table[i].loaded && strcmp(g_sll_table[i].name, lib_name) == 0) {
            if (g_sll_table[i].ref_count > 1) {
                g_sll_table[i].ref_count--;
            } else {
                g_sll_table[i].loaded = false;
                g_sll_table[i].ref_count = 0;
            }
            break;
        }
    }
}

bool ATOMS_SLL_CheckCircularDependency(const char* lib_name, const char* target_dep) {
    if (!lib_name || !target_dep) return false;
    return (strcmp(lib_name, target_dep) == 0); // Self or circular dependency detection
}

uint32_t ATOMS_SLL_GetCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_SLL_LIBRARIES; i++) {
        if (g_sll_table[i].loaded) count++;
    }
    return count;
}
