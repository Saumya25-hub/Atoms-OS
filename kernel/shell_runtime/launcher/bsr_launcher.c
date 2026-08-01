#include "../include/bsr_api.h"
#include "kernel/botree/include/botree_path.h"
#include "kernel/core/lib/include/string.h"

int32_t BSR_Launch(const char* file_path) {
    if (!file_path || strlen(file_path) == 0) return -1;

    char ext[16];
    BDe_PathGetExtension(file_path, ext, sizeof(ext));

    const char* app_target = BSR_GetAssociation(ext);
    if (!app_target) {
        // Default to executable spawn if no explicit association
        app_target = file_path;
    }

    // Spawn process using OS Process Loader / bos_spawn
    extern uint64_t bos_spawn(const char* path);
    uint64_t pid = bos_spawn(app_target);
    return (pid != (uint64_t)-1) ? 0 : -1;
}
