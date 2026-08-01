#include "explorer_view.h"
#include "explorer_cache.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/core/lib/include/string.h"

static void format_short_name(char* dest, const char* src, size_t max_len) {
    if (!src || !dest) return;
    size_t len = strlen(src);
    if (len <= max_len) {
        strcpy(dest, src);
        return;
    }
    strncpy(dest, src, max_len - 3);
    dest[max_len - 3] = '.';
    dest[max_len - 2] = '.';
    dest[max_len - 1] = '.';
    dest[max_len] = '\0';
}

void explorer_view_render(ExplorerContext* ctx) {
    if (!ctx) return;
    BWE_InvalidateWindow(ctx->canvas_id);
    BWE_InvalidateWindow(ctx->window_id);
}

void explorer_view_paint(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    if (!fb) return;
    
    BWE_Window* canvas_win = BWE_GetWindow(canvas_id);
    if (!canvas_win) return;
    
    BWE_Window* parent = BWE_GetWindow(canvas_win->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent || !parent->user_data) return;
    
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
    // Ensure BWE layout bounds are fully computed
    extern void BWE_UpdateLayout(uint32_t);
    BWE_UpdateLayout(ctx->window_id);
    
    ExplorerDirCache* cache = explorer_cache_get_directory(ctx->current_path);
    if (!cache) return;
    
    uint32_t bg_color = 0xFF0F172A; // Modern Dark Slate background
    uint32_t text_color = 0xFFF1F5F9; // Pure white text
    uint32_t select_bg = 0x603B82F6; // Accent blue selection card
    uint32_t hover_bg  = 0x303B82F6; // Translucent hover card
    
    int32_t cw = canvas_win->screen_bounds.width;
    int32_t ch = canvas_win->screen_bounds.height;
    if (cw <= 0 || ch <= 0) return;
    
    // Fill canvas background
    BWE_FillRect((BVFramebuffer*)fb, clip->x, clip->y, clip->width, clip->height, bg_color);
    
    ctx->profiler.visible_items_count = 0;
    ctx->profiler.total_items_count = cache->item_count;
    ctx->profiler.repaint_count++;
    
    int32_t y_offset = -ctx->scroll_offset_y;
    int32_t bx = canvas_win->screen_bounds.x;
    int32_t by = canvas_win->screen_bounds.y;

    if (ctx->view_mode == EXP_VIEW_ICON) {
        int32_t item_w = 96;
        int32_t item_h = 96;
        int32_t cols = cw / item_w;
        if (cols <= 0) cols = 1;
        
        for (uint32_t i = 0; i < cache->item_count; i++) {
            int32_t row = i / cols;
            int32_t col = i % cols;
            int32_t rel_y = row * item_h + 12 + y_offset;
            int32_t ix = bx + col * item_w + 12;
            int32_t iy = by + rel_y;
            
            // Viewport clipping (only draw visible entries)
            if (rel_y + item_h < 0 || rel_y > ch) continue;
            
            ctx->profiler.visible_items_count++;
            
            // Modern Rounded Selection / Hover Card Backdrop
            if ((int32_t)i == ctx->selected_index) {
                BWE_FillRect((BVFramebuffer*)fb, ix - 4, iy - 4, item_w - 8, item_h - 4, select_bg);
                BWE_DrawRect((BVFramebuffer*)fb, ix - 4, iy - 4, item_w - 8, item_h - 4, 0xFF3B82F6, 1);
            } else if ((int32_t)i == ctx->hovered_index) {
                BWE_FillRect((BVFramebuffer*)fb, ix - 4, iy - 4, item_w - 8, item_h - 4, hover_bg);
                BWE_DrawRect((BVFramebuffer*)fb, ix - 4, iy - 4, item_w - 8, item_h - 4, 0xFF60A5FA, 1);
            }
            
            // Render 3D Gold Folder or File Asset Card
            ExplorerItem* item = &cache->items[i];
            if (item->is_directory) {
                // 3D Folder Icon (Yellow / Gold)
                BWE_FillRect((BVFramebuffer*)fb, ix + 16, iy + 6, 20, 8, 0xFFD97706); // Top tab
                BWE_FillRect((BVFramebuffer*)fb, ix + 12, iy + 12, 40, 30, 0xFFF59E0B); // Folder body
                BWE_DrawRect((BVFramebuffer*)fb, ix + 12, iy + 12, 40, 30, 0xFFB45309, 1);
            } else {
                // Document / File Card with Color Accent Badge
                uint32_t card_color = item->icon_color;
                BWE_FillRect((BVFramebuffer*)fb, ix + 16, iy + 6, 32, 36, card_color);
                BWE_DrawRect((BVFramebuffer*)fb, ix + 16, iy + 6, 32, 36, 0xFFFFFFFF, 1);
                
                // Document Corner Fold Graphic
                BWE_FillRect((BVFramebuffer*)fb, ix + 38, iy + 6, 10, 10, 0xFF1E293B);
            }
            
            // Draw Item Name (Truncated nicely)
            char short_name[16];
            format_short_name(short_name, item->name, 12);
            
            int32_t text_x = ix + (item_w - 16 - (strlen(short_name) * 7)) / 2;
            if (text_x < ix) text_x = ix;
            BWE_DrawText((BVFramebuffer*)fb, short_name, text_x, iy + 48, text_color, NULL);
        }
    } else {
        // Modern List View (Row-based)
        int32_t row_h = 28;
        for (uint32_t i = 0; i < cache->item_count; i++) {
            int32_t rel_y = i * row_h + 6 + y_offset;
            int32_t iy = by + rel_y;
            if (rel_y + row_h < 0 || rel_y > ch) continue;
            
            ctx->profiler.visible_items_count++;
            
            if ((int32_t)i == ctx->selected_index) {
                BWE_FillRect((BVFramebuffer*)fb, bx + 6, iy, cw - 12, row_h - 2, select_bg);
                BWE_DrawRect((BVFramebuffer*)fb, bx + 6, iy, cw - 12, row_h - 2, 0xFF3B82F6, 1);
            } else if ((int32_t)i == ctx->hovered_index) {
                BWE_FillRect((BVFramebuffer*)fb, bx + 6, iy, cw - 12, row_h - 2, hover_bg);
            }
            
            // File / Folder Icon Badge (16x16)
            ExplorerItem* item = &cache->items[i];
            if (item->is_directory) {
                BWE_FillRect((BVFramebuffer*)fb, bx + 12, iy + 4, 16, 14, 0xFFF59E0B);
            } else {
                BWE_FillRect((BVFramebuffer*)fb, bx + 12, iy + 4, 14, 16, item->icon_color);
            }
            
            // File Name & Details
            BWE_DrawText((BVFramebuffer*)fb, item->name, bx + 36, iy + 4, text_color, NULL);
        }
    }
}

