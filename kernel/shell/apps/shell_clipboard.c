// ============================================================
// ATOMS OS — Shell Clipboard Engine (Cut, Copy, Paste)
// ============================================================

#include "shell_clipboard.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

static char s_clipboard_path[256] = {0};
static bool s_clipboard_is_cut = false;

extern void Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

void Shell_ClipboardCut(const char* src_path) {
    if (!src_path || src_path[0] == '\0') return;
    strcpy(s_clipboard_path, src_path);
    s_clipboard_is_cut = true;
    Shell_ShowNotification("Clipboard", "Cut item to clipboard", 3000);
}

void Shell_ClipboardCopy(const char* src_path) {
    if (!src_path || src_path[0] == '\0') return;
    strcpy(s_clipboard_path, src_path);
    s_clipboard_is_cut = false;
    Shell_ShowNotification("Clipboard", "Copied item to clipboard", 3000);
}

const char* Shell_ClipboardGetPath(void) {
    return s_clipboard_path;
}

bool Shell_ClipboardIsCut(void) {
    return s_clipboard_is_cut;
}

bool Shell_ClipboardPaste(const char* dest_dir) {
    if (s_clipboard_path[0] == '\0' || !dest_dir) {
        Shell_ShowNotification("Paste", "Clipboard is empty", 3000);
        return false;
    }

    // Extract filename from source path
    const char* filename = s_clipboard_path;
    const char* p = s_clipboard_path;
    while (*p) { if (*p == '/' || *p == '\\') filename = p + 1; p++; }

    char dest_path[256];
    if (strcmp(dest_dir, "/") == 0) {
        strcpy(dest_path, "/");
        strcat(dest_path, filename);
    } else {
        strcpy(dest_path, dest_dir);
        strcat(dest_path, "/");
        strcat(dest_path, filename);
    }

    if (s_clipboard_is_cut) {
        // Cut -> Move
        if (vfs_rename(s_clipboard_path, dest_path) == 0) {
            s_clipboard_path[0] = '\0';
            s_clipboard_is_cut = false;
            Shell_ShowNotification("Paste", "Moved item successfully!", 3000);
            return true;
        } else {
            Shell_ShowNotification("Paste", "Failed to move item", 3000);
            return false;
        }
    } else {
        // Copy -> Read source & write dest
        int in_fd = vfs_open(s_clipboard_path);
        if (in_fd < 0) {
            Shell_ShowNotification("Paste Error", "Failed to open source file", 3000);
            return false;
        }

        vfs_create(dest_path);
        int out_fd = vfs_open(dest_path);
        if (out_fd < 0) {
            vfs_close(in_fd);
            Shell_ShowNotification("Paste Error", "Failed to create destination file", 3000);
            return false;
        }

        static char buf[4096];
        int bytes = 0;
        while ((bytes = vfs_read(in_fd, buf, sizeof(buf))) > 0) {
            vfs_write(out_fd, buf, bytes);
        }

        vfs_close(in_fd);
        vfs_close(out_fd);

        Shell_ShowNotification("Paste", "Copied file successfully!", 3000);
        return true;
    }
}
