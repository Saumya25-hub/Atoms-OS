#include "../include/bwe.h"
#include "kernel/ame/include/ame.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/graphics/BSPE/include/bspe.h"
#include "kernel/drivers/input/cursor/cursor_hotspot.h"
#include "kernel/graphics/BSPE/Cursor/bspe_cursor_present.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/core/lib/include/string.h"
#include <stddef.h>
#include "kernel/performance/include/profiler.h"


typedef struct BOS_Surface {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t memory_size;
    void* memory_ptr;
    bool dirty;
} BOS_Surface;

#define BWE_MAX_CACHE_SURFACES 64
static BOS_Surface g_surface_cache_pool[BWE_MAX_CACHE_SURFACES];
static uint32_t g_surface_cache_count = 0;

static BOS_Surface* BSCE_Pool_GetSlot(uint32_t surface_id) {
    for (uint32_t i = 0; i < g_surface_cache_count; i++) {
        if (g_surface_cache_pool[i].id == surface_id) {
            return &g_surface_cache_pool[i];
        }
    }
    return NULL;
}

void BSCE_Pool_FreeSlot(uint32_t surface_id) {
    for (uint32_t i = 0; i < g_surface_cache_count; i++) {
        if (g_surface_cache_pool[i].id == surface_id) {
            extern void kfree(void* ptr);
            if (g_surface_cache_pool[i].memory_ptr) {
                kfree(g_surface_cache_pool[i].memory_ptr);
                g_surface_cache_pool[i].memory_ptr = NULL;
            }
            for (uint32_t j = i; j < g_surface_cache_count - 1; j++) {
                g_surface_cache_pool[j] = g_surface_cache_pool[j + 1];
            }
            g_surface_cache_count--;
            break;
        }
    }
}

void BSCE_MarkSlotDirty(uint32_t surface_id) {
    BOS_Surface* s = BSCE_Pool_GetSlot(surface_id);
    if (s) {
        s->dirty = true;
    }
}

void BWE_InvalidateAllSurfaces(void) {
    for (uint32_t i = 0; i < g_surface_cache_count; i++) {
        g_surface_cache_pool[i].dirty = true;
    }
}


static BOS_Surface* Surface_CreateForWindow(uint32_t window_id, uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return NULL;
    
    BOS_Surface* existing = BSCE_Pool_GetSlot(window_id);
    if (existing) {
        if (existing->width != width || existing->height != height) {
            extern void kfree(void* ptr);
            if (existing->memory_ptr) kfree(existing->memory_ptr);
            extern void* kmalloc(uint32_t size);
            uint32_t buf_size = width * height * sizeof(uint32_t);
            existing->memory_ptr = kmalloc(buf_size);
            existing->width = width;
            existing->height = height;
            existing->stride = width * sizeof(uint32_t);
            existing->memory_size = buf_size;
            existing->dirty = true;
        }
        return existing;
    }

    if (g_surface_cache_count >= BWE_MAX_CACHE_SURFACES) return NULL;

    extern void* kmalloc(uint32_t size);
    uint32_t buf_size = width * height * sizeof(uint32_t);
    void* ptr = kmalloc(buf_size);
    if (!ptr) return NULL;

    BOS_Surface* slot = &g_surface_cache_pool[g_surface_cache_count++];
    slot->id = window_id;
    slot->width = width;
    slot->height = height;
    slot->stride = width * sizeof(uint32_t);
    slot->memory_size = buf_size;
    slot->memory_ptr = ptr;
    slot->dirty = true;

    return slot;
}

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

volatile uint64_t g_instrument_frame_id = 0;

#ifndef BWE_RUNTIME_TRACE
#define BWE_RUNTIME_TRACE 0
#endif

#ifndef BWE_ENABLE_RENDER_TRACE
#define BWE_ENABLE_RENDER_TRACE 0
#endif

