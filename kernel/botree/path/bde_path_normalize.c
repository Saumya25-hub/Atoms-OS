#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"

int32_t BDe_PathNormalize(const char* in_path, char* out_buf, size_t max_len) {
    if (!in_path || !out_buf || max_len == 0) return -1;

    size_t in_len = strlen(in_path);
    if (in_len == 0) {
        if (max_len < 2) return -1;
        out_buf[0] = '/';
        out_buf[1] = '\0';
        return 0;
    }

    size_t out_idx = 0;
    bool prev_was_slash = false;

    for (size_t i = 0; i < in_len && out_idx < max_len - 1; i++) {
        char c = in_path[i];

        // Replace backslashes with standard POSIX slashes
        if (c == '\\') c = '/';

        if (c == '/') {
            if (!prev_was_slash) {
                out_buf[out_idx++] = '/';
                prev_was_slash = true;
            }
        } else {
            out_buf[out_idx++] = c;
            prev_was_slash = false;
        }
    }

    // Strip trailing slash unless it is the root "/"
    if (out_idx > 1 && out_buf[out_idx - 1] == '/') {
        out_idx--;
    }

    out_buf[out_idx] = '\0';
    return 0;
}
