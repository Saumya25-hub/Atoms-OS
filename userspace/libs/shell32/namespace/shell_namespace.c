#include "../include/shell32_api.h"

BOOL shell_namespace_resolve(const char* pidl_path, char* out_path, uint32_t max_len) {
    if (!pidl_path || !out_path || max_len == 0) return false;
    out_path[0] = '\0';
    return true;
}
