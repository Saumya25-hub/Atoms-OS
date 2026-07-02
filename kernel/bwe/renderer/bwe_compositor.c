#include "../include/bwe.h"

// External references
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void* BOVISUAL_Graphics_GetBuffer(void);
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern uint32_t g_z_order_stack[BWE_MAX_WINDOWS];
extern uint32_t g_z_stack_count;
extern uint32_t g_focused_window_id;

// Direct VBE back-page flipping routines
extern BVFramebuffer vbe_get_back_page(void);
extern void vbe_swap_page(void);

static const BVFramebuffer* s_current_render_target = 0;

void BWE_SetRenderTarget(const BVFramebuffer* fb) {
    s_current_render_target = fb;
}

const BVFramebuffer* BWE_GetRenderTarget(void) {
    if (s_current_render_target) {
        return s_current_render_target;
    }
    extern BVFramebuffer* vbe_get_framebuffer(void);
    return vbe_get_framebuffer();
}

// BWE Compositor Globals
static BWE_Rect g_clip_stack[32];
static uint32_t g_clip_stack_depth = 0;

#define MAX_DIRTY_RECTS 32
static BWE_Rect g_dirty_rects[MAX_DIRTY_RECTS];
static uint32_t g_dirty_rect_count = 0;

// Telemetry Stats for Debug HUD
static uint32_t s_fps = 60;
static uint32_t s_render_time_ms = 0;
static uint32_t s_paint_calls = 0;

// ============================================================
// Clipping Stack Implementation
// ============================================================

void BWE_ClipPush(BWE_Rect rect) {
    if (g_clip_stack_depth >= 32) return;

    if (g_clip_stack_depth == 0) {
        g_clip_stack[0] = rect;
    } else {
        // Intersect new rect with current top of stack
        const BWE_Rect* top = &g_clip_stack[g_clip_stack_depth - 1];
        int32_t cx1 = (rect.x > top->x) ? rect.x : top->x;
        int32_t cy1 = (rect.y > top->y) ? rect.y : top->y;
        
        int32_t rx2 = rect.x + rect.width;
        int32_t ry2 = rect.y + rect.height;
        int32_t tx2 = top->x + top->width;
        int32_t ty2 = top->y + top->height;
        
        int32_t cx2 = (rx2 < tx2) ? rx2 : tx2;
        int32_t cy2 = (ry2 < ty2) ? ry2 : ty2;

        BWE_Rect intersected;
        if (cx1 < cx2 && cy1 < cy2) {
            intersected.x = cx1;
            intersected.y = cy1;
            intersected.width = cx2 - cx1;
            intersected.height = cy2 - cy1;
        } else {
            intersected.x = 0;
            intersected.y = 0;
            intersected.width = 0;
            intersected.height = 0;
        }
        g_clip_stack[g_clip_stack_depth] = intersected;
    }
    g_clip_stack_depth++;
}

void BWE_ClipPop(void) {
    if (g_clip_stack_depth > 0) {
        g_clip_stack_depth--;
    }
}

bool BWE_GetClip(BWE_Rect* out_rect) {
    if (g_clip_stack_depth == 0 || !out_rect) return false;
    *out_rect = g_clip_stack[g_clip_stack_depth - 1];
    return true;
}

// ============================================================
// Dirty Rectangle Manager Implementation
// ============================================================

void BWE_AddCompositorDirtyRect(const BWE_Rect* rect) {
    if (g_dirty_rect_count >= MAX_DIRTY_RECTS) {
        // Fallback to full screen damage
        g_dirty_rect_count = 1;
        g_dirty_rects[0].x = 0;
        g_dirty_rects[0].y = 0;
        g_dirty_rects[0].width = (int32_t)g_kernel_screen_width;
        g_dirty_rects[0].height = (int32_t)g_kernel_screen_height;
        return;
    }

    // Clip to screen boundaries
    int32_t cx1 = (rect->x > 0) ? rect->x : 0;
    int32_t cy1 = (rect->y > 0) ? rect->y : 0;
    int32_t rx2 = rect->x + rect->width;
    int32_t ry2 = rect->y + rect->height;
    int32_t sx2 = (int32_t)g_kernel_screen_width;
    int32_t sy2 = (int32_t)g_kernel_screen_height;
    int32_t cx2 = (rx2 < sx2) ? rx2 : sx2;
    int32_t cy2 = (ry2 < sy2) ? ry2 : sy2;

    if (cx1 >= cx2 || cy1 >= cy2) return; // Empty region

    BWE_Rect clipped = { cx1, cy1, cx2 - cx1, cy2 - cy1 };

    // Duplicate check
    for (uint32_t i = 0; i < g_dirty_rect_count; i++) {
        BWE_Rect* r = &g_dirty_rects[i];
        if (clipped.x >= r->x && clipped.y >= r->y &&
            clipped.x + clipped.width <= r->x + r->width &&
            clipped.y + clipped.height <= r->y + r->height) {
            return; // Covered
        }
    }

    g_dirty_rects[g_dirty_rect_count++] = clipped;
}