// ============================================================
// Full Redraw Request (used by debug console toggle, etc.)
// ============================================================
static volatile bool s_full_redraw_requested = false;

void BWE_RequestFullRedraw(void) {
    s_full_redraw_requested = true;
}

void inst_print_event(const char* event) {
#if BWE_RUNTIME_TRACE
    extern void display_print(const char*);
    extern void display_print_dec(uint32_t);
    display_print("[FRAME ");
    display_print_dec((uint32_t)g_instrument_frame_id);
    display_print("] ");
    display_print(event);
    display_print("\n");
#else
    (void)event;
#endif
}

void inst_print_ptr(const char* name, void* ptr) {
#if BWE_RUNTIME_TRACE
    extern void display_print(const char*);
    extern void display_print_hex(uint64_t);
    display_print(name);
    display_print(" : 0x");
    display_print_hex((uint64_t)(uintptr_t)ptr);
    display_print("\n");
#else
    (void)name;
    (void)ptr;
#endif
}

void inst_print_val(const char* name, uint32_t val) {
#if BWE_RUNTIME_TRACE
    extern void display_print(const char*);
    extern void display_print_dec(uint32_t);
    display_print(name);
    display_print(" : ");
    display_print_dec(val);
    display_print("\n");
#else
    (void)name;
    (void)val;
#endif
}

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

BWE_Rect g_dirty_rects[BWE_MAX_DIRTY_RECTS];
uint32_t g_dirty_rect_count = 0;

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
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    if (g_dirty_rect_count >= BWE_MAX_DIRTY_RECTS) {
        // Fallback to full screen damage
        g_dirty_rect_count = 1;
        g_dirty_rects[0].x = 0;
        g_dirty_rects[0].y = 0;
        g_dirty_rects[0].width = (int32_t)BOVISUAL_Graphics_GetWidth();
        g_dirty_rects[0].height = (int32_t)BOVISUAL_Graphics_GetHeight();
        return;
    }

    // Clip to screen boundaries
    int32_t cx1 = (rect->x > 0) ? rect->x : 0;
    int32_t cy1 = (rect->y > 0) ? rect->y : 0;
    int32_t rx2 = rect->x + rect->width;
    int32_t ry2 = rect->y + rect->height;
    int32_t sx2 = (int32_t)BOVISUAL_Graphics_GetWidth();
    int32_t sy2 = (int32_t)BOVISUAL_Graphics_GetHeight();
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
    if (!src || !src->buffer || !dest || !dest->buffer) return;

    uint32_t src_max_pixels = src->width * src->height;
    uint32_t dest_max_pixels = dest->width * dest->height;

    uint32_t src_pitch_w = (src->pitch > 0 && (src->pitch / 4) <= src->width) ? (src->pitch / 4) : src->width;
    uint32_t dest_pitch_w = (dest->pitch > 0 && (dest->pitch / 4) <= dest->width) ? (dest->pitch / 4) : dest->width;

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
        if (x2 > (int32_t)src->width) x2 = (int32_t)src->width;
        if (y2 > (int32_t)src->height) y2 = (int32_t)src->height;

        for (int32_t y = y1; y < y2; y++) {
            uint32_t src_row_offset = (uint32_t)y * src_pitch_w;
            uint32_t dest_row_offset = (uint32_t)y * dest_pitch_w;
            for (int32_t x = x1; x < x2; x++) {
                uint32_t s_idx = src_row_offset + (uint32_t)x;
                uint32_t d_idx = dest_row_offset + (uint32_t)x;
                if (d_idx < dest_max_pixels && s_idx < src_max_pixels) {
                    dest->buffer[d_idx] = src->buffer[s_idx];
                }
            }
        }
    }
}

// ============================================================
// Software Composition Pipeline Execution
// ============================================================

