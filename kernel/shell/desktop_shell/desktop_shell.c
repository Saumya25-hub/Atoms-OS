#include "desktop_shell.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/ui/task_panel.h"
#include "kernel/ui/start_menu.h"
#include "kernel/media/bopawn/wallpaper/wallpaper_manager.h"

// Telemetry counters
extern uint32_t g_hud_open_windows;
extern uint32_t g_hud_desktop_icons;
extern uint32_t g_hud_focused_window;
extern uint32_t g_hud_hovered_control;
extern uint32_t g_hud_taskbar_buttons;
extern uint32_t g_hud_notifications;
extern bool g_hud_visible;

// Screen Resolution
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

// Notification Queue
#define MAX_NOTIFICATIONS 4
typedef struct {
    char title[64];
    char message[128];
    uint64_t expire_ticks;
    bool active;
} ShellNotification;
static ShellNotification s_notifications[MAX_NOTIFICATIONS];

// Wallpaper config
#include "kernel/gui/surface/surface.h"

static struct BOSSurface* g_desktop_wallpaper = NULL;
static struct BOSSurface* g_old_desktop_wallpaper = NULL;
static uint32_t g_wallpaper_fade_alpha = 255;
uint32_t g_wallpaper_bg_color = 0xFF0B1120;

void desktop_set_wallpaper(struct BOSSurface* surface) {
    if (g_desktop_wallpaper && g_desktop_wallpaper != surface) {
        surface_destroy(g_desktop_wallpaper);
    }
    g_desktop_wallpaper = surface;
    g_wallpaper_fade_alpha = 255;
}

void desktop_set_wallpaper_transition(struct BOSSurface* old_surface, struct BOSSurface* new_surface) {
    g_old_desktop_wallpaper = old_surface;
    g_desktop_wallpaper = new_surface;
    g_wallpaper_fade_alpha = 0;
}

void desktop_set_wallpaper_alpha(uint32_t alpha) {
    g_wallpaper_fade_alpha = alpha;
}

void desktop_end_wallpaper_transition(void) {
    if (g_old_desktop_wallpaper) {
        surface_destroy(g_old_desktop_wallpaper);
        g_old_desktop_wallpaper = NULL;
    }
    g_wallpaper_fade_alpha = 255;
}

struct BOSSurface* desktop_get_wallpaper(void) {
    return g_desktop_wallpaper;
}

void desktop_refresh_background(void) {
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
}

void Shell_DrawWallpaper(const BVFramebuffer* fb, const BWE_Rect* clip) {
    if (!g_desktop_wallpaper) {
        BWE_FillRect(fb, clip->x, clip->y, clip->width, clip->height, g_wallpaper_bg_color);
        return;
    }
    
    int32_t dest_w = (int32_t)fb->width;
    int32_t dest_h = (int32_t)fb->height;
    int32_t src_w = g_desktop_wallpaper->width;
    int32_t src_h = g_desktop_wallpaper->height;
    
    for (int32_t y = clip->y; y < clip->y + clip->height; y++) {
        if (y < 0 || y >= dest_h || y >= src_h) continue;
        
        uint32_t dest_row = y * (fb->pitch / 4);
        uint32_t src_row = y * src_w;
        
        for (int32_t x = clip->x; x < clip->x + clip->width; x++) {
            if (x < 0 || x >= dest_w || x >= src_w) continue;
            
            if (g_old_desktop_wallpaper && g_wallpaper_fade_alpha < 255) {
                uint32_t old_pixel = g_old_desktop_wallpaper->framebuffer[src_row + x];
                uint32_t new_pixel = g_desktop_wallpaper->framebuffer[src_row + x];
                
                uint8_t a = (uint8_t)(g_wallpaper_fade_alpha);
                uint8_t inv_a = 255 - a;
                
                uint8_t r = (((old_pixel >> 16) & 0xFF) * inv_a + ((new_pixel >> 16) & 0xFF) * a) / 255;
                uint8_t g = (((old_pixel >> 8) & 0xFF) * inv_a + ((new_pixel >> 8) & 0xFF) * a) / 255;
                uint8_t b = ((old_pixel & 0xFF) * inv_a + (new_pixel & 0xFF) * a) / 255;
                
                fb->buffer[dest_row + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            } else {
                fb->buffer[dest_row + x] = g_desktop_wallpaper->framebuffer[src_row + x];
            }
        }
    }
}


// Selection Rectangle State
static bool s_desktop_selecting = false;
static int32_t s_select_start_x = 0;
static int32_t s_select_start_y = 0;
static int32_t s_select_current_x = 0;
static int32_t s_select_current_y = 0;

// Taskbar and Desktop Globals
extern uint32_t g_task_panel_win_id;
extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

// Forward declarations
static void icon_render_callback(BWE_Window* self);
static void icon_event_callback(uint32_t id, const BWE_Event* event);

// Notification System API
bwe_error_t Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms) {
    int slot = -1;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!s_notifications[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        // Reuse slot 0
        slot = 0;
    }
    
    strcpy(s_notifications[slot].title, title);
    strcpy(s_notifications[slot].message, message);
    extern uint64_t timer_get_ticks(void);
    s_notifications[slot].expire_ticks = timer_get_ticks() + duration_ms;
    s_notifications[slot].active = true;
    
    g_hud_notifications = 0;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (s_notifications[i].active) g_hud_notifications++;
    }

    BWE_InvalidateWindow(BWE_DESKTOP_ID);
    return BWE_SUCCESS;
}

