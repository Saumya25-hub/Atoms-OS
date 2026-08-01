#include "dom.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/ui/boasset/asset_types.h"
#include "kernel/engine/horse_engine.h"

extern void display_print(const char* s);

static DesktopObject s_dom_pool[DOM_MAX_OBJECTS];
static uint32_t s_dom_count = 0;
static uint32_t s_next_obj_id = 1;

void dom_init(void) {
    memset(s_dom_pool, 0, sizeof(s_dom_pool));
    s_dom_count = 0;
    s_next_obj_id = 1;
    display_print("[DOM] Desktop Object Manager Initialized.\n");
}

static uint32_t resolve_icon_asset(DOMObjectType type, uint32_t app_id, const char* vfs_path) {
    if (app_id == APP_ID_EXPLORER) return ICON_EXPLORER;
    if (app_id == APP_ID_TERMINAL) return ICON_TERMINAL;
    if (app_id == APP_ID_SETTINGS) return ICON_SETTINGS;
    if (app_id == APP_ID_CALCULATOR) return ICON_CALCULATOR;
    if (app_id == APP_ID_STRESS_TEST) return ICON_STRESS_TEST;
    if (app_id == APP_ID_MUSIC) return ICON_MUSIC;
    if (app_id == APP_ID_DOOM) return ICON_DOOM;
    if (app_id == APP_ID_INPUT_LAB) return ICON_INPUT_LAB;
    if (app_id == APP_ID_ATRIX) return ICON_ATRIX;
    if (app_id == APP_ID_GRAPH_3D) return ICON_GRAPH_3D;
    if (app_id == APP_ID_TMH) return ICON_TMH;

    if (vfs_path) {
        if (strstr(vfs_path, "Terminal") != NULL) return ICON_TERMINAL;
        if (strstr(vfs_path, "Settings") != NULL) return ICON_SETTINGS;
        if (strstr(vfs_path, "Explorer") != NULL || strstr(vfs_path, "This PC") != NULL) return ICON_EXPLORER;
        if (strstr(vfs_path, "Recycle") != NULL) return ICON_FILE;
    }

    if (type == DOM_OBJ_FOLDER) return ICON_FOLDER;
    if (type == DOM_OBJ_DRIVE) return ICON_EXPLORER;
    if (type == DOM_OBJ_USB) return ICON_EXPLORER;
    if (type == DOM_OBJ_RECYCLE_BIN) return ICON_FILE;

    // Check extension for files
    if (vfs_path) {
        int len = strlen(vfs_path);
        if (len > 4 && strcmp(vfs_path + len - 4, ".elf") == 0) return ICON_FILE;
    }

    return ICON_FILE;
}

DesktopObject* dom_create_object(const char* vfs_path, const char* display_name, DOMObjectType type, uint32_t app_id) {
    int slot = -1;
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (!s_dom_pool[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return NULL;

    DesktopObject* obj = &s_dom_pool[slot];
    memset(obj, 0, sizeof(DesktopObject));

    obj->obj_id = s_next_obj_id++;
    obj->type = type;
    obj->app_id = app_id;
    obj->active = true;
    obj->grid_col = -1;
    obj->grid_row = -1;

    if (vfs_path) strncpy(obj->vfs_path, vfs_path, sizeof(obj->vfs_path) - 1);
    if (display_name) strncpy(obj->display_name, display_name, sizeof(obj->display_name) - 1);

    obj->icon_asset_id = resolve_icon_asset(type, app_id, vfs_path);
    s_dom_count++;

    return obj;
}

bool dom_destroy_object(uint32_t obj_id) {
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && s_dom_pool[i].obj_id == obj_id) {
            s_dom_pool[i].active = false;
            if (s_dom_count > 0) s_dom_count--;
            return true;
        }
    }
    return false;
}

bool dom_destroy_by_path(const char* path) {
    if (!path) return false;
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && strcmp(s_dom_pool[i].vfs_path, path) == 0) {
            s_dom_pool[i].active = false;
            if (s_dom_count > 0) s_dom_count--;
            return true;
        }
    }
    return false;
}

DesktopObject* dom_find_by_id(uint32_t obj_id) {
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && s_dom_pool[i].obj_id == obj_id) {
            return &s_dom_pool[i];
        }
    }
    return NULL;
}

DesktopObject* dom_find_by_path(const char* path) {
    if (!path) return NULL;
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && strcmp(s_dom_pool[i].vfs_path, path) == 0) {
            return &s_dom_pool[i];
        }
    }
    return NULL;
}

DesktopObject* dom_find_by_win_id(uint32_t win_id) {
    if (win_id == 0) return NULL;
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && s_dom_pool[i].bwe_win_id == win_id) {
            return &s_dom_pool[i];
        }
    }
    return NULL;
}

DesktopObject* dom_find_by_grid(int32_t col, int32_t row) {
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && s_dom_pool[i].grid_col == col && s_dom_pool[i].grid_row == row) {
            return &s_dom_pool[i];
        }
    }
    return NULL;
}

DesktopObject* dom_get_all_objects(uint32_t* out_count) {
    if (out_count) *out_count = s_dom_count;
    return s_dom_pool;
}

void dom_clear_selection(void) {
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active) {
            s_dom_pool[i].is_selected = false;
        }
    }
}

void dom_select_all(void) {
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active) {
            s_dom_pool[i].is_selected = true;
        }
    }
}

uint32_t dom_get_selected_objects(DesktopObject** out_array, uint32_t max_count) {
    uint32_t count = 0;
    for (int i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (s_dom_pool[i].active && s_dom_pool[i].is_selected) {
            if (count < max_count) {
                out_array[count++] = &s_dom_pool[i];
            }
        }
    }
    return count;
}
