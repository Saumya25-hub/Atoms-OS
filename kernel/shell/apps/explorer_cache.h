#ifndef BOS_EXPLORER_CACHE_H
#define BOS_EXPLORER_CACHE_H

#include <stdint.h>
#include <stdbool.h>

#define EXPLORER_MAX_DIR_ITEMS 10000

typedef enum {
    EXP_ITEM_TYPE_UNKNOWN = 0,
    EXP_ITEM_TYPE_FOLDER,
    EXP_ITEM_TYPE_FILE_TXT,
    EXP_ITEM_TYPE_FILE_EXE,
    EXP_ITEM_TYPE_FILE_IMG,
    EXP_ITEM_TYPE_FILE_AUDIO,
    EXP_ITEM_TYPE_FILE_BIN
} ExplorerItemType;

typedef struct {
    char             name[128];
    uint32_t         size_bytes;
    bool             is_directory;
    ExplorerItemType type;
    uint32_t         icon_color;
} ExplorerItem;

typedef struct {
    char         path[256];
    ExplorerItem items[EXPLORER_MAX_DIR_ITEMS];
    uint32_t     item_count;
    bool         is_valid;
} ExplorerDirCache;

// Icon Bitmaps Cache Structure (Pre-rendered static 16x16 / 32x32 color patterns)
typedef struct {
    uint32_t folder_color;
    uint32_t file_color;
    uint32_t exe_color;
    uint32_t img_color;
} ExplorerIconCache;

// API
void explorer_cache_init(void);
ExplorerDirCache* explorer_cache_get_directory(const char* path);
void explorer_cache_invalidate(const char* path);
ExplorerItemType explorer_get_file_type(const char* name, bool is_dir);
uint32_t explorer_get_item_color(ExplorerItemType type);

#endif // BOS_EXPLORER_CACHE_H
