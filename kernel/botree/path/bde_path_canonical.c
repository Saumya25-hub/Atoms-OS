#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"

bool BDe_PathIsAbsolute(const char* path) {
    if (!path) return false;
    if (path[0] == '/') return true;
    if (path[0] != '\0' && path[1] == ':') return true; // Windows C: style
    return false;
}

int32_t BDe_PathJoin(const char* path_a, const char* path_b, char* out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return -1;
    if (!path_a || strlen(path_a) == 0) return BDe_PathNormalize(path_b, out_buf, max_len);
    if (!path_b || strlen(path_b) == 0) return BDe_PathNormalize(path_a, out_buf, max_len);

    if (BDe_PathIsAbsolute(path_b)) {
        return BDe_PathNormalize(path_b, out_buf, max_len);
    }

    char temp[BDE_PATH_MAX];
    size_t len_a = strlen(path_a);
    strcpy(temp, path_a);

    if (len_a > 0 && temp[len_a - 1] != '/' && temp[len_a - 1] != '\\') {
        strcat(temp, "/");
    }
    strcat(temp, path_b);

    return BDe_PathNormalize(temp, out_buf, max_len);
}

int32_t BDe_PathCanonicalize(const char* base_path, const char* rel_path, char* out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return -1;

    char joined[BDE_PATH_MAX];
    if (BDe_PathJoin(base_path, rel_path, joined, BDE_PATH_MAX) != 0) return -1;

    // Stack-based dot (.) and dot-dot (..) resolution
    char segments[32][BDE_NAME_MAX];
    int seg_count = 0;

    char* ptr = joined;
    if (*ptr == '/') ptr++;

    while (*ptr != '\0') {
        char seg[BDE_NAME_MAX];
        int idx = 0;
        while (*ptr != '\0' && *ptr != '/' && idx < BDE_NAME_MAX - 1) {
            seg[idx++] = *ptr++;
        }
        seg[idx] = '\0';

        if (*ptr == '/') ptr++;

        if (strcmp(seg, ".") == 0 || idx == 0) {
            continue;
        } else if (strcmp(seg, "..") == 0) {
            if (seg_count > 0) seg_count--;
        } else {
            if (seg_count < 32) {
                strcpy(segments[seg_count++], seg);
            }
        }
    }

    // Reconstruct path
    out_buf[0] = '\0';
    if (seg_count == 0) {
        strcpy(out_buf, "/");
        return 0;
    }

    for (int i = 0; i < seg_count; i++) {
        strcat(out_buf, "/");
        strcat(out_buf, segments[i]);
    }
    return 0;
}
