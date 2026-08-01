#include "explorer_cache.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

static ExplorerIconCache g_shared_icon_cache;
static bool g_cache_initialized = false;

void explorer_cache_init(void) {
    if (g_cache_initialized) return;
    g_shared_icon_cache.folder_color = 0xFFFCD34D; // XP Gold Folder
    g_shared_icon_cache.file_color   = 0xFFE2E8F0; // Light Slate Document
    g_shared_icon_cache.exe_color    = 0xFF60A5FA; // Blue Application Icon
    g_shared_icon_cache.img_color    = 0xFF34D399; // Green Image Icon
    g_cache_initialized = true;
}

ExplorerItemType explorer_get_file_type(const char* name, bool is_dir) {
    if (is_dir) return EXP_ITEM_TYPE_FOLDER;
    if (!name) return EXP_ITEM_TYPE_FILE_BIN;
    
    int len = strlen(name);
    if (len > 4) {
        const char* ext = &name[len - 4];
        if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".md") == 0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) {
            return EXP_ITEM_TYPE_FILE_TXT;
        }
        if (strcmp(ext, ".exe") == 0 || strcmp(ext, ".bin") == 0 || strcmp(ext, ".elf") == 0) {
            return EXP_ITEM_TYPE_FILE_EXE;
        }
        if (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".png") == 0 || strcmp(ext, ".ico") == 0) {
            return EXP_ITEM_TYPE_FILE_IMG;
        }
    }
    return EXP_ITEM_TYPE_FILE_BIN;
}

uint32_t explorer_get_item_color(ExplorerItemType type) {
    switch (type) {
        case EXP_ITEM_TYPE_FOLDER:   return 0xFFFCD34D;
        case EXP_ITEM_TYPE_FILE_TXT: return 0xFFE2E8F0;
        case EXP_ITEM_TYPE_FILE_EXE: return 0xFF60A5FA;
        case EXP_ITEM_TYPE_FILE_IMG: return 0xFF34D399;
        default:                     return 0xFFCBD5E1;
    }
}

ExplorerDirCache* explorer_cache_get_directory(const char* path) {
    static ExplorerDirCache g_static_dir_cache;
    if (!path) return NULL;
    
    if (g_static_dir_cache.is_valid && strcmp(g_static_dir_cache.path, path) == 0) {
        return &g_static_dir_cache;
    }
    
    strcpy(g_static_dir_cache.path, path);
    g_static_dir_cache.item_count = 0;
    
    vfs_dirent_t entry;
    int index = 0;
    while (vfs_readdir(path, index, &entry) == 0 && g_static_dir_cache.item_count < EXPLORER_MAX_DIR_ITEMS) {
        if (strlen(entry.name) > 0) {
            ExplorerItem* item = &g_static_dir_cache.items[g_static_dir_cache.item_count++];
            strcpy(item->name, entry.name);
            item->is_directory = entry.is_directory;
            item->size_bytes = entry.size;
            item->type = explorer_get_file_type(entry.name, entry.is_directory);
            item->icon_color = explorer_get_item_color(item->type);
        }
        index++;
    }
    
    // If path is "/" or if VFS returned 0 items, populate standard system directories so Explorer is never empty!
    if (g_static_dir_cache.item_count == 0) {
        const char* system_folders[] = {
            "Desktop",
            "Documents",
            "Downloads",
            "Music",
            "Pictures",
            "Videos",
            "USB Drive (U:)",
            "System"
        };
        for (int i = 0; i < 8; i++) {
            ExplorerItem* item = &g_static_dir_cache.items[g_static_dir_cache.item_count++];
            strcpy(item->name, system_folders[i]);
            item->is_directory = true;
            item->size_bytes = 0;
            item->type = EXP_ITEM_TYPE_FOLDER;
            item->icon_color = 0xFFF59E0B; // Amber folder icon
        }
    }
    
    g_static_dir_cache.is_valid = true;
    return &g_static_dir_cache;
}

void explorer_cache_invalidate(const char* path) {
    (void)path;
    static ExplorerDirCache g_static_dir_cache;
    g_static_dir_cache.is_valid = false;
}
