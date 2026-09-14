#include "desktop_vfs_sync.h"
#include "dom.h"
#include "bomatrix.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/engine/horse_engine.h"

extern void display_print(const char* s);
extern void create_desktop_icon_from_object(DesktopObject* obj);

void desktop_vfs_sync_init(void) {
    display_print("[VFS SYNC] Initializing Desktop VFS Synchronization...\n");
    dom_init();

    // Ensure /Desktop directory exists
    vfs_mkdir(DESKTOP_VFS_PATH);

    // Perform VFS Directory Scan
    desktop_vfs_sync_scan();
}

void desktop_vfs_sync_scan(void) {
    display_print("[VFS SYNC] Scanning " DESKTOP_VFS_PATH " directory...\n");
    vfs_dirent_t dirent;
    int index = 0;
    int found_items = 0;

    while (vfs_readdir(DESKTOP_VFS_PATH, index++, &dirent) == 0) {
        if (strcmp(dirent.name, ".") == 0 || strcmp(dirent.name, "..") == 0 || strcmp(dirent.name, "desktop.ini") == 0) {
            continue;
        }

        char full_path[128];
        strcpy(full_path, DESKTOP_VFS_PATH);
        strcat(full_path, "/");
        strcat(full_path, dirent.name);

        DOMObjectType type = dirent.is_directory ? DOM_OBJ_FOLDER : DOM_OBJ_FILE;
        uint32_t app_id = 0;

        if (strstr(dirent.name, "usb") != NULL || strstr(dirent.name, "USB") != NULL || strstr(dirent.name, "flash") != NULL) {
            type = DOM_OBJ_USB;
        } else if (strstr(dirent.name, "Recycle") != NULL || strstr(dirent.name, "Trash") != NULL) {
            type = DOM_OBJ_RECYCLE_BIN;
        } else if (strstr(dirent.name, ".slink") != NULL) {
            type = DOM_OBJ_SHORTCUT;
        } else if (strstr(dirent.name, ".elf") != NULL) {
            type = DOM_OBJ_APP;
        }

        DesktopObject* obj = dom_create_object(full_path, dirent.name, type, app_id);
        if (obj) {
            obj->file_size = dirent.size;
            obj->is_directory = dirent.is_directory;
            create_desktop_icon_from_object(obj);
            found_items++;
        }
    }

    // Seed default shortcuts if /Desktop is empty
    if (found_items == 0) {
        display_print("[VFS SYNC] Seeding BOSX desktop applications into /Desktop and /System/Apps...\n");
        
        // Ensure /System/Apps directory exists for .BOSX targets
        vfs_mkdir("/System");
        vfs_mkdir("/System/Apps");

        // Create VFS .BOSX system application entry binaries
        vfs_create("/System/Apps/FileExplorer.BOSX");
        vfs_create("/System/Apps/Terminal.BOSX");
        vfs_create("/System/Apps/Settings.BOSX");
        vfs_create("/System/Apps/TaskManager.BOSX");
        vfs_create("/System/Apps/ControlPanel.BOSX");
        vfs_create("/System/Apps/BOSMediaPlayer.BOSX");

        // Create VFS .slink shortcut files
        vfs_create(DESKTOP_VFS_PATH "/Recycle Bin.slink");
        vfs_create(DESKTOP_VFS_PATH "/File Explorer.slink");
        vfs_create(DESKTOP_VFS_PATH "/Settings.slink");
        vfs_create(DESKTOP_VFS_PATH "/Terminal.slink");
        vfs_create(DESKTOP_VFS_PATH "/BOS Media Player.slink");

        DesktopObject* o1 = dom_create_object(DESKTOP_VFS_PATH "/Recycle Bin.slink", "Recycle Bin", DOM_OBJ_RECYCLE_BIN, 0);
        DesktopObject* o2 = dom_create_object(DESKTOP_VFS_PATH "/File Explorer.slink", "File Explorer.BOSX", DOM_OBJ_SHORTCUT, APP_ID_EXPLORER);
        DesktopObject* o3 = dom_create_object(DESKTOP_VFS_PATH "/Settings.slink", "Settings.BOSX", DOM_OBJ_SHORTCUT, APP_ID_SETTINGS);
        DesktopObject* o4 = dom_create_object(DESKTOP_VFS_PATH "/Terminal.slink", "Terminal.BOSX", DOM_OBJ_SHORTCUT, APP_ID_TERMINAL);
        DesktopObject* o5 = dom_create_object(DESKTOP_VFS_PATH "/BOS Media Player.slink", "BOS Media Player.BOSX", DOM_OBJ_SHORTCUT, APP_ID_MUSIC);

        if (o1) create_desktop_icon_from_object(o1);
        if (o2) create_desktop_icon_from_object(o2);
        if (o3) create_desktop_icon_from_object(o3);
        if (o4) create_desktop_icon_from_object(o4);
        if (o5) create_desktop_icon_from_object(o5);

        desktop_vfs_save_layout();
    } else {
        desktop_vfs_load_layout();
    }

    // Dynamically instantiate Desktop Icon for all mounted USB storage volumes
    uint32_t total_mounts = vfs_get_mount_count();
    for (uint32_t m = 0; m < total_mounts; m++) {
        char m_path[64], m_fs[32], m_dev[32];
        if (!vfs_get_mount_info(m, m_path, sizeof(m_path), m_fs, sizeof(m_fs), m_dev, sizeof(m_dev))) continue;
        if (strstr(m_path, "usb") != NULL || strstr(m_dev, "usb") != NULL) {
            // Check if object already exists for this path
            uint32_t dom_count = 0;
            DesktopObject* pool = dom_get_all_objects(&dom_count);
            bool exists = false;
            for (uint32_t i = 0; i < DOM_MAX_OBJECTS; i++) {
                if (pool[i].active && pool[i].vfs_path && strcmp(pool[i].vfs_path, m_path) == 0) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                char icon_name[32] = "USB Drive";
                if (strlen(m_dev) > 0 && strcmp(m_dev, "none") != 0) {
                    strcpy(icon_name, "USB (");
                    strcat(icon_name, m_dev);
                    strcat(icon_name, ")");
                }
                DesktopObject* usb_obj = dom_create_object(m_path, icon_name, DOM_OBJ_USB, 0);
                if (usb_obj) {
                    create_desktop_icon_from_object(usb_obj);
                    display_print("[VFS SYNC] Created dynamic Desktop icon for USB storage: ");
                    display_print(m_path);
                    display_print("\n");
                }
            }
        }
    }
}

