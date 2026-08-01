#include "../include/botree.h"
#include "../include/botree_tx.h"
#include "kernel/core/lib/include/string.h"

static char s_clipboard_paths[16][BDE_PATH_MAX];
static uint32_t s_clipboard_count = 0;
static bool s_is_cut_op = false;

int32_t BDe_ClipboardCopy(const char** paths, uint32_t count) {
    if (!paths || count == 0) return -1;
    s_clipboard_count = 0;
    s_is_cut_op = false;
    for (uint32_t i = 0; i < count && i < 16; i++) {
        strcpy(s_clipboard_paths[s_clipboard_count++], paths[i]);
    }
    return 0;
}

int32_t BDe_ClipboardCut(const char** paths, uint32_t count) {
    int32_t res = BDe_ClipboardCopy(paths, count);
    if (res == 0) s_is_cut_op = true;
    return res;
}

int32_t BDe_ClipboardPaste(const char* target_dir) {
    if (!target_dir || s_clipboard_count == 0) return -1;
    for (uint32_t i = 0; i < s_clipboard_count; i++) {
        if (s_is_cut_op) {
            BDe_TransactionMove(s_clipboard_paths[i], target_dir);
        } else {
            BDe_TransactionCopy(s_clipboard_paths[i], target_dir);
        }
    }
    if (s_is_cut_op) s_clipboard_count = 0;
    return 0;
}
