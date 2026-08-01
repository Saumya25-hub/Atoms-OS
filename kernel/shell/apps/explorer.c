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

void Explorer_Refresh(ExplorerContext* ctx) {
    if (!ctx || !ctx->current_folder) return;

    // Clear view items
    ctx->view_item_count = 0;
    ctx->selected_index = -1;
    ctx->scroll_y = 0;

    // Query children from BSOM — Explorer NEVER touches VFS
    BSOMObject** children = NULL;
    uint32_t count = 0;
    BSOM_GetChildren(ctx->current_folder, &children, &count);

    // Since BSOM pool returns 0 children for stub folders,
    // populate default virtual namespace items via BSOM
    if (count == 0) {
        const char* default_names[] = {
            "Desktop", "Documents", "Downloads", "Music",
            "Pictures", "Videos", "USB Drive (U:)", "System"
        };
        for (int i = 0; i < 8 && ctx->view_item_count < EXPLORER_MAX_VIEW_ITEMS; i++) {
            BSOMObject* child = BSOM_CreateObject(default_names[i], BSOM_CLASS_FOLDER);
            if (child) {
                ctx->view_items[ctx->view_item_count].obj = child;
                ctx->view_items[ctx->view_item_count].icon_id = BSOM_GetIcon(child);
                ctx->view_items[ctx->view_item_count].is_selected = false;
                ctx->view_item_count++;
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

        // 1. Toolbar Clicks → Delegated to BSOM Navigation
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

        // 2. Sidebar Clicks → Navigate via BSOM
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

        // 3. Main Grid Clicks → Selection & Activation via BSOM
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
                                Explorer_Navigate(ctx, item->name);
                            } else {
                                BSOM_Invoke(item);
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
    }
}

// ============================================================
// Explorer Create / Init
// ============================================================

int Explorer_Create(uint32_t* out_win) {
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
    (void)win_id;
    // Release all view items
    for (uint32_t i = 0; i < g_explorer_ctx.view_item_count; i++) {
        if (g_explorer_ctx.view_items[i].obj) {
            BSOM_Release(g_explorer_ctx.view_items[i].obj);
        }
    }
    if (g_explorer_ctx.current_folder) {
        BSOM_Release(g_explorer_ctx.current_folder);
    }
}

// Legacy compatibility wrappers
int explorer_init(uint32_t* out_win) {
    return Explorer_Create(out_win);
}

void explorer_navigate(ExplorerContext* ctx, const char* path) {
    Explorer_Navigate(ctx, path);
}
