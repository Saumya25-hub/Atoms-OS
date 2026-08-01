#include "../include/botree.h"
#include "kernel/core/lib/include/string.h"

int32_t BDe_ShortcutResolve(const char* link_path, char* out_target, size_t max_len) {
    if (!link_path || !out_target || max_len == 0) return -1;
    strcpy(out_target, link_path);
    return 0;
}
