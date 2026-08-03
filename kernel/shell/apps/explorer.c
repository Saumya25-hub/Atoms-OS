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

    const char* path = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
    if (!path || strlen(path) == 0) path = "/";

    // 1. Query real VFS directory contents using vfs_readdir
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

    // 2. Populate ATOMS OS custom system namespaces if at root "A:\" ("/")
    if (strcmp(path, "/") == 0) {
        const char* atoms_dirs[] = {
            "ATOMS", "SYS32", "SURFACE", "APPS", "USERS", "NTFS"
        };
        for (int i = 0; i < 6; i++) {
            bool exists = false;
            for (uint32_t j = 0; j < ctx->view_item_count; j++) {
                if (ctx->view_items[j].obj && strcmp(ctx->view_items[j].obj->name, atoms_dirs[i]) == 0) {
                    exists = true;
                    break;
                }
            }
            if (!exists && ctx->view_item_count < EXPLORER_MAX_VIEW_ITEMS) {
                char child_path[256];
                strcpy(child_path, "/");
                strcat(child_path, atoms_dirs[i]);
                vfs_mkdir(child_path);
                BSOMObject* child = BSOM_CreateObject(child_path, BSOM_CLASS_FOLDER);
                if (child) {
                    strcpy(child->name, atoms_dirs[i]);
                    ctx->view_items[ctx->view_item_count].obj = child;
                    ctx->view_items[ctx->view_item_count].icon_id = 1;
                    ctx->view_items[ctx->view_item_count].is_selected = false;
                    ctx->view_item_count++;
                }
            }
        }
    } else if (strstr(path, "SYS32") != NULL || strstr(path, "sys32") != NULL) {
        if (ctx->view_item_count == 0) {
            const char* sys_files[] = {
                "kernel32.sll", "user32.sll", "gdi32.sll", "bwe.sll",
                "advapi32.sll", "shell32.sll", "atoms_sys.bin", "config.ini"
            };
            for (int i = 0; i < 8 && ctx->view_item_count < EXPLORER_MAX_VIEW_ITEMS; i++) {
                char child_path[256];
                strcpy(child_path, path);
                strcat(child_path, "/");
                strcat(child_path, sys_files[i]);
                vfs_create(child_path);
                BSOMObject* child = BSOM_CreateObject(child_path, BSOM_CLASS_DOCUMENT);
                if (child) {
                    strcpy(child->name, sys_files[i]);
                    ctx->view_items[ctx->view_item_count].obj = child;
                    ctx->view_items[ctx->view_item_count].icon_id = 2;
                    ctx->view_items[ctx->view_item_count].is_selected = false;
                    ctx->view_item_count++;
                }
            }
        }
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
                                if (target->class_type == BSOM_CLASS_FOLDER && strlen(target_path) > 0) {
                                    Explorer_Navigate(ctx, target_path);
                                } else {
                                    if (strlen(target->name) > 0 && (strstr(target->name, ".txt") || strstr(target->name, ".TXT") ||
                                                         strstr(target->name, ".log") || strstr(target->name, ".ini") ||
                                                         strstr(target->name, ".md"))) {
                                        notes_app_open(target_path);
                                    } else {
                                        BSOM_Invoke(target);
                                    }
                                }
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
                                char msg[256];
                                strcpy(msg, "Path: ");
                                strcat(msg, target_path);
                                Shell_ShowNotification("Properties", msg, 4000);
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
                    "/", "/Desktop", "/Documents", "/Downloads",
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

            int32_t item_w = 90;
            int32_t item_h = 80;
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
                        // Double Click → Open/Invoke via BSOM
                        BSOMObject* item = ctx->view_items[clicked_idx].obj;
                        if (item) {
                            if (item->class_type == BSOM_CLASS_FOLDER) {
                                if (strlen(item->path) > 0) {
                                    Explorer_Navigate(ctx, item->path);
                                } else {
                                    Explorer_Navigate(ctx, item->name);
                                }
                            } else {
                                if (strlen(item->name) > 0 && (strstr(item->name, ".txt") || strstr(item->name, ".TXT") ||
                                                   strstr(item->name, ".log") || strstr(item->name, ".ini") ||
                                                   strstr(item->name, ".md"))) {
                                    char full_p[256];
                                    if (strlen(item->path) > 0) strcpy(full_p, item->path);
                                    else {
                                        const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                                        if (strcmp(cur_p, "/") == 0) { strcpy(full_p, "/"); strcat(full_p, item->name); }
                                        else { strcpy(full_p, cur_p); strcat(full_p, "/"); strcat(full_p, item->name); }
                                    }
                                    notes_app_open(full_p);
                                } else {
                                    BSOM_Invoke(item);
                                }
                            }
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

    // Initial navigation to root via BSOM
    Explorer_Navigate(&g_explorer_ctx, "/");

    if (out_win) *out_win = g_explorer_ctx.window_id;
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