// Render priority borders and details for notifications
static void draw_notification_card(const BVFramebuffer* fb, const char* title, const char* msg, int32_t x, int32_t y) {
    int32_t w = 260;
    int32_t h = 60;
    
    BWE_FillRect(fb, x, y, w, h, 0xEE1E293B); // Slate-800 backdrop
    BWE_DrawRect(fb, x, y, w, h, 0xFF475569, 1); // Slate-600 border
    BWE_FillRect(fb, x, y, 4, h, 0xFF3B82F6); // Blue indicator
    
    BWE_DrawText(fb, title, x + 12, y + 10, 0xFFF1F5F9, 0);
    BWE_DrawText(fb, msg, x + 12, y + 32, 0xFF94A3B8, 0);
}

static void render_notifications(const BVFramebuffer* fb) {
    extern uint64_t timer_get_ticks(void);
    uint64_t now = timer_get_ticks();
    int32_t base_x = (int32_t)g_kernel_screen_width - 275;
    int32_t base_y = (int32_t)g_kernel_screen_height - 48 - 70; // 48px is taskbar height
    
    g_hud_notifications = 0;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (s_notifications[i].active) {
            if (now > s_notifications[i].expire_ticks) {
                s_notifications[i].active = false;
                BWE_InvalidateWindow(BWE_DESKTOP_ID);
                continue;
            }
            draw_notification_card(fb, s_notifications[i].title, s_notifications[i].message, base_x, base_y);
            base_y -= 70;
            g_hud_notifications++;
        }
    }
}

// Desktop Surface Event Handler (Selection rect & desktop clicks)
static void desktop_event_handler(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    extern BWE_Window g_windows[];
    
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        // Toggle off Start Menu if open
        if (g_start_menu_open) {
            g_start_menu_open = false;
            BOS_Hide(g_start_menu_win_id);
            BWE_InvalidateWindow(BWE_DESKTOP_ID);
        }
        
        // Start selection rectangle
        s_desktop_selecting = true;
        s_select_start_x = event->data.mouse.x;
        s_select_start_y = event->data.mouse.y;
        s_select_current_x = event->data.mouse.x;
        s_select_current_y = event->data.mouse.y;
        
        // Deselect all icons
        for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
            if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].type == BWE_TYPE_DESKTOP_ICON) {
                g_windows[i].control_data.button.is_pressed = false;
            }
        }
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (event->type == BWE_EVENT_MOUSE_MOVE) {
        if (s_desktop_selecting) {
            s_select_current_x = event->data.mouse.x;
            s_select_current_y = event->data.mouse.y;
            
            // Calculate bounding box
            int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x : s_select_current_x;
            int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y : s_select_current_y;
            int32_t x2 = s_select_start_x > s_select_current_x ? s_select_start_x : s_select_current_x;
            int32_t y2 = s_select_start_y > s_select_current_y ? s_select_start_y : s_select_current_y;
            
            // Highlight intersecting icons
            for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
                BWE_Window* icon = &g_windows[i];
                if (icon->state != BWE_STATE_DESTROYED && icon->type == BWE_TYPE_DESKTOP_ICON) {
                    BWE_Rect ib = icon->screen_bounds;
                    bool intersect = (ib.x < x2 && ib.x + ib.width > x1 &&
                                      ib.y < y2 && ib.y + ib.height > y1);
                    icon->control_data.button.is_pressed = intersect;
                }
            }
            BWE_InvalidateWindow(BWE_DESKTOP_ID);
        }
    } else if (event->type == BWE_EVENT_MOUSE_UP) {
        s_desktop_selecting = false;
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
    }
}

