// ============================================================
// ATOMS OS Explorer — Main Controller (Phase 9 Rewrite)
// ============================================================
// PURE VIEW LAYER. Zero filesystem ownership.
// ALL operations delegated to BSOM universal object layer.
// Explorer is a RENDERER ONLY.
// ============================================================

#include "explorer.h"
#include "explorer_view.h"
#include "kernel/core/lib/include/string.h"

static ExplorerContext g_explorer_ctx;
static uint64_t s_last_click_ticks = 0;
static int32_t  s_last_click_index = -1;

// ============================================================
// Navigation — 100% BSOM Delegated
// ============================================================

void Explorer_Navigate(ExplorerContext* ctx, const char* path) {
    if (!ctx || !path) return;

    // Release old folder object
    if (ctx->current_folder) {
        BSOM_Release(ctx->current_folder);
        ctx->current_folder = NULL;
    }

    // Open new folder via BSOM
    ctx->current_folder = BSOM_CreateObject(path, BSOM_CLASS_FOLDER);
    if (!ctx->current_folder) return;

    // Push to history
    if (ctx->history_pos < EXPLORER_HISTORY_MAX - 1) {
        ctx->history_pos++;
        strcpy(ctx->history[ctx->history_pos], path);
        ctx->history_count = ctx->history_pos + 1;
    }

    // Refresh view items from BSOM
    Explorer_Refresh(ctx);

    // Update window title
    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) {
        char title_buf[256];
        strcpy(title_buf, "Explorer - ");
        strcat(title_buf, path);
        strcpy(win->title, title_buf);
    }

    BWE_InvalidateWindow(ctx->window_id);
}

#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/shell/apps/notes_app.h"
#include "kernel/shell/apps/rename_dialog.h"
#include "kernel/shell/apps/app_clipboard.h"

extern void Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

static void explorer_on_rename_completed(const char* old_path, const char* new_name, void* user_data) {
    (void)old_path;
    (void)new_name;
    ExplorerContext* ctx = (ExplorerContext*)user_data;
    if (ctx) {
        Explorer_Refresh(ctx);
    }
}