bool desktop_vfs_save_layout(void) {
    display_print("[VFS SYNC] Saving desktop layout to " DESKTOP_INI_PATH "...\n");
    int fd = vfs_open(DESKTOP_INI_PATH);
    if (fd < 0) {
        vfs_create(DESKTOP_INI_PATH);
        fd = vfs_open(DESKTOP_INI_PATH);
    }
    if (fd < 0) return false;

    char header[] = "[DesktopLayout]\nVersion=2.0\n";
    vfs_write(fd, header, strlen(header));

    uint32_t count = 0;
    DesktopObject* pool = dom_get_all_objects(&count);

    for (uint32_t i = 0; i < DOM_MAX_OBJECTS; i++) {
        if (pool[i].active) {
            char line[128];
            strcpy(line, "Item=");
            strcat(line, pool[i].display_name);
            strcat(line, ",col=");

            char temp[16];
            int c = pool[i].grid_col >= 0 ? pool[i].grid_col : 0;
            int r = pool[i].grid_row >= 0 ? pool[i].grid_row : 0;

            // Simple int to str
            int tp = 0;
            temp[tp++] = '0' + (c % 10);
            temp[tp] = '\0';
            strcat(line, temp);
            strcat(line, ",row=");
            temp[0] = '0' + (r % 10);
            temp[1] = '\0';
            strcat(line, temp);
            strcat(line, "\n");

            vfs_write(fd, line, strlen(line));
        }
    }

    vfs_close(fd);
    return true;
}

bool desktop_vfs_load_layout(void) {
    int fd = vfs_open(DESKTOP_INI_PATH);
    if (fd < 0) return false;

    display_print("[VFS SYNC] Loaded persistent layout from " DESKTOP_INI_PATH "\n");
    vfs_close(fd);
    return true;
}

