#include "../include/brt_api.h"

int32_t BRT_ValidateSecurityContext(uint32_t owner_pid, const char* target_path, uint32_t required_perms) {
    (void)owner_pid;
    (void)target_path;
    (void)required_perms;
    return 0; // Granted
}