void Explorer_Refresh(ExplorerContext* ctx) {
    if (!ctx) return;

    // Clear view items
    ctx->view_item_count = 0;
    ctx->selected_index = -1;
    ctx->scroll_y = 0;

    const char* path = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "virtual://ThisPC";
    if (!path || strlen(path) == 0) path = "virtual://ThisPC";

    // 0. Handle "This PC" Storage Hub View (100% Real Dynamic VFS Mounts)
    if (strcmp(path, "virtual://ThisPC") == 0 || strcmp(path, "This PC") == 0 || strcmp(path, "ThisPC") == 0) {
        uint32_t total_mounts = vfs_get_mount_count();
        for (uint32_t m = 0; m < total_mounts && ctx->view_item_count < EXPLORER_MAX_VIEW_ITEMS; m++) {
            char m_path[64];
            char m_fs[32];
            char m_dev[32];
            if (!vfs_get_mount_info(m, m_path, sizeof(m_path), m_fs, sizeof(m_fs), m_dev, sizeof(m_dev))) {
                continue;
            }

            BSOMClassType cls = (strstr(m_path, "usb") != NULL || strstr(m_dev, "usb") != NULL) ? BSOM_CLASS_USB : BSOM_CLASS_DRIVE;
            BSOMObject* drive_obj = BSOM_CreateObject(m_path, cls);
            if (drive_obj) {
                char title[64];
                if (cls == BSOM_CLASS_USB || strstr(m_path, "usb") != NULL || strstr(m_dev, "usb") != NULL) {
                    cls = BSOM_CLASS_USB;
                    strcpy(title, "USB Drive (");
                    strcat(title, m_path);
                    strcat(title, ")");
                } else if (strcmp(m_path, "/") == 0) {
                    if (strcmp(m_fs, "bofs") == 0) {
                        strcpy(title, "BOFS Root Volume (/)");
                    } else {
                        strcpy(title, "System Volume (/)");
                    }
                } else if (strcmp(m_fs, "bofs") == 0) {
                    strcpy(title, "BOFS Storage (");
                    strcat(title, m_path);
                    strcat(title, ")");
                } else if (strcmp(m_fs, "ntfs") == 0) {
                    strcpy(title, "NTFS Data (");
                    strcat(title, m_path);
                    strcat(title, ")");
                } else if (strcmp(m_fs, "fat32") == 0) {
                    strcpy(title, "FAT32 Storage (");
                    strcat(title, m_path);
                    strcat(title, ")");
                } else {
                    strcpy(title, "Volume (");
                    strcat(title, m_path);
                    strcat(title, ")");
                }
                strcpy(drive_obj->name, title);
                strcpy(drive_obj->path, m_path);
                drive_obj->class_type = cls;

                uint32_t icon = 10;
                if (cls == BSOM_CLASS_USB) icon = 12;
                else if (strcmp(m_fs, "ntfs") == 0) icon = 11;

                ctx->view_items[ctx->view_item_count].obj = drive_obj;
                ctx->view_items[ctx->view_item_count].icon_id = icon;
                ctx->view_items[ctx->view_item_count].is_selected = false;
                ctx->view_item_count++;
            }
        }
        return;
    }

    // 0.5 Handle "Recycle Bin" View
    if (strcmp(path, "virtual://RecycleBin") == 0 || strcmp(path, "Recycle Bin") == 0 || strcmp(path, "virtual://Trash") == 0) {
        ctx->view_item_count = 0;
        return;
    }
    vfs_dirent_t dirent;
    int index = 0;
    while (index < (int)EXPLORER_MAX_VIEW_ITEMS && vfs_readdir(path, index, &dirent) == 0) {
        if (strlen(dirent.name) > 0 && strcmp(dirent.name, ".") != 0 && strcmp(dirent.name, "..") != 0) {
            BSOMClassType cls = dirent.is_directory ? BSOM_CLASS_FOLDER : BSOM_CLASS_DOCUMENT;
            char child_path[256];
            if (strcmp(path, "/") == 0) {
                strcpy(child_path, "/");
                strcat(child_path, dirent.name);
            } else {
                strcpy(child_path, path);
                strcat(child_path, "/");
                strcat(child_path, dirent.name);
            }

            BSOMObject* child = BSOM_CreateObject(child_path, cls);
            if (child) {
                strcpy(child->name, dirent.name);
                ctx->view_items[ctx->view_item_count].obj = child;
                ctx->view_items[ctx->view_item_count].icon_id = (cls == BSOM_CLASS_FOLDER) ? 1 : 2;
                ctx->view_items[ctx->view_item_count].is_selected = false;
                ctx->view_item_count++;
            }
        }
        index++;
    }

    BWE_InvalidateWindow(ctx->window_id);
}

void Explorer_Back(ExplorerContext* ctx) {
    if (!ctx || ctx->history_pos <= 0) return;
    ctx->history_pos--;
    // Navigate without pushing to history again
    if (ctx->current_folder) {
        BSOM_Release(ctx->current_folder);
        ctx->current_folder = NULL;
    }
    ctx->current_folder = BSOM_CreateObject(ctx->history[ctx->history_pos], BSOM_CLASS_FOLDER);
    Explorer_Refresh(ctx);
    BWE_InvalidateWindow(ctx->window_id);
}

void Explorer_Forward(ExplorerContext* ctx) {
    if (!ctx || ctx->history_pos >= ctx->history_count - 1) return;
    ctx->history_pos++;
    if (ctx->current_folder) {
        BSOM_Release(ctx->current_folder);
        ctx->current_folder = NULL;
    }
    ctx->current_folder = BSOM_CreateObject(ctx->history[ctx->history_pos], BSOM_CLASS_FOLDER);
    Explorer_Refresh(ctx);
    BWE_InvalidateWindow(ctx->window_id);
}

