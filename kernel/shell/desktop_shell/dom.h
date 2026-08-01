#ifndef DESKTOP_OBJECT_MANAGER_H
#define DESKTOP_OBJECT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#define DOM_MAX_OBJECTS 256

typedef enum {
    DOM_OBJ_NONE = 0,
    DOM_OBJ_FOLDER,
    DOM_OBJ_FILE,
    DOM_OBJ_SHORTCUT,      // .slink
    DOM_OBJ_DRIVE,         // Fixed partition
    DOM_OBJ_USB,           // Hot-plugged USB
    DOM_OBJ_RECYCLE_BIN,   // Trash Bin
    DOM_OBJ_APP,           // Built-in Application
    DOM_OBJ_VIRTUAL_FOLDER
} DOMObjectType;

typedef struct {
    uint32_t      obj_id;
    char          vfs_path[128];
    char          display_name[64];
    DOMObjectType type;
    uint32_t      file_size;
    uint8_t       is_directory;
    uint32_t      icon_asset_id;
    int32_t       grid_col;
    int32_t       grid_row;
    int32_t       x;
    int32_t       y;
    uint32_t      bwe_win_id;
    uint32_t      app_id;
    bool          is_selected;
    bool          is_dragging;
    bool          active;
} DesktopObject;

// DOM API
void dom_init(void);
DesktopObject* dom_create_object(const char* vfs_path, const char* display_name, DOMObjectType type, uint32_t app_id);
bool dom_destroy_object(uint32_t obj_id);
bool dom_destroy_by_path(const char* path);

DesktopObject* dom_find_by_id(uint32_t obj_id);
DesktopObject* dom_find_by_path(const char* path);
DesktopObject* dom_find_by_win_id(uint32_t win_id);
DesktopObject* dom_find_by_grid(int32_t col, int32_t row);

DesktopObject* dom_get_all_objects(uint32_t* out_count);
void dom_clear_selection(void);
void dom_select_all(void);
uint32_t dom_get_selected_objects(DesktopObject** out_array, uint32_t max_count);

#endif // DESKTOP_OBJECT_MANAGER_H
