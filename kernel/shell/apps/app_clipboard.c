// ============================================================
// ATOMS OS — App Clipboard Engine (Cut, Copy, Paste)
// ============================================================

#include "app_clipboard.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

static char s_app_clipboard_path[256] = {0};
static bool s_app_clipboard_is_cut = false;

extern void Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

void App_ClipboardCut(const char* src_path) {
    if (!src_path || src_path[0] == '\0') return;
    strcpy(s_app_clipboard_path, src_path);
    s_app_clipboard_is_cut = true;
    Shell_ShowNotification("Clipboard", "Cut item to clipboard", 3000);
}

void App_ClipboardCopy(const char* src_path) {
    if (!src_path || src_path[0] == '\0') return;
    strcpy(s_app_clipboard_path, src_path);
    s_app_clipboard_is_cut = false;
    Shell_ShowNotification("Clipboard", "Copied item to clipboard", 3000);
}

const char* App_ClipboardGetPath(void) {
    return s_app_clipboard_path;
}

bool App_ClipboardIsCut(void) {
    return s_app_clipboard_is_cut;
}

static bool app_clipboard_copy_file(const char* src_path, const char* dest_path) {
    int in_fd = vfs_open(src_path);
    if (in_fd < 0) return false;

    vfs_create(dest_path);
    int out_fd = vfs_open(dest_path);
    if (out_fd < 0) {
        vfs_close(in_fd);
        return false;
    }

    static char buf[4096];
    int bytes = 0;
    while ((bytes = vfs_read(in_fd, buf, sizeof(buf))) > 0) {
        vfs_write(out_fd, buf, bytes);
    }

    vfs_close(in_fd);
    vfs_close(out_fd);
    return true;
}

static bool app_clipboard_copy_item(const char* src_path, const char* dest_path) {
    if (!src_path || !dest_path) return false;

    vfs_dirent_t test_ent;
    bool is_dir = (vfs_readdir(src_path, 0, &test_ent) == 0);

    if (is_dir) {
        vfs_mkdir(dest_path);

        vfs_dirent_t dirent;
        int idx = 0;
        while (vfs_readdir(src_path, idx, &dirent) == 0) {
            idx++;
            if (strlen(dirent.name) == 0 || strcmp(dirent.name, ".") == 0 || strcmp(dirent.name, "..") == 0) continue;

            char child_src[256];
            strcpy(child_src, src_path);
            strcat(child_src, "/");
            strcat(child_src, dirent.name);

            char child_dest[256];
            strcpy(child_dest, dest_path);
            strcat(child_dest, "/");
            strcat(child_dest, dirent.name);

            app_clipboard_copy_item(child_src, child_dest);
        }
        return true;
    } else {
        return app_clipboard_copy_file(src_path, dest_path);
    }
}

bool App_ClipboardPaste(const char* dest_dir) {
    if (s_app_clipboard_path[0] == '\0' || !dest_dir) {
        Shell_ShowNotification("Paste", "Clipboard is empty", 3000);
        return false;
    }

    // Extract filename from source path
    const char* filename = s_app_clipboard_path;
    const char* p = s_app_clipboard_path;
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

    if (s_app_clipboard_is_cut) {
        // Cut -> Move: Try direct vfs_rename first, fallback to copy+delete across folders
        if (vfs_rename(s_app_clipboard_path, dest_path) == 0) {
            s_app_clipboard_path[0] = '\0';
            s_app_clipboard_is_cut = false;
            Shell_ShowNotification("Paste", "Moved item successfully!", 3000);
            return true;
        } else if (app_clipboard_copy_item(s_app_clipboard_path, dest_path)) {
            vfs_delete(s_app_clipboard_path);
            s_app_clipboard_path[0] = '\0';
            s_app_clipboard_is_cut = false;
            Shell_ShowNotification("Paste", "Moved item successfully!", 3000);
            return true;
        } else {
            Shell_ShowNotification("Paste Error", "Failed to move item", 3000);
            return false;
        }
    } else {
        // Copy -> Copy file or folder recursively
        if (app_clipboard_copy_item(s_app_clipboard_path, dest_path)) {
            Shell_ShowNotification("Paste", "Copied item successfully!", 3000);
            return true;
        } else {
            Shell_ShowNotification("Paste Error", "Failed to copy item", 3000);
            return false;
        }
    }
}