static void compose_window_recursive(const BVFramebuffer* ram_fb, BWE_Window* win) {
    if (!win || win->state == BWE_STATE_HIDDEN) return;

    // Desktop wallpaper rendering
    if (win->id == BWE_DESKTOP_ID) {
        extern void Shell_DrawWallpaper(const BVFramebuffer* fb, const BWE_Rect* clip);
        BWE_Rect clip;
        BWE_Rect full_rect = {0, 0, (int32_t)ram_fb->width, (int32_t)ram_fb->height};
        if (BWE_GetClip(&clip)) {
            Shell_DrawWallpaper(ram_fb, &clip);
        } else {
            Shell_DrawWallpaper(ram_fb, &full_rect);
        }
        win->is_dirty = false;
        return;
    }

    // 1. BSCE Surface Cache Lookup


    BOS_Surface* cached = BSCE_Pool_GetSlot(win->id);
    if (!cached || !cached->memory_ptr || cached->width != (uint32_t)win->screen_bounds.width || cached->height != (uint32_t)win->screen_bounds.height) {
        cached = Surface_CreateForWindow(win->id, (uint32_t)win->screen_bounds.width, (uint32_t)win->screen_bounds.height);
    }


    bool cache_hit = (cached != NULL && cached->memory_ptr != NULL && cached->memory_size > 0);
    bool is_dirty = win->is_dirty || (cached ? cached->dirty : true) || (win->type == BWE_TYPE_DESKTOP_ICON);

#if BWE_ENABLE_RENDER_TRACE
    if (win->type == BWE_TYPE_LABEL || win->type == BWE_TYPE_BUTTON || win->is_dirty) {
        extern void serial_write_direct(const char* str);
        extern void serial_write_dec_direct(int val);
        serial_write_direct("[RENDER_TRACE 4] compose_window_recursive ID=");
        serial_write_dec_direct((int)win->id);
        serial_write_direct(" is_dirty=");
        serial_write_direct(is_dirty ? "TRUE" : "FALSE");
        serial_write_direct(" cache_hit=");
        serial_write_direct(cache_hit ? "TRUE" : "FALSE");
        serial_write_direct("\n");
    }
#endif

    // ------------------------------------------------------------
    // RETAINED-MODE FAST PATH (Surface Cache Blit)
    // ------------------------------------------------------------
    if (!is_dirty && cache_hit) {
        BWE_Rect clip;
        if (BWE_GetClip(&clip)) {
            uint32_t win_w = (uint32_t)win->screen_bounds.width;
            uint32_t win_h = (uint32_t)win->screen_bounds.height;
            uint32_t fb_pitch_w = ram_fb->pitch / 4;
            uint32_t src_stride_w = (cached && cached->stride > 0) ? (cached->stride / 4) : win_w;
            uint32_t* src_buf = (uint32_t*)cached->memory_ptr;

            int32_t x1 = win->screen_bounds.x;
            int32_t y1 = win->screen_bounds.y;
            int32_t x2 = x1 + win->screen_bounds.width;
            int32_t y2 = y1 + win->screen_bounds.height;

            if (x1 < clip.x) x1 = clip.x;
            if (y1 < clip.y) y1 = clip.y;
            if (x2 > clip.x + clip.width) x2 = clip.x + clip.width;
            if (y2 > clip.y + clip.height) y2 = clip.y + clip.height;

            if (x1 < x2 && y1 < y2) {
                uint32_t copy_w = (uint32_t)(x2 - x1);
                uint32_t copy_bytes = copy_w * sizeof(uint32_t);

                for (int32_t cy = y1; cy < y2; cy++) {
                    int32_t src_y = cy - win->screen_bounds.y;
                    int32_t src_x = x1 - win->screen_bounds.x;
                    if (src_y >= 0 && src_y < (int32_t)win_h && src_x >= 0 && src_x < (int32_t)win_w) {
                        uint32_t dest_idx = cy * fb_pitch_w + x1;
                        uint32_t src_idx = src_y * src_stride_w + src_x;
                        memcpy(&ram_fb->buffer[dest_idx], &src_buf[src_idx], copy_bytes);
                    }
                }
            }
        }
        return; // SKIP ALL CPU REPAINTS & CHILD RECURSIONS!
    }

    // ------------------------------------------------------------
    // REPAINT PATH & POST-PAINT CACHE CAPTURE
    // ------------------------------------------------------------

    BWE_Rect effective_clip;
    bool has_clip = BWE_GetClip(&effective_clip);
    bool full_coverage = false;
    if (has_clip) {
        full_coverage = (effective_clip.x <= win->screen_bounds.x &&
                         effective_clip.y <= win->screen_bounds.y &&
                         effective_clip.x + effective_clip.width >= win->screen_bounds.x + win->screen_bounds.width &&
                         effective_clip.y + effective_clip.height >= win->screen_bounds.y + win->screen_bounds.height);
    } else {
        full_coverage = true;
    }

    // Draw Shadow & Chrome Frame if not desktop
    if (!(win->flags & BWE_WINDOW_BORDERLESS)) {
        bool active = (win->id == g_focused_window_id);
        uint32_t border_color = active ? 0xFF0058EE : 0xFF475569;
        BWE_DrawShadow(ram_fb, &win->screen_bounds, active);
        BWE_DrawBorder(ram_fb, &win->screen_bounds, border_color, active);
        const char* title_text = (win->title[0] != '\0') ? win->title : (active ? "Active Window" : "Window");
        bool resizable = (win->flags & BWE_WINDOW_RESIZABLE) != 0;
        BWE_DrawTitleBar(ram_fb, &win->screen_bounds, title_text, active, resizable);

        // Fill client area background so child controls don't render
        // on top of stale wallpaper/garbage pixels
        uint32_t frame_bg = active ? BOTHEME_GetColor(BOTHEME_FRAME_BG_ACTIVE)
                                   : BOTHEME_GetColor(BOTHEME_FRAME_BG_INACTIVE);
        int32_t cx = win->screen_bounds.x + 5;
        int32_t cy = win->screen_bounds.y + 35;
        int32_t cw = win->screen_bounds.width - 10;
        int32_t ch = win->screen_bounds.height - 40;
        if (cw > 0 && ch > 0) {
            uint32_t bg2 = (win->gradient_mode != 0) ? win->gradient_color_end : frame_bg;
            BWE_FillRectEx(ram_fb, cx, cy, cw, ch, frame_bg, bg2, win->gradient_mode, win->corner_radius);
        }
    }

    /* Blit user-space private window surface if allocated */
    if (win->control_data.canvas.pixel_buffer) {
        int32_t cx = win->screen_bounds.x + 5;
        int32_t cy = win->screen_bounds.y + 35;
        int32_t cw = win->screen_bounds.width - 10;
        int32_t ch = win->screen_bounds.height - 40;
        if (win->flags & BWE_WINDOW_BORDERLESS) {
            cx = win->screen_bounds.x;
            cy = win->screen_bounds.y;
            cw = win->screen_bounds.width;
            ch = win->screen_bounds.height;
        }
        uint32_t bw = win->control_data.canvas.buffer_w;
        uint32_t bh = win->control_data.canvas.buffer_h;
        if (cw > (int32_t)bw) cw = (int32_t)bw;
        if (ch > (int32_t)bh) ch = (int32_t)bh;

        const uint32_t* src = win->control_data.canvas.pixel_buffer;
        for (int32_t row = 0; row < ch; row++) {
            int32_t dst_y = cy + row;
            if (dst_y < 0 || dst_y >= (int32_t)ram_fb->height) continue;
            for (int32_t col = 0; col < cw; col++) {
                int32_t dst_x = cx + col;
                if (dst_x < 0 || dst_x >= (int32_t)ram_fb->width) continue;
                uint32_t pixel = src[row * bw + col];
                if ((pixel >> 24) > 0) {
                    ram_fb->buffer[dst_y * (ram_fb->pitch / 4) + dst_x] = pixel;
                }
            }
        }
        static bool s_surface_composite_logged = false;
        if (!s_surface_composite_logged) {
            s_surface_composite_logged = true;
            extern void com1_puts(const char* s);
            com1_puts("[BWE_GUI] SURFACE COMPOSITE PASS\r\n");
        }
    }

    // Invoke custom on_render callback if present
    if (win->on_render) {
#if BWE_ENABLE_RENDER_TRACE
        extern void serial_write_direct(const char* str);
        extern void serial_write_dec_direct(int val);
        serial_write_direct("[RENDER_TRACE 5] Executing on_render for ID=");
        serial_write_dec_direct((int)win->id);
        serial_write_direct("\n");
#endif
        win->on_render(win);
        s_paint_calls++;
    }

    extern void BOImage_BOHeartTickFlush(void);
    BOImage_BOHeartTickFlush();

    // Render child sub-surfaces in parent relative layout Z-order
    for (uint32_t i = 0; i < win->child_count; i++) {
        BWE_Window* child = BWE_GetWindow(win->children[i]);
        if (child) {
            BWE_Rect client_clip = win->screen_bounds;
            if (!(win->flags & BWE_WINDOW_BORDERLESS)) {
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

    // Capture painted pixels into BSCE surface backing buffer
    if (full_coverage && cached && cached->memory_ptr && win->screen_bounds.width > 0 && win->screen_bounds.height > 0 && cached->width == (uint32_t)win->screen_bounds.width && cached->height == (uint32_t)win->screen_bounds.height) {

        uint32_t win_w = (uint32_t)win->screen_bounds.width;
        uint32_t win_h = (uint32_t)win->screen_bounds.height;
        uint32_t fb_pitch_w = ram_fb->pitch / 4;
        uint32_t dest_stride_w = (cached->stride > 0) ? (cached->stride / 4) : win_w;
        uint32_t* dest_buf = (uint32_t*)cached->memory_ptr;

        for (uint32_t y = 0; y < win_h; y++) {
            int32_t fb_y = win->screen_bounds.y + (int32_t)y;
            if (fb_y >= 0 && fb_y < (int32_t)ram_fb->height) {
                int32_t fb_x = win->screen_bounds.x;
                if (fb_x >= 0 && fb_x + (int32_t)win_w <= (int32_t)ram_fb->width) {
                    memcpy(&dest_buf[y * dest_stride_w], &ram_fb->buffer[fb_y * fb_pitch_w + fb_x], win_w * sizeof(uint32_t));
                }
            }
        }
        cached->dirty = false;
    }

    if (full_coverage) {
        win->is_dirty = false;
    }
}

uint32_t g_hud_open_windows = 0;
uint32_t g_hud_desktop_icons = 0;
uint32_t g_hud_focused_window = 0;
uint32_t g_hud_hovered_control = 0;
uint32_t g_hud_taskbar_buttons = 0;
uint32_t g_hud_notifications = 0;
uint32_t g_hud_memory_usage = 0;
bool g_hud_visible = false;

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

    BWE_Rect hud_rect = { 10, 10, 360, 320 };
    BWE_FillRect(fb, hud_rect.x, hud_rect.y, hud_rect.width, hud_rect.height, 0xCC000000); // Semitransparent black panel
    BWE_DrawRect(fb, hud_rect.x, hud_rect.y, hud_rect.width, hud_rect.height, 0xFFFFFFFF, 1);

    BWE_DrawText(fb, "ATOMS OS - BWE V2.1 DESKTOP HUD", hud_rect.x + 10, hud_rect.y + 10, 0xFF00FF00, 0);
    
    char buf[64];
    char num_buf[16];
    extern uint32_t g_dirty_rect_count;

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

    extern char g_kbd_selected_icon_name[64];
    extern int32_t g_kbd_selected_icon_index;
    extern uint32_t g_kbd_selected_app_id;

    strcpy(buf, "Selected Icon: ");
    strcat(buf, g_kbd_selected_icon_index == -1 ? "NONE" : g_kbd_selected_icon_name);
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 250, 0xFFFFFF00, 0); // Yellow

    strcpy(buf, "Selected Index: ");
    if (g_kbd_selected_icon_index == -1) strcat(buf, "NONE");
    else { hud_itoa((uint32_t)g_kbd_selected_icon_index, num_buf); strcat(buf, num_buf); }
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 270, 0xFFFFFF00, 0);

    strcpy(buf, "Selected App ID: ");
    if (g_kbd_selected_icon_index == -1) strcat(buf, "NONE");
    else { hud_itoa(g_kbd_selected_app_id, num_buf); strcat(buf, num_buf); }
    BWE_DrawText(fb, buf, hud_rect.x + 10, hud_rect.y + 290, 0xFFFFFF00, 0);
}