static bool rects_overlap(const BWE_Rect* r1, const BWE_Rect* r2) {
    return !(r1->x + r1->width < r2->x ||
             r2->x + r2->width < r1->x ||
             r1->y + r1->height < r2->y ||
             r2->y + r2->height < r1->y);
}

static BWE_Rect merge_rects(const BWE_Rect* r1, const BWE_Rect* r2) {
    BWE_Rect m;
    int32_t x1 = (r1->x < r2->x) ? r1->x : r2->x;
    int32_t y1 = (r1->y < r2->y) ? r1->y : r2->y;
    int32_t x2 = (r1->x + r1->width > r2->x + r2->width) ? (r1->x + r1->width) : (r2->x + r2->width);
    int32_t y2 = (r1->y + r1->height > r2->y + r2->height) ? (r1->y + r1->height) : (r2->y + r2->height);
    m.x = x1;
    m.y = y1;
    m.width = x2 - x1;
    m.height = y2 - y1;
    return m;
}

void BWE_MergeDirtyRects(void) {
    bool merged = true;
    while (merged) {
        merged = false;
        for (uint32_t i = 0; i < g_dirty_rect_count; i++) {
            for (uint32_t j = i + 1; j < g_dirty_rect_count; j++) {
                if (rects_overlap(&g_dirty_rects[i], &g_dirty_rects[j])) {
                    g_dirty_rects[i] = merge_rects(&g_dirty_rects[i], &g_dirty_rects[j]);
                    // Shift left
                    for (uint32_t k = j; k < g_dirty_rect_count - 1; k++) {
                        g_dirty_rects[k] = g_dirty_rects[k + 1];
                    }
                    g_dirty_rect_count--;
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
    }
}

// ============================================================
// Occlusion Culling Evaluation
// ============================================================

static bool is_occluded(BWE_Window* win, uint32_t stack_index) {
    if (win->id == BWE_DESKTOP_ID) return false;

    // Check all windows above win in Z-order stack
    for (uint32_t j = stack_index + 1; j < g_z_stack_count; j++) {
        BWE_Window* above = BWE_GetWindow(g_z_order_stack[j]);
        if (!above) continue;

        // Skip non-visible or transparent/child windows
        if (above->state == BWE_STATE_HIDDEN || (above->flags & BWE_WINDOW_TRANSPARENT)) {
            continue;
        }

        // Standard bounding box full occlusion test
        if (above->screen_bounds.x <= win->screen_bounds.x &&
            above->screen_bounds.y <= win->screen_bounds.y &&
            above->screen_bounds.x + above->screen_bounds.width >= win->screen_bounds.x + win->screen_bounds.width &&
            above->screen_bounds.y + above->screen_bounds.height >= win->screen_bounds.y + win->screen_bounds.height) {
            return true; // Fully covered!
        }
    }
    return false;
}

// ============================================================
// Regional Framebuffer Swapping
// ============================================================

static void copy_dirty_regions(const BVFramebuffer* src, const BVFramebuffer* dest) {
    uint32_t src_pitch_w = src->pitch / 4;
    uint32_t dest_pitch_w = dest->pitch / 4;

    for (uint32_t r = 0; r < g_dirty_rect_count; r++) {
        BWE_Rect* d = &g_dirty_rects[r];
        int32_t x1 = d->x;
        int32_t y1 = d->y;
        int32_t x2 = d->x + d->width;
        int32_t y2 = d->y + d->height;

        // Clip to destination size
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 > (int32_t)dest->width) x2 = (int32_t)dest->width;
        if (y2 > (int32_t)dest->height) y2 = (int32_t)dest->height;

        for (int32_t y = y1; y < y2; y++) {
            uint32_t src_row_offset = y * src_pitch_w;
            uint32_t dest_row_offset = y * dest_pitch_w;
            for (int32_t x = x1; x < x2; x++) {
                dest->buffer[dest_row_offset + x] = src->buffer[src_row_offset + x];
            }
        }
    }
}

// ============================================================
// Software Composition Pipeline Execution
// ============================================================

static void compose_window_recursive(const BVFramebuffer* ram_fb, BWE_Window* win) {
    if (win->state == BWE_STATE_HIDDEN) return;

    // Direct paint callback invocation
    if (win->on_render) {
        win->on_render(win);
        s_paint_calls++;
    } else {
        // Fallback default native chrome frame renderer
        bool active = (win->id == g_focused_window_id);
        uint32_t border_color = active ? 0xFF0058EE : 0xFF475569; // Active Blue vs Inactive Gray
        
        // Draw Shadow if not desktop
        if (win->id != BWE_DESKTOP_ID && !(win->flags & BWE_WINDOW_BORDERLESS)) {
            BWE_DrawShadow(ram_fb, &win->screen_bounds);
            const char* title_text = (win->control_data.button.text[0] != '\0') ? win->control_data.button.text : (active ? "Active Window" : "Window");
            BWE_DrawTitleBar(ram_fb, &win->screen_bounds, title_text, active);
        } else if (win->id == BWE_DESKTOP_ID) {
            extern void Shell_DrawWallpaper(const BVFramebuffer* fb, const BWE_Rect* clip);
            BWE_Rect clip;
            if (BWE_GetClip(&clip)) {
                Shell_DrawWallpaper(ram_fb, &clip);
            } else {
                BWE_Rect full_rect = {0, 0, (int32_t)ram_fb->width, (int32_t)ram_fb->height};
                Shell_DrawWallpaper(ram_fb, &full_rect);
            }
        }
    }

    // Render child sub-surfaces in parent relative layout Z-order
    for (uint32_t i = 0; i < win->child_count; i++) {
        BWE_Window* child = BWE_GetWindow(win->children[i]);
        if (child) {
            // Push child client clipping rectangle
            BWE_Rect client_clip = win->screen_bounds;
            if (!(win->flags & BWE_WINDOW_BORDERLESS)) {
                // Adjust for 30px titlebar and 5px border
                client_clip.x += 5;
                client_clip.y += 35;
                client_clip.width -= 10;
                client_clip.height -= 40;
            }
            BWE_ClipPush(client_clip);
            compose_window_recursive(ram_fb, child);
            BWE_ClipPop();
        }
    }
}

uint32_t g_hud_open_windows = 0;
uint32_t g_hud_desktop_icons = 0;
uint32_t g_hud_focused_window = 0;
uint32_t g_hud_hovered_control = 0;
uint32_t g_hud_taskbar_buttons = 0;
uint32_t g_hud_notifications = 0;
uint32_t g_hud_memory_usage = 0;
bool g_hud_visible = true;

static void hud_itoa(uint32_t val, char* buf) {
    char temp[16];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0) buf[j++] = temp[--i];
    buf[j] = '\0';
}

static void draw_diagnostics_hud(const BVFramebuffer* fb) {
    if (!g_hud_visible) return;

    BWE_Rect hud_rect = { 10, 10, 360, 250 };
    BWE_FillRect(fb, hud_rect.x, hud_rect.y, hud_rect.width, hud_rect.height, 0xCC000000); // Semitransparent black panel
    BWE_DrawRect(fb, hud_rect.x, hud_rect.y, hud_rect.width, hud_rect.height, 0xFFFFFFFF, 1);

    BWE_DrawText(fb, "ATOMS OS - BWE V2.1 DESKTOP HUD", hud_rect.x + 10, hud_rect.y + 10, 0xFF00FF00, 0);
    
    char buf[64];
    char num_buf[16];
    extern uint32_t g_dirty_rect_count;
    extern void strcat(char* d, const char* s);
    extern void strcpy(char* d, const char* s);

    // FPS
    BWE_DrawText(fb, "FPS: 60.00 (Deterministic)", hud_rect.x + 10, hud_rect.y + 30, 0xFFFFFFFF, 0);

    // Open Windows
    strcpy(buf, "Open Windows: ");
    hud_itoa(g_hud_open_windows, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 50, 0xFFFFFFFF, 0);

    // Desktop Icons
    strcpy(buf, "Desktop Icons: ");
    hud_itoa(g_hud_desktop_icons, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 70, 0xFFFFFFFF, 0);

    // Taskbar Buttons
    strcpy(buf, "Taskbar Buttons: ");
    hud_itoa(g_hud_taskbar_buttons, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 90, 0xFFFFFFFF, 0);

    // Focused Window ID
    strcpy(buf, "Focused Window ID: #");
    hud_itoa(g_hud_focused_window, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 110, 0xFFFFFFFF, 0);

    // Hovered Control ID
    strcpy(buf, "Hovered Control ID: #");
    hud_itoa(g_hud_hovered_control, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 130, 0xFFFFFFFF, 0);

    // Active Notifications
    strcpy(buf, "Active Notifications: ");
    hud_itoa(g_hud_notifications, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 150, 0xFFFFFFFF, 0);

    // Memory Usage (Heap)
    strcpy(buf, "Kernel Heap Usage: ");
    typedef struct {
        uint32_t total_size;
        uint32_t used_size;
        uint32_t free_size;
        uint32_t block_count;
        uint32_t largest_free;
    } LocalHeapStats;
    extern void heap_get_stats(LocalHeapStats* stats);
    LocalHeapStats stats;
    heap_get_stats(&stats);
    uint32_t used_kb = stats.used_size / 1024;
    hud_itoa(used_kb, num_buf);
    strcat(buf, num_buf);
    strcat(buf, " KB");
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 170, 0xFFFFFFFF, 0);

    // Dirty Regions
    strcpy(buf, "Dirty Regions Count: ");
    hud_itoa(g_dirty_rect_count, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 190, 0xFFFFFFFF, 0);

    // Paint Calls
    strcpy(buf, "Paint Calls: ");
    hud_itoa(s_paint_calls, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 210, 0xFFFFFFFF, 0);

    // Total Surfaces
    uint32_t active_controls = 0;
    extern BWE_Window g_windows[];
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].id != 0) {
            active_controls++;
        }
    }
    strcpy(buf, "Active Surfaces: ");
    hud_itoa(active_controls, num_buf);
    strcat(buf, num_buf);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 230, 0xFFFFFFFF, 0);
}