// Desktop custom paint callback to render wallpaper & selection rectangle
static void desktop_paint_handler(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    extern bool BWE_GetClip(BWE_Rect* out_rect);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    
    // Draw Wallpaper cropped to active clip region
    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip = self->screen_bounds;
    }
    Shell_DrawWallpaper(fb, &clip);
    
    // Draw Selection Box (Order: Desktop -> Icons -> Selection Rectangle)
    // Wait, the children (icons) will draw after this callback return,
    // so to draw the selection rectangle ON TOP of icons, we draw it at the very end in BWE_ComposeFrame, or we draw it here?
    // Actually, drawing it at the end of BWE_ComposeFrame is cleaner because it renders on top of the icons.
}

// Snapping/layout desktop icons helper
static void create_desktop_icon(const char* name, uint32_t app_id, int32_t grid_x, int32_t grid_y) {
    uint32_t icon_id;
    int32_t x = grid_x * 90 + 15;
    int32_t y = grid_y * 90 + 15;
    
    BOS_CreateSurface(BWE_DESKTOP_ID, x, y, 75, 75, BWE_WINDOW_CHILD | BWE_WINDOW_MOVABLE, &icon_id);
    BWE_Window* win = BWE_GetWindow(icon_id);
    if (win) {
        win->type = BWE_TYPE_DESKTOP_ICON;
        strcpy(win->control_data.button.text, name);
        win->on_render = icon_render_callback;
        win->on_event = icon_event_callback;
        win->user_data = (void*)(uintptr_t)app_id; // Store Application Registry ID
        g_hud_desktop_icons++;
    }
}