int32_t explorer_view_hit_test(ExplorerContext* ctx, int32_t local_x, int32_t local_y) {
    if (!ctx) return -1;
    
    ExplorerDirCache* cache = explorer_cache_get_directory(ctx->current_path);
    if (!cache || cache->item_count == 0) return -1;
    
    BWE_Window* canvas_win = BWE_GetWindow(ctx->canvas_id);
    if (!canvas_win) return -1;
    int32_t cw = canvas_win->screen_bounds.width;
    
    int32_t y_offset = -ctx->scroll_offset_y;
    
    if (ctx->view_mode == EXP_VIEW_ICON) {
        int32_t item_w = 96;
        int32_t item_h = 96;
        int32_t cols = cw / item_w;
        if (cols <= 0) cols = 1;
        
        for (uint32_t i = 0; i < cache->item_count; i++) {
            int32_t row = i / cols;
            int32_t col = i % cols;
            int32_t ix = col * item_w + 12;
            int32_t iy = row * item_h + 12 + y_offset;
            
            if (local_x >= ix && local_x <= ix + item_w && local_y >= iy && local_y <= iy + item_h) {
                return (int32_t)i;
            }
        }
    } else {
        int32_t row_h = 28;
        for (uint32_t i = 0; i < cache->item_count; i++) {
            int32_t iy = i * row_h + 6 + y_offset;
            if (local_y >= iy && local_y <= iy + row_h) {
                return (int32_t)i;
            }
        }
    }
    return -1;
}

void explorer_view_handle_click(ExplorerContext* ctx, int32_t local_x, int32_t local_y, bool double_click) {
    if (!ctx) return;
    
    int32_t hit = explorer_view_hit_test(ctx, local_x, local_y);
    ctx->selected_index = hit;
    
    if (hit >= 0 && double_click) {
        ExplorerDirCache* cache = explorer_cache_get_directory(ctx->current_path);
        if (cache && hit < (int32_t)cache->item_count) {
            ExplorerItem* item = &cache->items[hit];
            if (item->is_directory) {
                char full_path[256];
                if (strcmp(ctx->current_path, "/") == 0) {
                    strcpy(full_path, "/");
                    strcat(full_path, item->name);
                } else {
                    strcpy(full_path, ctx->current_path);
                    strcat(full_path, "/");
                    strcat(full_path, item->name);
                }
                explorer_navigate(ctx, full_path);
            }
        }
    }
    BWE_InvalidateWindow(ctx->window_id);
}
