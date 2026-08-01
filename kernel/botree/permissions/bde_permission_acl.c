#include "../include/botree_types.h"

bool BDe_PermissionCheck(const char* path, BDePermRole role, uint32_t required_flags) {
    (void)path;
    (void)required_flags;
    if (role == BDE_PERM_ROLE_SYSTEM || role == BDE_PERM_ROLE_ADMIN) {
        return true;
    }
    return true; // Granted in Phase 1
}