// Icon Render Engine
static void icon_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    
    BWE_Rect b = self->screen_bounds;
    bool is_selected = self->control_data.button.is_pressed;
    bool is_hovered = self->control_data.button.is_hovered;
    
    if (is_selected) {
        BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0x443B82F6); // 25% alpha blue
        BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0xFF3B82F6, 1);
    } else if (is_hovered) {
        BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0x22FFFFFF); // 13% alpha white
        BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0x88FFFFFF, 1);
    }
    
    // Draw procedural icon shape
    int32_t ix = b.x + 20;
    int32_t iy = b.y + 10;
    
    if (strcmp(self->control_data.button.text, "Computer") == 0) {
        BWE_FillRect(fb, ix, iy + 5, 35, 25, 0xFF3B82F6); // Folder main
        BWE_FillRect(fb, ix, iy, 15, 6, 0xFF2563EB);      // Folder tab
    } else if (strcmp(self->control_data.button.text, "Terminal") == 0) {
        BWE_FillRect(fb, ix, iy, 35, 30, 0xFF0F172A);
        BWE_DrawRect(fb, ix, iy, 35, 30, 0xFF64748B, 1);
        BWE_DrawText(fb, ">_", ix + 6, iy + 8, 0xFF10B981, 0);
    } else if (strcmp(self->control_data.button.text, "Settings") == 0) {
        BWE_FillRect(fb, ix + 10, iy + 5, 15, 20, 0xFF64748B);
        BWE_FillRect(fb, ix + 7, iy + 8, 21, 14, 0xFF64748B);
        BWE_FillRect(fb, ix + 12, iy + 10, 11, 10, 0xFF0F172A); // Hole
    } else if (strcmp(self->control_data.button.text, "Calculator") == 0) {
        BWE_FillRect(fb, ix + 4, iy, 28, 30, 0xFF475569);
        BWE_FillRect(fb, ix + 8, iy + 4, 20, 6, 0xFF94A3B8); // Screen
        BWE_FillRect(fb, ix + 8, iy + 14, 4, 4, 0xFFF1F5F9);
        BWE_FillRect(fb, ix + 16, iy + 14, 4, 4, 0xFFF1F5F9);
        BWE_FillRect(fb, ix + 24, iy + 14, 4, 4, 0xFFF1F5F9);
        BWE_FillRect(fb, ix + 8, iy + 22, 4, 4, 0xFFF1F5F9);
        BWE_FillRect(fb, ix + 16, iy + 22, 4, 4, 0xFFF1F5F9);
        BWE_FillRect(fb, ix + 24, iy + 22, 4, 4, 0xFFF1F5F9);
    } else if (strcmp(self->control_data.button.text, "Music") == 0) {
        BWE_FillRect(fb, ix + 10, iy + 5, 16, 20, 0xFF2563EB);  // Note body
        BWE_FillRect(fb, ix + 24, iy + 5, 4, 20, 0xFF2563EB);   // Note stem
        BWE_FillRect(fb, ix + 6, iy + 22, 10, 6, 0xFF3B82F6);   // Note head
    } else {
        BWE_FillRect(fb, ix + 8, iy + 8, 20, 20, 0xFFEAB308);
    }
    
    int32_t len = strlen(self->control_data.button.text);
    int32_t tx = b.x + (b.width - (len * 8)) / 2;
    BWE_DrawText(fb, self->control_data.button.text, tx, b.y + 48, 0xFFFFFFFF, 0);
}

// Snapping implementation on dragging end
static void icon_event_callback(uint32_t id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(id);
    if (!self) return;
    
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        // Highlight selection
        self->control_data.button.is_pressed = true;
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
    } else if (event->type == BWE_EVENT_MOUSE_UP) {
        int32_t grid_size = 90;
        int32_t x = self->local_bounds.x;
        int32_t y = self->local_bounds.y;
        
        int32_t snapped_x = ((x + grid_size / 2) / grid_size) * grid_size + 15;
        int32_t snapped_y = ((y + grid_size / 2) / grid_size) * grid_size + 15;
        
        if (snapped_x + self->local_bounds.width > (int32_t)g_kernel_screen_width) {
            snapped_x = (int32_t)g_kernel_screen_width - self->local_bounds.width - 15;
        }
        if (snapped_x < 15) snapped_x = 15;
        
        if (snapped_y + self->local_bounds.height > (int32_t)g_kernel_screen_height - 60) {
            snapped_y = (int32_t)g_kernel_screen_height - 60 - self->local_bounds.height - 15;
        }
        if (snapped_y < 15) snapped_y = 15;
        
        BOS_SetBounds(id, snapped_x, snapped_y, self->local_bounds.width, self->local_bounds.height);
        
        // Handle double click logic
        static uint64_t s_last_click_ticks = 0;
        static uint32_t s_last_click_id = 0;
        extern uint64_t timer_get_ticks(void);
        uint64_t now = timer_get_ticks();
        if (id == s_last_click_id && (now - s_last_click_ticks) < 400) {
            uint32_t app_id = (uint32_t)(uintptr_t)self->user_data;
            horse_launch(app_id);
            s_last_click_ticks = 0;
        } else {
            s_last_click_ticks = now;
            s_last_click_id = id;
        }
        BWE_InvalidateWindow(BWE_DESKTOP_ID);
    }
}

extern uint64_t timer_get_ticks(void);
#include "kernel/core/memory/heap/include/heap.h"

#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/shell/rook/include/rook.h"
extern rook_page_t* rook_page_welcome_get(void);

