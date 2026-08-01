#include "../include/bar_api.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    char app_name[64];
    char file_extension[16];
    bool registered;
} BARRegistryEntry;

static BARRegistryEntry g_registry[32];

int32_t bar_registry_register_assoc(const char* app, const char* ext) {
    if (!app || !ext) return -1;
    for (int i = 0; i < 32; i++) {
        if (!g_registry[i].registered) {
            strcpy(g_registry[i].app_name, app);
            strcpy(g_registry[i].file_extension, ext);
            g_registry[i].registered = true;
            return 0;
        }
    }
    return -1;
}