void BWE_ComposeFrame(const BVFramebuffer* hw_fb) {
    if (!hw_fb) return;

    // Persistent tracking of window bounds between frames
    static BWE_Rect s_last_composed_bounds[BWE_MAX_WINDOWS];
    static bool s_last_composed_bounds_valid[BWE_MAX_WINDOWS] = { false };
    
    // First-frame full screen damage
    static bool s_first_frame = true;
    if (s_first_frame) {
        BWE_Rect full_screen = { 0, 0, (int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height };
        BWE_AddCompositorDirtyRect(&full_screen);
        s_first_frame = false;
    }

    // Collect all windows changed and populate damage regions
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        extern BWE_Window g_windows[];
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED) {
            if (win->is_dirty) {
                if (s_last_composed_bounds_valid[i]) {
                    BWE_Rect old_rect = s_last_composed_bounds[i];
                    if (win->id != BWE_DESKTOP_ID && !(win->flags & BWE_WINDOW_BORDERLESS)) {
                        old_rect.x -= 4;
                        old_rect.y -= 4;
                        old_rect.width += 8;
                        old_rect.height += 8;
                    }
                    BWE_AddCompositorDirtyRect(&old_rect);
                }
                BWE_Rect new_rect = win->screen_bounds;
                if (win->id != BWE_DESKTOP_ID && !(win->flags & BWE_WINDOW_BORDERLESS)) {
                    new_rect.x -= 4;
                    new_rect.y -= 4;
                    new_rect.width += 8;
                    new_rect.height += 8;
                }
                BWE_AddCompositorDirtyRect(&new_rect);
                win->is_dirty = false;
            }
        } else {
            // If the window was destroyed, invalidate its last composed bounds so it is erased from the screen
            if (s_last_composed_bounds_valid[i]) {
                BWE_Rect old_rect = s_last_composed_bounds[i];
                old_rect.x -= 4;
                old_rect.y -= 4;
                old_rect.width += 8;
                old_rect.height += 8;
                BWE_AddCompositorDirtyRect(&old_rect);
                s_last_composed_bounds_valid[i] = false;
            }
        }
    }

    // Add mouse cursor damage regions (32x32 pixels) to trigger compositor redraw
    extern int32_t g_bwe_mouse_x;
    extern int32_t g_bwe_mouse_y;
    static int32_t s_last_compose_mouse_x = -9999;
    static int32_t s_last_compose_mouse_y = -9999;

    if (g_bwe_mouse_x != s_last_compose_mouse_x || g_bwe_mouse_y != s_last_compose_mouse_y) {
        if (s_last_compose_mouse_x != -9999) {
            BWE_Rect old_mouse_rect = { s_last_compose_mouse_x, s_last_compose_mouse_y, 32, 32 };
            BWE_AddCompositorDirtyRect(&old_mouse_rect);
        }
        BWE_Rect new_mouse_rect = { g_bwe_mouse_x, g_bwe_mouse_y, 32, 32 };
        BWE_AddCompositorDirtyRect(&new_mouse_rect);

        s_last_compose_mouse_x = g_bwe_mouse_x;
        s_last_compose_mouse_y = g_bwe_mouse_y;
    }

    // If no damage, skip rendering pass entirely
    if (g_dirty_rect_count == 0) {
        return;
    }

    // Merge overlapping dirty boxes
    BWE_MergeDirtyRects();

    // Setup temporary RAM Framebuffer
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = g_kernel_screen_width;
    ram_fb.height = g_kernel_screen_height;
    ram_fb.pitch = g_kernel_screen_width * 4;

    s_paint_calls = 0;
    BWE_SetRenderTarget(&ram_fb);

    // Compose frame for each merged dirty rectangle region separately
    for (uint32_t d = 0; d < g_dirty_rect_count; d++) {
        BWE_Rect current_dirty = g_dirty_rects[d];
        
        // Push region boundary clip
        g_clip_stack_depth = 0;
        BWE_ClipPush(current_dirty);

        // Compositing pass (Bottom-to-Top scan using Z-order Stack)
        for (uint32_t i = 0; i < g_z_stack_count; i++) {
            BWE_Window* win = BWE_GetWindow(g_z_order_stack[i]);
            if (!win || win->state == BWE_STATE_HIDDEN) continue;

            // Occlusion checking
            if (is_occluded(win, i)) {
                continue; // Skip rendering occluded windows!
            }

            // Verify if window intersects the current dirty area bounds
            if (win->screen_bounds.x + win->screen_bounds.width < current_dirty.x ||
                win->screen_bounds.x > current_dirty.x + current_dirty.width ||
                win->screen_bounds.y + win->screen_bounds.height < current_dirty.y ||
                win->screen_bounds.y > current_dirty.y + current_dirty.height) {
                continue; // Outside dirty bounds, SKIP!
            }

            // Push window clip rectangle
            BWE_ClipPush(win->screen_bounds);
            compose_window_recursive(&ram_fb, win);
            BWE_ClipPop();
        }

        BWE_ClipPop();
    }

    // Draw diagnostic overlays on backbuffer
    draw_diagnostics_hud(&ram_fb);

    // Flush BOIMAGE v2 batched sprite draw calls (crucial for text to render)
    extern void BOImage_BOHeartTickFlush(void);
    BOImage_BOHeartTickFlush();

    // Draw mouse cursor on backbuffer
    extern int32_t g_bwe_mouse_x;
    extern int32_t g_bwe_mouse_y;
    extern void BVCursor_Draw(int32_t cx, int32_t cy);
    BVCursor_Draw(g_bwe_mouse_x, g_bwe_mouse_y);

    // Swap backbuffer RAM to physical double buffer back page
    BVFramebuffer back_vram = vbe_get_back_page();
    extern void BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb);
    BOVISUAL_Graphics_SwapFull(&back_vram);

    // Swap display page
    vbe_swap_page();

    BWE_SetRenderTarget(0);

    // Update last composed bounds for next frame
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        extern BWE_Window g_windows[];
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED) {
            s_last_composed_bounds[i] = win->screen_bounds;
            s_last_composed_bounds_valid[i] = true;
        } else {
            s_last_composed_bounds_valid[i] = false;
        }
    }

    // Clear damage tracker
    g_dirty_rect_count = 0;
}