static bool s_boot_experience_active = false;
static bool s_boot_audio_started = false;
static uint32_t s_boot_frame_count = 0;
static uint32_t* s_welcome_buffer = 0;

// Boot experience duration constants (in frames at ~60 FPS)
#define BOOT_WELCOME_FRAMES   180   // 3 seconds fully opaque
#define BOOT_FADE_FRAMES      120   // 2 seconds fade
#define BOOT_TOTAL_FRAMES     (BOOT_WELCOME_FRAMES + BOOT_FADE_FRAMES)

bool Desktop_Shell_IsBootExperienceActive(void) {
    return s_boot_experience_active;
}

void Desktop_Shell_StartBootExperience(void) {
    s_boot_experience_active = true;
    s_boot_audio_started = false;
    s_boot_frame_count = 0;
    
    uint32_t total_pixels = g_kernel_screen_width * g_kernel_screen_height;
    s_welcome_buffer = (uint32_t*)kmalloc(total_pixels * sizeof(uint32_t));
    
    // Reset ROOK welcome page animation state
    rook_page_t* w = rook_page_welcome_get();
    if (w && w->ops.on_enter) {
        w->ops.on_enter(w);
    }
}

// Master Hook in BWE_ComposeFrame for extra overlay renderings
void Shell_PostComposeHook(const BVFramebuffer* fb) {
    // --- BOOT EXPERIENCE OVERLAY ---
    if (s_boot_experience_active && s_welcome_buffer) {
        s_boot_frame_count++;
        
        // Start audio on first frame AFTER sti (deferred from init)
        if (!s_boot_audio_started) {
            s_boot_audio_started = true;
            extern void audio_player_open(const char* path);
            extern void audio_player_play(void);
            audio_player_open("/BOOT1.WAV");
            audio_player_play();
        }
        
        rook_page_t* w = rook_page_welcome_get();
        if (w) {
            // Update welcome screen animation
            if (w->ops.on_update) {
                w->ops.on_update(w, 16);
            }
            
            if (s_boot_frame_count <= BOOT_WELCOME_FRAMES) {
                // Phase 1: Fully opaque Welcome Screen (0s - 3s)
                if (w->ops.on_render) {
                    w->ops.on_render(w, (uint32_t*)fb->buffer, fb->pitch);
                }
            } else if (s_boot_frame_count <= BOOT_TOTAL_FRAMES) {
                // Phase 2: Smooth fade out (3s - 5s)
                if (w->ops.on_render) {
                    w->ops.on_render(w, s_welcome_buffer, g_kernel_screen_width * sizeof(uint32_t));
                }
                
                uint32_t fade_progress = s_boot_frame_count - BOOT_WELCOME_FRAMES;
                // Alpha of welcome screen: 255 -> 0 over BOOT_FADE_FRAMES
                uint32_t alpha = 255 - (fade_progress * 255 / BOOT_FADE_FRAMES);
                uint32_t inv_alpha = 255 - alpha;
                
                uint32_t total_pixels = g_kernel_screen_width * g_kernel_screen_height;
                uint32_t* dst = (uint32_t*)fb->buffer;
                uint32_t* src = s_welcome_buffer;
                
                for (uint32_t i = 0; i < total_pixels; i++) {
                    uint32_t desk = dst[i];
                    uint32_t welc = src[i];
                    
                    uint32_t rb_desk = desk & 0x00FF00FF;
                    uint32_t g_desk  = desk & 0x0000FF00;
                    
                    uint32_t rb_welc = welc & 0x00FF00FF;
                    uint32_t g_welc  = welc & 0x0000FF00;
                    
                    uint32_t rb = ((rb_desk * inv_alpha) + (rb_welc * alpha)) >> 8;
                    uint32_t g  = ((g_desk * inv_alpha) + (g_welc * alpha)) >> 8;
                    
                    dst[i] = (rb & 0x00FF00FF) | (g & 0x0000FF00) | 0xFF000000;
                }
            } else {
                // Phase 3: Boot experience complete
                s_boot_experience_active = false;
                kfree(s_welcome_buffer);
                s_welcome_buffer = 0;
            }
        }
    }

    // 1. Draw Selection Box
    if (s_desktop_selecting) {
        int32_t x1 = s_select_start_x < s_select_current_x ? s_select_start_x : s_select_current_x;
        int32_t y1 = s_select_start_y < s_select_current_y ? s_select_start_y : s_select_current_y;
        int32_t w = s_select_start_x > s_select_current_x ? s_select_start_x - x1 : x1 - s_select_start_x;
        int32_t h = s_select_start_y > s_select_current_y ? s_select_start_y - y1 : y1 - s_select_start_y;
        if (w < 0) w = -w;
        if (h < 0) h = -h;
        
        BWE_FillRect(fb, x1, y1, w, h, 0x333B82F6); // 20% alpha blue
        BWE_DrawRect(fb, x1, y1, w, h, 0xFF3B82F6, 1);
    }
    
    // 2. Draw Active Notifications
    render_notifications(fb);
    
    // 3. Update HUD Telemetry
    g_hud_focused_window = BOS_GetFocus();
    
    extern BWE_Window g_windows[];
    uint32_t open_wins = 0;
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].parent_id == BWE_DESKTOP_ID && g_windows[i].id != BWE_DESKTOP_ID && g_windows[i].id != g_task_panel_win_id && g_windows[i].type == BWE_TYPE_WINDOW) {
            open_wins++;
        }
    }
    g_hud_open_windows = open_wins;
    
    // Update active cursor location
    extern int32_t g_bwe_mouse_x;
    extern int32_t g_bwe_mouse_y;
    g_hud_hovered_control = 0;
    for (int32_t i = (int32_t)BWE_MAX_WINDOWS - 1; i >= 0; i--) {
        if (g_windows[i].state != BWE_STATE_DESTROYED && g_windows[i].id != BWE_DESKTOP_ID) {
            BWE_HitZone hit = BWE_HitTest(g_windows[i].id, g_bwe_mouse_x, g_bwe_mouse_y);
            if (hit != BWE_HIT_NONE) {
                g_hud_hovered_control = g_windows[i].id;
                break;
            }
        }
    }
}

