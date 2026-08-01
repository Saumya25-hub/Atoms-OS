#include "../include/brt_api.h"
#include "kernel/core/lib/include/string.h"

static char s_clipboard_path[BDE_PATH_MAX];
static bool s_clipboard_is_cut = false;

int32_t BRT_SetClipboard(const char* path, bool is_cut) {
    if (!path) return -1;
    strcpy(s_clipboard_path, path);
    s_clipboard_is_cut = is_cut;
    return 0;
}

int32_t BRT_GetClipboard(char* out_path, size_t max_len, bool* out_is_cut) {
    if (!out_path || max_len == 0) return -1;
    strcpy(out_path, s_clipboard_path);
    if (out_is_cut) *out_is_cut = s_clipboard_is_cut;
    return 0;
}
