#include "explorer_clipboard.h"
#include "kernel/core/lib/include/string.h"

void explorer_clipboard_init(ExplorerClipboard* cb) {
    if (!cb) return;
    memset(cb, 0, sizeof(ExplorerClipboard));
}

void explorer_clipboard_clear(ExplorerClipboard* cb) {
    if (!cb) return;
    cb->op = CLIPBOARD_OP_NONE;
    cb->count = 0;
}

bool explorer_clipboard_add(ExplorerClipboard* cb, const char* path, ClipboardOperation op) {
    if (!cb || !path || strlen(path) == 0) return false;
    if (cb->op != op) {
        explorer_clipboard_clear(cb);
        cb->op = op;
    }
    if (cb->count < BSEC_CLIPBOARD_MAX_FILES) {
        strcpy(cb->paths[cb->count++], path);
        return true;
    }
    return false;
}

bool explorer_clipboard_copy(ExplorerClipboard* cb, const char* path) {
    return explorer_clipboard_add(cb, path, CLIPBOARD_OP_COPY);
}

bool explorer_clipboard_cut(ExplorerClipboard* cb, const char* path) {
    return explorer_clipboard_add(cb, path, CLIPBOARD_OP_CUT);
}

bool explorer_clipboard_has_data(const ExplorerClipboard* cb) {
    return cb && cb->op != CLIPBOARD_OP_NONE && cb->count > 0;
}
