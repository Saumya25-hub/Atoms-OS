#ifndef BSEC_EXPLORER_CLIPBOARD_H
#define BSEC_EXPLORER_CLIPBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define BSEC_CLIPBOARD_MAX_FILES 64

typedef enum {
    CLIPBOARD_OP_NONE = 0,
    CLIPBOARD_OP_COPY,
    CLIPBOARD_OP_CUT
} ClipboardOperation;

typedef struct {
    ClipboardOperation op;
    char               paths[BSEC_CLIPBOARD_MAX_FILES][256];
    uint32_t           count;
} ExplorerClipboard;

void explorer_clipboard_init(ExplorerClipboard* cb);
void explorer_clipboard_clear(ExplorerClipboard* cb);
bool explorer_clipboard_copy(ExplorerClipboard* cb, const char* path);
bool explorer_clipboard_cut(ExplorerClipboard* cb, const char* path);
bool explorer_clipboard_add(ExplorerClipboard* cb, const char* path, ClipboardOperation op);
bool explorer_clipboard_has_data(const ExplorerClipboard* cb);

#endif // BSEC_EXPLORER_CLIPBOARD_H