volatile uint64_t g_frames_presented_count = 0;

void BWE_ComposeFrame(const BVFramebuffer* hw_fb) {
    if (!hw_fb) return;
    /* STEP 14 TEMPORARY INSTRUMENTATION */
    uint64_t comp_start_tsc = step14_rdtsc();
    /* END STEP 14 */

    // Advance all active ATOMS Motion Engine (AME) animations for this frame
    AME_Tick(0);

    // Persistent tracking of window bounds between frames
    static BWE_Rect s_last_composed_bounds[BWE_MAX_WINDOWS];
    static bool s_last_composed_bounds_valid[BWE_MAX_WINDOWS] = { false };
    
    // First-frame full screen damage
    static bool s_first_frame = true;
    static int s_boot_force_redraws = 10;
    
    extern void inst_print_event(const char*);
    extern void inst_print_ptr(const char*, void*);
    extern void inst_print_val(const char*, uint32_t);
    extern volatile uint64_t g_instrument_frame_id;

    g_instrument_frame_id++;

    bos_profiler_frame_begin();

    // inst_print_event("BWE_ComposeFrame START");
    // inst_print_val("frame id", (uint32_t)g_instrument_frame_id);
    // inst_print_val("timestamp", (uint32_t)timer_get_ticks());
    // inst_print_val("dirty rect count", g_dirty_rect_count);

    if (s_boot_force_redraws > 0) {
        s_full_redraw_requested = true;
        s_boot_force_redraws--;
    }

    extern bool Desktop_Shell_IsBootExperienceActive(void);
    if (s_first_frame || s_full_redraw_requested || Desktop_Shell_IsBootExperienceActive() || AME_IsBootExperienceActive()) {
        extern uint32_t BOVISUAL_Graphics_GetWidth(void);
        extern uint32_t BOVISUAL_Graphics_GetHeight(void);
        BWE_Rect full_screen = { 0, 0, (int32_t)BOVISUAL_Graphics_GetWidth(), (int32_t)BOVISUAL_Graphics_GetHeight() };
        BWE_AddCompositorDirtyRect(&full_screen);
        s_first_frame = false;
        s_full_redraw_requested = false;
    }

    // Collect all windows changed and populate damage regions
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        extern BWE_Window g_windows[];
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED) {
            if (win->is_dirty) {
#if BWE_ENABLE_RENDER_TRACE
                extern void serial_write_direct(const char* str);
                extern void serial_write_dec_direct(int val);
                serial_write_direct("[RENDER_TRACE 3] Compositor detected win->is_dirty=true for ID=");
                serial_write_dec_direct((int)win->id);
                serial_write_direct("\n");
#endif
                if (s_last_composed_bounds_valid[i]) {
                    BWE_Rect old_rect = s_last_composed_bounds[i];
                    BWE_AddCompositorDirtyRect(&old_rect);
                }
                BWE_Rect new_rect = win->screen_bounds;
                BWE_AddCompositorDirtyRect(&new_rect);
                // win->is_dirty is cleared in compose_window_recursive AFTER rendering completes!
            }
        } else {
            // If the window was destroyed, invalidate its last composed bounds so it is erased from the screen
            if (s_last_composed_bounds_valid[i]) {
                BWE_Rect old_rect = s_last_composed_bounds[i];
                BWE_AddCompositorDirtyRect(&old_rect);
                s_last_composed_bounds_valid[i] = false;
            }
        }
    }

    // Add mouse cursor damage regions accurately to trigger compositor redraw
    extern bool g_bspe_cursor_fast_path_enabled;
    if (!g_bspe_cursor_fast_path_enabled) {
        extern int32_t g_bwe_mouse_x;
        extern int32_t g_bwe_mouse_y;
        static CursorBoundingBox s_old_box = {0};
        static bool s_has_old_box = false;

        BSPE_CursorPresenterState cursor_state;
        cursor_state.is_initialized = false;
        BSPE_CursorPresenter_GetState(&cursor_state);

        if (cursor_state.is_initialized && cursor_state.visible) {
            CursorBoundingBox new_box;
            cursor_hotspot_calculate_box(cursor_state.current_x, cursor_state.current_y, cursor_state.width, cursor_state.height, cursor_state.hotspot_x, cursor_state.hotspot_y, cursor_state.scale_percent, g_kernel_screen_width, g_kernel_screen_height, &new_box);

            bool moved_or_changed = !s_has_old_box || s_old_box.draw_x != new_box.draw_x || s_old_box.draw_y != new_box.draw_y || s_old_box.draw_w != new_box.draw_w || s_old_box.draw_h != new_box.draw_h;

            if (moved_or_changed) {
                extern volatile uint64_t g_cursor_damage_requests_count;
                g_cursor_damage_requests_count++;

                if (s_has_old_box) {
                    BWE_Rect old_mouse_rect = { s_old_box.draw_x, s_old_box.draw_y, s_old_box.draw_w, s_old_box.draw_h };
                    BWE_AddCompositorDirtyRect(&old_mouse_rect);
                }
                BWE_Rect new_mouse_rect = { new_box.draw_x, new_box.draw_y, new_box.draw_w, new_box.draw_h };
                BWE_AddCompositorDirtyRect(&new_mouse_rect);

                s_old_box = new_box;
                s_has_old_box = true;
            }
        } else {
            // Fallback for uninitialized BSPE (legacy behavior)
            static int32_t s_last_compose_mouse_x = -9999;
            static int32_t s_last_compose_mouse_y = -9999;
            if (g_bwe_mouse_x != s_last_compose_mouse_x || g_bwe_mouse_y != s_last_compose_mouse_y) {
                extern volatile uint64_t g_cursor_damage_requests_count;
                g_cursor_damage_requests_count++;

                if (s_last_compose_mouse_x != -9999) {
                    BWE_Rect old_mouse_rect = { s_last_compose_mouse_x, s_last_compose_mouse_y, 32, 32 };
                    BWE_AddCompositorDirtyRect(&old_mouse_rect);
                }
                BWE_Rect new_mouse_rect = { g_bwe_mouse_x, g_bwe_mouse_y, 32, 32 };
                BWE_AddCompositorDirtyRect(&new_mouse_rect);

                s_last_compose_mouse_x = g_bwe_mouse_x;
                s_last_compose_mouse_y = g_bwe_mouse_y;
            }
        }
    }

    // If no damage, skip rendering pass entirely
    if (g_dirty_rect_count == 0) {
        return;
    }

    extern void BSPE_CursorPresenter_BeginComposition(void);
    BSPE_CursorPresenter_BeginComposition();

    // Merge overlapping dirty boxes
    BWE_MergeDirtyRects();

    extern uint32_t BOVISUAL_Graphics_GetPitch(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    // Setup temporary RAM Framebuffer
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();

    s_paint_calls = 0;
    BWE_SetRenderTarget(&ram_fb);

    // Compose frame for each merged dirty rectangle region separately
    for (uint32_t d = 0; d < g_dirty_rect_count; d++) {
        BWE_Rect current_dirty = g_dirty_rects[d];
        
        // Push region boundary clip
        g_clip_stack_depth = 0;
        BWE_ClipPush(current_dirty);

        // Compositing pass (Bottom-to-Top scan using Z-order Stack)
        extern bool Desktop_Shell_IsBootExperienceActive(void);
        if (Desktop_Shell_IsBootExperienceActive()) {
            // Skip compositing desktop windows/icons while Boot or Login page is active
            BWE_ClipPop();
            continue;
        }

        for (uint32_t i = 0; i < g_z_stack_count; i++) {
            BWE_Window* win = BWE_GetWindow(g_z_order_stack[i]);
            if (!win || win->state == BWE_STATE_HIDDEN) continue;

            // Only initiate rendering from top-level windows (and Desktop)
            if (win->id != BWE_DESKTOP_ID && win->parent_id != BWE_DESKTOP_ID) continue;

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
            // inst_print_event("Windows Draw");
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

    // Call Shell Post Compose Hook (used for Boot Experience and Selection Overlay)
    extern void Shell_PostComposeHook(const BVFramebuffer* fb);
    Shell_PostComposeHook(&ram_fb);

    extern void BSPE_CursorPresenter_EndComposition(void);
    extern bool g_bspe_cursor_fast_path_enabled;
    if (g_bspe_cursor_fast_path_enabled) {
        BSPE_CursorPresenter_EndComposition();
    } else {
        // Draw mouse cursor on backbuffer (Legacy path)
        extern int32_t g_bwe_mouse_x;
        extern int32_t g_bwe_mouse_y;
        extern void BVCursor_Draw(int32_t cx, int32_t cy);
        /* STEP 14 TEMPORARY INSTRUMENTATION */
        uint64_t cur_start_tsc = step14_rdtsc();
        /* END STEP 14 */
        
        /* STEP 17: Software Cursor Retirement */
        extern bool cursor_backend_is_hardware(void);
        if (!cursor_backend_is_hardware()) {
            BVCursor_Draw(g_bwe_mouse_x, g_bwe_mouse_y);
        }
        
        /* STEP 14 TEMPORARY INSTRUMENTATION */
        uint64_t cur_end_tsc = step14_rdtsc();
        step14_log_cursor_draw(step14_cycles_to_us(cur_end_tsc - cur_start_tsc));
        /* END STEP 14 */
    }

    // Swap backbuffer RAM to physical double buffer back page
    extern BVFramebuffer* vbe_get_back_page_ptr(void);
    BVFramebuffer* back_vram_ptr = vbe_get_back_page_ptr();
    extern void BOVISUAL_Graphics_SwapFull(const BVFramebuffer* hw_fb);
    g_frames_presented_count++;
    
    // inst_print_ptr("ram_fb pointer", ram_fb.buffer);
    // inst_print_ptr("front buffer pointer", vbe_get_framebuffer()->buffer);
    // inst_print_ptr("back buffer pointer", back_vram_ptr->buffer);
    
    // inst_print_event("SwapFull Queue");
    BOVISUAL_Graphics_SwapFull(back_vram_ptr);

    // Swap display page ONLY if AGDTE is not handling it
    extern bool AGDTE_IsInitialized(void);
    if (!AGDTE_IsInitialized()) {
        vbe_swap_page();
    }

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

    /* STEP 14 TEMPORARY INSTRUMENTATION */
    uint64_t comp_end_tsc = step14_rdtsc();
    step14_log_compositor_done(g_dirty_rect_count, step14_cycles_to_us(comp_end_tsc - comp_start_tsc));
    /* END STEP 14 */
    // Clear damage tracker
    g_dirty_rect_count = 0;
    bos_profiler_frame_end();
}