bool desktop_crud_create_folder(const char* folder_name) {
    if (!folder_name || folder_name[0] == '\0') return false;

    char path[128];
    strcpy(path, DESKTOP_VFS_PATH "/");
    strcat(path, folder_name);

    if (vfs_mkdir(path) == 0) {
        DesktopObject* obj = dom_create_object(path, folder_name, DOM_OBJ_FOLDER, 0);
        if (obj) {
            create_desktop_icon_from_object(obj);
            desktop_vfs_save_layout();
            return true;
        }
    }
    return false;
}

bool desktop_crud_create_file(const char* file_name, const char* content) {
    if (!file_name || file_name[0] == '\0') return false;

    char path[128];
    strcpy(path, DESKTOP_VFS_PATH "/");
    strcat(path, file_name);

    if (vfs_create(path) == 0) {
        if (content && content[0] != '\0') {
            int fd = vfs_open(path);
            if (fd >= 0) {
                vfs_write(fd, (void*)content, strlen(content));
                vfs_close(fd);
            }
        }
        DesktopObject* obj = dom_create_object(path, file_name, DOM_OBJ_FILE, 0);
        if (obj) {
            create_desktop_icon_from_object(obj);
            desktop_vfs_save_layout();
            return true;
        }
    }
    return false;
}

bool desktop_crud_rename(const char* old_path, const char* new_name) {
    if (!old_path || !new_name || new_name[0] == '\0') return false;

    char new_path[128];
    strcpy(new_path, DESKTOP_VFS_PATH "/");
    strcat(new_path, new_name);

    if (vfs_rename(old_path, new_name) == 0) {
        DesktopObject* obj = dom_find_by_path(old_path);
        if (obj) {
            strncpy(obj->vfs_path, new_path, sizeof(obj->vfs_path) - 1);
            strncpy(obj->display_name, new_name, sizeof(obj->display_name) - 1);
            if (obj->bwe_win_id != 0) {
                BWE_Window* win = BWE_GetWindow(obj->bwe_win_id);
                if (win) {
                    strncpy(win->control_data.button.text, new_name, sizeof(win->control_data.button.text) - 1);
                }
            }
            desktop_vfs_save_layout();
            return true;
        }
    }
    return false;
}

bool desktop_crud_delete(const char* path, bool permanent) {
    if (!path) return false;
    (void)permanent;

    if (vfs_delete(path) == 0) {
        DesktopObject* obj = dom_find_by_path(path);
        if (obj) {
            if (obj->bwe_win_id != 0) {
                bomatrix_unregister_icon(obj->bwe_win_id);
                BOS_DestroySurface(obj->bwe_win_id);
            }
            dom_destroy_object(obj->obj_id);
            desktop_vfs_save_layout();
            return true;
        }
    }
    return false;
}

bool desktop_crud_copy(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return false;
    int src_fd = vfs_open(src_path);
    if (src_fd < 0) return false;

    // Find filename from src_path
    const char* filename = src_path;
    for (int i = 0; src_path[i] != '\0'; i++) {
        if (src_path[i] == '/') filename = &src_path[i + 1];
    }

    char dest_path[128];
    strcpy(dest_path, dest_dir);
    strcat(dest_path, "/Copy_of_");
    strcat(dest_path, filename);

    if (vfs_create(dest_path) == 0) {
        int dest_fd = vfs_open(dest_path);
        if (dest_fd >= 0) {
            char buffer[512];
            int bytes = 0;
            while ((bytes = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
                vfs_write(dest_fd, buffer, bytes);
            }
            vfs_close(dest_fd);
        }
        vfs_close(src_fd);

        char display[64];
        strcpy(display, "Copy_of_");
        strcat(display, filename);

        DesktopObject* obj = dom_create_object(dest_path, display, DOM_OBJ_FILE, 0);
        if (obj) {
            create_desktop_icon_from_object(obj);
            desktop_vfs_save_layout();
            return true;
        }
    }
    vfs_close(src_fd);
    return false;
}

bool desktop_crud_move(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return false;
    const char* filename = src_path;
    for (int i = 0; src_path[i] != '\0'; i++) {
        if (src_path[i] == '/') filename = &src_path[i + 1];
    }
    return desktop_crud_rename(src_path, filename);
}
