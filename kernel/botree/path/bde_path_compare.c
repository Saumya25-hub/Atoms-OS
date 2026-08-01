#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"

bool BDe_PathEquals(const char* path_a, const char* path_b) {
    if (!path_a || !path_b) return false;

    char norm_a[BDE_PATH_MAX];
    char norm_b[BDE_PATH_MAX];

    if (BDe_PathNormalize(path_a, norm_a, BDE_PATH_MAX) != 0) return false;
    if (BDe_PathNormalize(path_b, norm_b, BDE_PATH_MAX) != 0) return false;

    // Case-insensitive path comparison for Windows/BOS compatibility
    int i = 0;
    while (norm_a[i] != '\0' && norm_b[i] != '\0') {
        char ca = norm_a[i];
        char cb = norm_b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
        i++;
    }
    return (norm_a[i] == '\0' && norm_b[i] == '\0');
}
