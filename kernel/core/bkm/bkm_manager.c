#include "bkm_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static BKM_Module g_bkm_table[ATOMS_MAX_BKM_MODULES];

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATOMS_BKM_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_BKM_MODULES; i++) {
        g_bkm_table[i].name[0] = '\0';
        g_bkm_table[i].initialized = false;
    }
    bwe_log("INFO", "ATOMS BKM Kernel Module Framework Initialized");
}

bool ATOMS_BKM_RegisterModule(const char* name, const char* version, BKM_DriverClass driver_class, int (*on_init)(void), void (*on_shutdown)(void)) {
    if (!name) return false;

    for (uint32_t i = 0; i < ATOMS_MAX_BKM_MODULES; i++) {
        if (!g_bkm_table[i].initialized) {
            str_copy_limit(g_bkm_table[i].name, name, sizeof(g_bkm_table[i].name));
            str_copy_limit(g_bkm_table[i].version, version ? version : "1.0", sizeof(g_bkm_table[i].version));
            g_bkm_table[i].driver_class = driver_class;
            g_bkm_table[i].on_init = on_init;
            g_bkm_table[i].on_shutdown = on_shutdown;
            g_bkm_table[i].initialized = true;

            if (on_init) {
                on_init();
            }
            return true;
        }
    }
    return false;
}

uint32_t ATOMS_BKM_GetModuleCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_BKM_MODULES; i++) {
        if (g_bkm_table[i].initialized) count++;
    }
    return count;
}

void ATOMS_BKM_ShutdownAll(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_BKM_MODULES; i++) {
        if (g_bkm_table[i].initialized) {
            if (g_bkm_table[i].on_shutdown) {
                g_bkm_table[i].on_shutdown();
            }
            g_bkm_table[i].initialized = false;
        }
    }
}