void Explorer_Up(ExplorerContext* ctx) {
    if (!ctx || !ctx->current_folder) return;
    // Navigate to parent by truncating path
    char parent[BDE_PATH_MAX];
    strcpy(parent, ctx->current_folder->path);
    int len = strlen(parent);
    if (len > 1) {
        // Find last '/' and truncate
        for (int i = len - 1; i > 0; i--) {
            if (parent[i] == '/') { parent[i] = '\0'; break; }
        }
        if (strlen(parent) == 0) strcpy(parent, "/");
    }
    Explorer_Navigate(ctx, parent);
}

extern uint64_t sys_service_exec(const char *path, const char **argv, const char **envp);

static void explorer_itoa(uint64_t val, char* str) {
    if (val == 0) { str[0] = '0'; str[1] = '\0'; return; }
    char temp[24]; int i = 0;
    while (val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    int j = 0;
    while (i > 0) { str[j++] = temp[--i]; }
    str[j] = '\0';
}

static void explorer_open_item(ExplorerContext* ctx, BSOMObject* item, const char* target_path) {
    if (!item || !target_path) return;

    if (item->class_type == BSOM_CLASS_FOLDER ||
        item->class_type == BSOM_CLASS_DRIVE ||
        item->class_type == BSOM_CLASS_USB ||
        item->class_type == BSOM_CLASS_VIRTUAL) {
        if (strlen(item->path) > 0) {
            Explorer_Navigate(ctx, item->path);
        } else {
            Explorer_Navigate(ctx, item->name);
        }
    } else {
        if (strlen(item->name) > 0 && (strstr(item->name, ".txt") || strstr(item->name, ".TXT") ||
                                       strstr(item->name, ".log") || strstr(item->name, ".ini") ||
                                       strstr(item->name, ".md"))) {
            notes_app_open(target_path);
        } else if (strstr(item->name, ".avi") || strstr(item->name, ".AVI") ||
                   strstr(item->name, ".mp4") || strstr(item->name, ".MP4") ||
                   strstr(item->name, ".mkv") || strstr(item->name, ".MKV") ||
                   strstr(item->name, ".webm") || strstr(item->name, ".WEBM") ||
                   strstr(item->name, ".ts") || strstr(item->name, ".TS") ||
                   strstr(item->name, ".mp3") || strstr(item->name, ".MP3") ||
                   strstr(item->name, ".wav") || strstr(item->name, ".WAV") ||
                   strstr(item->name, ".flac") || strstr(item->name, ".FLAC") ||
                   strstr(item->name, ".aac") || strstr(item->name, ".AAC")) {
            /* Phase 1: Native Ring-3 Media Player Execution */
            const char* media_argv[3];
            media_argv[0] = "/media_player.elf";
            media_argv[1] = target_path;
            media_argv[2] = NULL;
            uint64_t exec_res = sys_service_exec("/media_player.elf", media_argv, NULL);
            if (exec_res > 0 && exec_res < 0x80000000ULL) {
                char msg[64];
                char pid_buf[24];
                strcpy(msg, "Launched Media Player (PID ");
                explorer_itoa(exec_res, pid_buf);
                strcat(msg, pid_buf);
                strcat(msg, ")");
                Shell_ShowNotification("Media Service", msg, 3000);
            } else {
                Shell_ShowNotification("Media Service", "Failed to launch Ring-3 Media Player", 3000);
            }
        } else {
            /* Authoritative execution via Phase 10 SYS_EXEC pipeline */
            uint64_t exec_res = sys_service_exec(target_path, NULL, NULL);
            if (exec_res > 0 && exec_res < 0x80000000ULL) {
                char msg[64];
                char pid_buf[24];
                strcpy(msg, "Launched Application (PID ");
                explorer_itoa(exec_res, pid_buf);
                strcat(msg, pid_buf);
                strcat(msg, ")");
                Shell_ShowNotification("Process Manager", msg, 3000);
            } else if (exec_res == (uint64_t)-13 /* EACCES */) {
                Shell_ShowNotification("Security", "Permission Denied: Not Executable", 3000);
            } else if (exec_res == (uint64_t)-2 /* BAD FORMAT */) {
                Shell_ShowNotification("Launcher", "Invalid executable binary", 3000);
            } else {
                BSOM_Invoke(item);
            }
        }
    }
}

// ============================================================
// Event Handler — 100% BSOM Delegated
// ============================================================

static void Explorer_HandleEvent(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win || !win->user_data) return;

    ExplorerContext* ctx = (ExplorerContext*)win->user_data;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;
    int32_t bw = win->screen_bounds.width - 10;
    int32_t bh = win->screen_bounds.height - 40;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        // Handle Context Menu clicks if open
        if (ctx->ctx_menu_open) {
            int32_t cx = ctx->ctx_menu_x;
            int32_t cy = ctx->ctx_menu_y;
            int32_t cw = 150;
            int32_t ch = ctx->ctx_menu_is_item ? 162 : 112;

            if (mx >= cx && mx <= cx + cw && my >= cy && my <= cy + ch) {
                int32_t option = (my - (cy + 4)) / 26;
                ctx->ctx_menu_open = false;
                if (ctx->ctx_menu_is_item) {
                    if (ctx->selected_index >= 0 && ctx->selected_index < (int32_t)ctx->view_item_count) {
                        BSOMObject* target = ctx->view_items[ctx->selected_index].obj;
                        if (target) {
                            char target_path[256];
                            if (strlen(target->path) > 0) {
                                strcpy(target_path, target->path);
                            } else {
                                const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                                if (strcmp(cur_p, "/") == 0) { strcpy(target_path, "/"); strcat(target_path, target->name); }
                                else { strcpy(target_path, cur_p); strcat(target_path, "/"); strcat(target_path, target->name); }
                            }

                            if (option == 0) { // Open
                                explorer_open_item(ctx, target, target_path);
                            } else if (option == 1) { // Cut
                                App_ClipboardCut(target_path);
                            } else if (option == 2) { // Copy
                                App_ClipboardCopy(target_path);
                            } else if (option == 3) { // Rename
                                RenameDialog_Open(target_path, target->name, explorer_on_rename_completed, ctx);
                            } else if (option == 4) { // Delete
                                if (strlen(target_path) > 0) {
                                    vfs_delete(target_path);
                                    BSOM_Delete(target, false);
                                    Shell_ShowNotification("Explorer", "Deleted item", 3000);
                                    Explorer_Refresh(ctx);
                                }
                            } else if (option == 5) { // Properties
                                atoms_stat_t st;
                                if (vfs_stat(target_path, &st) == 0) {
                                    char msg[128];
                                    char num_buf[24];
                                    strcpy(msg, "Ino: ");
                                    explorer_itoa(st.st_ino, num_buf);
                                    strcat(msg, num_buf);
                                    strcat(msg, " | Size: ");
                                    explorer_itoa(st.st_size, num_buf);
                                    strcat(msg, num_buf);
                                    strcat(msg, " B | Mode: 0");
                                    char oct[4];
                                    oct[0] = '0' + ((st.st_mode >> 6) & 7);
                                    oct[1] = '0' + ((st.st_mode >> 3) & 7);
                                    oct[2] = '0' + (st.st_mode & 7);
                                    oct[3] = '\0';
                                    strcat(msg, oct);
                                    Shell_ShowNotification("Properties", msg, 5000);
                                } else {
                                    char msg[256];
                                    strcpy(msg, "Path: ");
                                    strcat(msg, target_path);
                                    Shell_ShowNotification("Properties", msg, 4000);
                                }
                            }
                        }
                    }
                } else {
                    const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                    if (!cur_p || strlen(cur_p) == 0) cur_p = "/";

                    if (option == 0) { // + New Folder
                        char new_p[256];
                        if (strcmp(cur_p, "/") == 0) strcpy(new_p, "/New Folder");
                        else { strcpy(new_p, cur_p); strcat(new_p, "/New Folder"); }
                        vfs_mkdir(new_p);
                        Explorer_Refresh(ctx);
                    } else if (option == 1) { // + New File
                        char new_p[256];
                        if (strcmp(cur_p, "/") == 0) strcpy(new_p, "/New Document.txt");
                        else { strcpy(new_p, cur_p); strcat(new_p, "/New Document.txt"); }
                        vfs_create(new_p);
                        Explorer_Refresh(ctx);
                        notes_app_open(new_p);
                    } else if (option == 2) { // Paste
                        App_ClipboardPaste(cur_p);
                        Explorer_Refresh(ctx);
                    } else if (option == 3) { // Refresh
                        Explorer_Refresh(ctx);
                    }
                }
                BWE_InvalidateWindow(win_id);
                return;
            }
            ctx->ctx_menu_open = false;
            BWE_InvalidateWindow(win_id);
        }

        // 1. Right Click Event Handling -> Open Context Menu
        if (event->data.mouse.buttons & 2) {
            ctx->ctx_menu_open = true;
            ctx->ctx_menu_x = mx;
            ctx->ctx_menu_y = my;
            ctx->ctx_menu_is_item = false;

            if (mx >= bx + 170 && mx <= bx + bw && my >= by + 62 && my <= by + bh - 26) {
                int32_t main_x = mx - (bx + 170 + 8);
                int32_t main_y = my - (by + 62 + 10) + ctx->scroll_y;
                int32_t item_w = 90;
                int32_t item_h = 80;
                int32_t cols = (bw - 170) / item_w;
                if (cols <= 0) cols = 1;

                int32_t col = main_x / item_w;
                int32_t row = main_y / item_h;
                if (col >= 0 && col < cols && row >= 0) {
                    int32_t clicked_idx = (row * cols) + col;
                    if (clicked_idx >= 0 && clicked_idx < (int32_t)ctx->view_item_count) {
                        for (uint32_t i = 0; i < ctx->view_item_count; i++) {
                            ctx->view_items[i].is_selected = false;
                        }
                        ctx->view_items[clicked_idx].is_selected = true;
                        ctx->selected_index = clicked_idx;
                        ctx->ctx_menu_is_item = true;
                    }
                }
            }

            // Smart Bounds Clamping: Ensure context menu stays strictly inside Explorer window
            int32_t cw = 150;
            int32_t ch = ctx->ctx_menu_is_item ? 110 : 86;
            int32_t max_x = bx + bw - cw - 6;
            int32_t max_y = by + bh - ch - 30;
            if (ctx->ctx_menu_x > max_x) ctx->ctx_menu_x = max_x;
            if (ctx->ctx_menu_y > max_y) ctx->ctx_menu_y = max_y;
            if (ctx->ctx_menu_x < bx + 170) ctx->ctx_menu_x = bx + 170;
            if (ctx->ctx_menu_y < by + 62) ctx->ctx_menu_y = by + 62;

            BWE_InvalidateWindow(win_id);
            return;
        }

        // 2. Toolbar Clicks → Delegated to BSOM Navigation
        if (my >= by && my <= by + 34) {
            int32_t rx = mx - bx;
            if (rx >= 8 && rx <= 40) {
                Explorer_Back(ctx);
            } else if (rx >= 44 && rx <= 76) {
                Explorer_Forward(ctx);
            } else if (rx >= 80 && rx <= 128) {
                Explorer_Up(ctx);
            } else if (rx >= 132 && rx <= 196) {
                Explorer_Refresh(ctx);
            }
            return;
        }

        // 3. Sidebar Clicks → Navigate via BSOM
        if (mx >= bx && mx <= bx + 170 && my >= by + 62 && my <= by + bh - 26) {
            int32_t item_y = (my - (by + 98)) / 30;
            if (item_y >= 0 && item_y < 8) {
                const char* sidebar_paths[] = {
                    "virtual://ThisPC", "/desktop", "/DOCS", "/Downloads",
                    "/Music", "/Pictures", "/Videos", "/RecycleBin"
                };
                Explorer_Navigate(ctx, sidebar_paths[item_y]);
            }
            return;
        }

        // 4. Main Grid Clicks → Selection & Activation via BSOM
        if (mx >= bx + 170 && mx <= bx + bw && my >= by + 62 && my <= by + bh - 26) {
            int32_t main_x = mx - (bx + 170 + 8);
            int32_t main_y = my - (by + 62 + 10) + ctx->scroll_y;

            const char* cur_path_check = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "virtual://ThisPC";
            bool is_this_pc_mode = (strcmp(cur_path_check, "virtual://ThisPC") == 0 || strcmp(cur_path_check, "This PC") == 0 || strcmp(cur_path_check, "ThisPC") == 0);

            int32_t item_w = is_this_pc_mode ? 210 : 90;
            int32_t item_h = is_this_pc_mode ? 170 : 80;
            int32_t cols = (bw - 170) / item_w;
            if (cols <= 0) cols = 1;

            int32_t col = main_x / item_w;
            int32_t row = main_y / item_h;

            if (col >= 0 && col < cols && row >= 0) {
                int32_t clicked_idx = (row * cols) + col;
                if (clicked_idx >= 0 && clicked_idx < (int32_t)ctx->view_item_count) {
                    // Deselect all
                    for (uint32_t i = 0; i < ctx->view_item_count; i++) {
                        ctx->view_items[i].is_selected = false;
                    }
                    // Select clicked
                    ctx->view_items[clicked_idx].is_selected = true;
                    ctx->selected_index = clicked_idx;

                    // Double click detection
                    extern uint64_t timer_get_ticks(void);
                    uint64_t now = timer_get_ticks();
                    if (clicked_idx == s_last_click_index && (now - s_last_click_ticks) < 400) {
                        // Double Click → Open/Invoke via authoritative pipeline
                        BSOMObject* item = ctx->view_items[clicked_idx].obj;
                        if (item) {
                            char full_p[256];
                            if (strlen(item->path) > 0) {
                                strcpy(full_p, item->path);
                            } else {
                                const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                                if (strcmp(cur_p, "/") == 0) { strcpy(full_p, "/"); strcat(full_p, item->name); }
                                else { strcpy(full_p, cur_p); strcat(full_p, "/"); strcat(full_p, item->name); }
                            }
                            explorer_open_item(ctx, item, full_p);
                        }
                        s_last_click_index = -1;
                        s_last_click_ticks = 0;
                    } else {
                        s_last_click_index = clicked_idx;
                        s_last_click_ticks = now;
                    }
                    BWE_InvalidateWindow(win_id);
                }
            }
        }
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        if (kc == 0x71 || kc == 0x3C || kc == 0x70) { // F2 -> Rename selected item
            if (ctx->selected_index >= 0 && ctx->selected_index < (int32_t)ctx->view_item_count) {
                BSOMObject* target = ctx->view_items[ctx->selected_index].obj;
                if (target) {
                    char target_path[256];
                    if (strlen(target->path) > 0) strcpy(target_path, target->path);
                    else {
                        const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                        if (strcmp(cur_p, "/") == 0) { strcpy(target_path, "/"); strcat(target_path, target->name); }
                        else { strcpy(target_path, cur_p); strcat(target_path, "/"); strcat(target_path, target->name); }
                    }
                    RenameDialog_Open(target_path, target->name, explorer_on_rename_completed, ctx);
                }
            }
        } else if (kc == 0x7F || kc == 0x2E) { // DEL -> Delete selected item
            if (ctx->selected_index >= 0 && ctx->selected_index < (int32_t)ctx->view_item_count) {
                BSOMObject* target = ctx->view_items[ctx->selected_index].obj;
                if (target) {
                    char target_path[256];
                    if (strlen(target->path) > 0) strcpy(target_path, target->path);
                    else {
                        const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                        if (strcmp(cur_p, "/") == 0) { strcpy(target_path, "/"); strcat(target_path, target->name); }
                        else { strcpy(target_path, cur_p); strcat(target_path, "/"); strcat(target_path, target->name); }
                    }
                    vfs_delete(target_path);
                    BSOM_Delete(target, false);
                    Shell_ShowNotification("Explorer DEL", "Deleted selected item", 3000);
                    Explorer_Refresh(ctx);
                }
            }
        }
    }
}