extern void display_print(const char* s);

static bwe_error_t demo_app_launch_wrapper(uint32_t* out_win) {
    extern void BWE_DemoApp_Initialize(void);
    BWE_DemoApp_Initialize();
    if (out_win) *out_win = 1; // Standard demo window ID is 1
    return BWE_SUCCESS;
}

// Desktop Shell Main Initializer
bwe_error_t Desktop_Shell_Initialize(void) {
    display_print("[SHELL] Starting ATOMS OS Native Workspace Shell...\n");
    
    memset(s_notifications, 0, sizeof(s_notifications));
    
    // Hook Desktop Window render and event callbacks
    BWE_Window* desktop = BWE_GetWindow(BWE_DESKTOP_ID);
    if (desktop) {
        desktop->on_render = desktop_paint_handler;
        desktop->on_event = desktop_event_handler;
    }
    
    horse_init();
    wallpaper_manager_init();
    
    g_hud_desktop_icons = 0;
    
    // Create Desktop Icons
    create_desktop_icon("Computer", APP_ID_EXPLORER, 0, 0);
    create_desktop_icon("Terminal", APP_ID_TERMINAL, 0, 1);
    create_desktop_icon("Settings", APP_ID_SETTINGS, 0, 2);
    create_desktop_icon("Calculator", APP_ID_CALCULATOR, 0, 3);
    create_desktop_icon("Sandbox", APP_ID_SANDBOX, 0, 4);
    create_desktop_icon("Stress Test", APP_ID_STRESS_TEST, 0, 5);
    create_desktop_icon("Music", APP_ID_MUSIC, 0, 6);
    
    // Initialize UI
    TaskPanel_Initialize();
    StartMenu_Initialize();
    
    Shell_ShowNotification("Welcome", "ATOMS OS Workspace V2.0 Ready!", 5000);
    return BWE_SUCCESS;
}
