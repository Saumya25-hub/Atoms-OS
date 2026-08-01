#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"

int32_t BDe_PathGetDirname(const char* in_path, char* out_buf, size_t max_len) {
    if (!in_path || !out_buf || max_len == 0) return -1;

    char norm[BDE_PATH_MAX];
    if (BDe_PathNormalize(in_path, norm, BDE_PATH_MAX) != 0) return -1;

    if (strcmp(norm, "/") == 0) {
        strcpy(out_buf, "/");
        return 0;
    }

    int last_slash = -1;
    for (int i = 0; norm[i] != '\0'; i++) {
        if (norm[i] == '/') last_slash = i;
    }

    if (last_slash <= 0) {
        strcpy(out_buf, "/");
    } else {
        strncpy(out_buf, norm, last_slash);
        out_buf[last_slash] = '\0';
    }
    return 0;
}

int32_t BDe_PathGetBasename(const char* in_path, char* out_buf, size_t max_len) {
    if (!in_path || !out_buf || max_len == 0) return -1;

    char norm[BDE_PATH_MAX];
    if (BDe_PathNormalize(in_path, norm, BDE_PATH_MAX) != 0) return -1;

    int last_slash = -1;
    for (int i = 0; norm[i] != '\0'; i++) {
        if (norm[i] == '/') last_slash = i;
    }

    if (last_slash < 0) {
        strcpy(out_buf, norm);
    } else {
        strcpy(out_buf, &norm[last_slash + 1]);
    }
    return 0;
}

int32_t BDe_PathGetExtension(const char* in_path, char* out_buf, size_t max_len) {
    if (!in_path || !out_buf || max_len == 0) return -1;

    char base[BDE_NAME_MAX];
    if (BDe_PathGetBasename(in_path, base, BDE_NAME_MAX) != 0) return -1;

    int last_dot = -1;
    for (int i = 0; base[i] != '\0'; i++) {
        if (base[i] == '.') last_dot = i;
    }

    if (last_dot <= 0) {
        out_buf[0] = '\0';
    } else {
        strcpy(out_buf, &base[last_dot]);
    }
    return 0;
}