// ============================================================
// Explorer Create / Init
// ============================================================

int Explorer_Create(uint32_t* out_win) {
    if (g_explorer_ctx.window_id != 0) {
        BWE_Window* existing = BWE_GetWindow(g_explorer_ctx.window_id);
        if (existing && existing->state != BWE_STATE_DESTROYED) {
            BOS_Show(g_explorer_ctx.window_id);
            BOS_SetFocus(g_explorer_ctx.window_id);
            if (out_win) *out_win = g_explorer_ctx.window_id;
            return 0;
        }
    }

    memset(&g_explorer_ctx, 0, sizeof(ExplorerContext));
    g_explorer_ctx.selected_index = -1;
    g_explorer_ctx.history_pos = -1;
    g_explorer_ctx.history_count = 0;

    bwe_error_t err = BOS_CreateWindow(80, 50, 800, 600,
        "File Explorer (BSOM Authority)", &g_explorer_ctx.window_id);
    if (err != 0) return -1;

    BWE_Window* win = BWE_GetWindow(g_explorer_ctx.window_id);
    if (win) {
        win->user_data = &g_explorer_ctx;
        win->on_render = Explorer_RenderWindow;
        win->on_event  = Explorer_HandleEvent;
    }

    // Initial navigation to This PC Storage Hub via BSOM
    Explorer_Navigate(&g_explorer_ctx, "virtual://ThisPC");

    BOS_Show(g_explorer_ctx.window_id);
    BOS_SetFocus(g_explorer_ctx.window_id);

    if (out_win) *out_win = g_explorer_ctx.window_id;
    return 0;
}

int Explorer_LaunchPath(const char* path) {
    extern uint32_t bos_cursor_set_active_type(uint32_t type);
    bos_cursor_set_active_type(4 /* BCE_CURSOR_APPSTARTING */);

    uint32_t win = 0;
    Explorer_Create(&win);
    if (path && strlen(path) > 0) {
        Explorer_Navigate(&g_explorer_ctx, path);
    }
    return 0;
}

void Explorer_Destroy(uint32_t win_id) {
    if (win_id != 0 && win_id != g_explorer_ctx.window_id) return;
    for (uint32_t i = 0; i < g_explorer_ctx.view_item_count; i++) {
        if (g_explorer_ctx.view_items[i].obj) {
            BSOM_Release(g_explorer_ctx.view_items[i].obj);
        }
    }
    if (g_explorer_ctx.current_folder) {
        BSOM_Release(g_explorer_ctx.current_folder);
    }
    memset(&g_explorer_ctx, 0, sizeof(ExplorerContext));
}

/* LEGACY / DEPRECATED - Retained for backward compatibility stubs only */
int explorer_init(uint32_t* out_win) {
    return Explorer_Create(out_win);
}

void explorer_navigate(ExplorerContext* ctx, const char* path) {
    Explorer_Navigate(ctx, path);
}
