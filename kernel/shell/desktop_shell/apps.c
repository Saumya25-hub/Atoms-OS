#include "desktop_shell.h"
#include "kernel/net/netif.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/audio/session/audio_player.h"
#include "kernel/wm/bwe/include/bwe_layout.h"
#include "userspace/shell/command.h"

// HUD variables
extern bool g_hud_visible;

// Helper to find the top-level parent window's user_data context
static void* get_top_parent_ctx(uint32_t win_id) {
    BWE_Window* curr = BWE_GetWindow(win_id);
    while (curr && curr->parent_id != BWE_DESKTOP_ID && curr->parent_id != curr->id && curr->parent_id != 0) {
        curr = BWE_GetWindow(curr->parent_id);
    }
    if (curr && curr->parent_id == BWE_DESKTOP_ID) {
        return curr->user_data;
    }
    return NULL;
}

// Helper to format integers to buffer
static void strcat_itoa(uint32_t val, char* buf) {
    char temp[16];
    int i = 0;
    if (val == 0) { strcat(buf, "0"); return; }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    char rev[16];
    int r = 0;
    while (i > 0) rev[r++] = temp[--i];
    rev[r] = '\0';
    strcat(buf, rev);
}

// Helper to parse integers
static int shell_atoi(const char* s) {
    int res = 0;
    int sign = 1;
    int i = 0;
    if (s[0] == '-') {
        sign = -1;
        i++;
    }
    for (; s[i] != '\0'; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            res = res * 10 + (s[i] - '0');
        } else {
            break;
        }
    }
    return sign * res;
}

// Removed old explorer

// ============================================================
// Interactive Terminal Implementation
// ============================================================
#define MAX_TERM_LINES 15
typedef struct {
    uint32_t win_id;
    uint32_t canvas_id;
    uint32_t textbox_id;
    void (*original_textbox_on_event)(uint32_t, const BWE_Event*);
    char lines[MAX_TERM_LINES][64];
    uint32_t line_count;
} TerminalCtx;

static void terminal_add_line(TerminalCtx* ctx, const char* line) {
    if (ctx->line_count < MAX_TERM_LINES) {
        strcpy(ctx->lines[ctx->line_count++], line);
    } else {
        for (int i = 0; i < MAX_TERM_LINES - 1; i++) {
            strcpy(ctx->lines[i], ctx->lines[i + 1]);
        }
        strcpy(ctx->lines[MAX_TERM_LINES - 1], line);
    }
}

static void terminal_output_sink(void* context, const char* str) {
    TerminalCtx* ctx = (TerminalCtx*)context;
    char buffer[128];
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            buffer[j] = '\0';
            terminal_add_line(ctx, buffer);
            j = 0;
        } else if (str[i] != '\r') {
            if (j < 127) buffer[j++] = str[i];
        }
    }
    if (j > 0) {
        buffer[j] = '\0';
        terminal_add_line(ctx, buffer);
    }
}

static void terminal_paint_callback(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;
    
    TerminalCtx* ctx = (TerminalCtx*)get_top_parent_ctx(canvas_id);
    if (!ctx) return;
    
    BWE_Rect b = self->screen_bounds;
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF000000); // Black CLI
    
    for (uint32_t i = 0; i < ctx->line_count; i++) {
        BWE_DrawText(fb, ctx->lines[i], b.x + 10, b.y + 10 + i * 18, 0xFF10B981, 0); // Green monospace-styled text
    }
}

static void terminal_textbox_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;
    
    TerminalCtx* ctx = (TerminalCtx*)get_top_parent_ctx(window_id);
    if (!ctx) return;
    
    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        uint32_t ch = (uint8_t)event->data.key.character;
        if (kc == 0x1C || kc == 0x0A || kc == 0x0D || kc == 13 || kc == 10 || ch == '\r' || ch == '\n') {
            char cmd[128];
            strcpy(cmd, self->control_data.textbox.text);
            
            // Clean command input and reset cursor position
            self->control_data.textbox.text[0] = '\0';
            self->control_data.textbox.cursor_pos = 0;
            BWE_InvalidateWindow(window_id);
            
            char echo[140];
            strcpy(echo, "root@atoms:~# ");
            strcat(echo, cmd);
            terminal_add_line(ctx, echo);
            
            if (cmd[0] != '\0') {
                if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
                    ctx->line_count = 0;
                } else if (strcmp(cmd, "exit") == 0) {
                    BOS_DestroySurface(ctx->win_id);
                    extern void TaskPanel_Update(void);
                    TaskPanel_Update();
                    return;
                } else {
                    static int shell_inited = 0;
                    if (!shell_inited) {
                        command_init();
                        shell_inited = 1;
                    }
                    Shell_ExecuteCommand(cmd, terminal_output_sink, ctx);
                }
            }
            if (ctx->canvas_id != 0) {
                BWE_InvalidateWindow(ctx->canvas_id);
            }
            return; // Consume Enter key event so it isn't appended to input box!
        }
    }

    // Call default textbox event handler for all non-Enter keys
    if (ctx->original_textbox_on_event) {
        ctx->original_textbox_on_event(window_id, event);
    }
}

static void terminal_window_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || !win->user_data) return;
    TerminalCtx* ctx = (TerminalCtx*)win->user_data;

    if (event->type == BWE_EVENT_KEY_DOWN || event->type == BWE_EVENT_KEY_UP) {
        if (ctx->textbox_id != 0) {
            BWE_Window* tb = BWE_GetWindow(ctx->textbox_id);
            if (tb && tb->on_event) {
                tb->on_event(ctx->textbox_id, event);
            }
        }
    } else if (event->type == BWE_EVENT_MOUSE_DOWN) {
        if (ctx->textbox_id != 0) {
            BOS_SetFocus(ctx->textbox_id);
        }
    }
}

/* 
 * ============================================================
 * LEGACY / DEPRECATED APPLICATION INITIALIZERS
 * Retained for backward compatibility stubs only.
 * Primary Desktop UI launches BOSX production runtimes.
 * ============================================================
 */

bwe_error_t terminal_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(150, 120, 500, 360, "Interactive Terminal", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    TerminalCtx* ctx = (TerminalCtx*)kcalloc(1, sizeof(TerminalCtx));
    ctx->win_id = win_id;
    ctx->line_count = 0;
    win->user_data = ctx;
    win->on_event = terminal_window_event_callback;
    
    // Command input textbox (Docked to bottom)
    BOS_CreateTextbox(win_id, 0, 290, 490, 30, "Type help for list of commands...", &ctx->textbox_id);
    BWE_SetAnchorMode(ctx->textbox_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_BOTTOM | BWE_ANCHOR_RIGHT);
    
    // Text output canvas (Docked to fill remaining space)
    BOS_CreateCanvas(win_id, 0, 0, 490, 290, terminal_paint_callback, &ctx->canvas_id);
    BWE_SetAnchorMode(ctx->canvas_id, BWE_ANCHOR_ALL);
    
    // Override event callback of the textbox to intercept Enter
    if (ctx->textbox_id != 0) {
        BWE_Window* tb = BWE_GetWindow(ctx->textbox_id);
        if (tb) {
            ctx->original_textbox_on_event = tb->on_event;
            tb->on_event = terminal_textbox_event_callback;
        }
        BOS_SetFocus(ctx->textbox_id);
    }
    
    // Initial welcome lines
    terminal_add_line(ctx, "ATOMS OS Terminal session active");
    terminal_add_line(ctx, "Type 'help' for shell command reference.");
    terminal_add_line(ctx, "");
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Settings Panel Implementation
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t right_panel_id;
} SettingsCtx;

static void load_settings_tab(SettingsCtx* ctx, const char* category);

#include "kernel/media/bopawn/wallpaper/wallpaper_settings.h"
#include "kernel/drivers/input/cursor/cursor_state.h"
#include "kernel/drivers/input/cursor/cursor_theme.h"

// Globals for Mouse test tab
static bool g_is_testing_mouse = false;
static uint32_t g_mouse_scroll_content_id = 0;

static void btn_test_cursor_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (btn) {
        CursorShape shape = (CursorShape)(uintptr_t)btn->user_data;
        cursor_state_set_shape(shape);
        g_is_testing_mouse = true;
    }
}

static void btn_stop_test_clicked(uint32_t btn_id) {
    (void)btn_id;
    cursor_state_set_shape(CURSOR_SHAPE_ARROW);
    g_is_testing_mouse = false;
}

static void mouse_scroll_cb(uint32_t scr_id, int32_t val) {
    (void)scr_id;
    if (g_mouse_scroll_content_id != 0) {
        BWE_Window* content = BWE_GetWindow(g_mouse_scroll_content_id);
        if (content) {
            BOS_SetBounds(g_mouse_scroll_content_id, 
                          content->local_bounds.x, 
                          -val, 
                          content->local_bounds.width, 
                          content->local_bounds.height);
        }
    }
}

static void mouse_canvas_paint_cb(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* win = BWE_GetWindow(canvas_id);
    if (!win) return;
    
    CursorShape shape = (CursorShape)(uintptr_t)win->user_data;
    uint32_t w = 0, h = 0, hx = 0, hy = 0;
    const uint32_t* bmp = cursor_theme_get_bitmap(shape, 0, &w, &h, &hx, &hy);
    
    if (bmp) {
        int32_t cx = (win->screen_bounds.width - w) / 2;
        int32_t cy = (win->screen_bounds.height - h) / 2;
        BWE_DrawBitmap(fb, bmp, win->screen_bounds.x + cx, win->screen_bounds.y + cy, w, h, 0, 0, w, h, w);
    }
}

static void btn_category_display_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK display btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK display ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "Display");
}
static void btn_category_wallpaper_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK wallpaper btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK wallpaper ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "Wallpaper");
}
static void btn_category_theme_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK theme btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK theme ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "Theme");
}
static void btn_category_system_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK system btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK system ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "System Info");
}
static void btn_category_mouse_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK mouse btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK mouse ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "Mouse Behavior");
}
static void btn_category_network_clicked(uint32_t btn_id) {
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK network btn_id", btn_id);
    bwe_log_id("INFO", "SETTINGS_TRACE BUTTON_CALLBACK network ctx_ptr", (uint32_t)(uintptr_t)ctx);
    if (ctx) load_settings_tab(ctx, "Network ATOME");
}

static void chk_hud_toggled(uint32_t chk_id, bool is_checked) {
    (void)chk_id;
    g_hud_visible = is_checked;
    BWE_InvalidateWindow(BWE_DESKTOP_ID);
}

#include "kernel/wm/botheme/botheme.h"

static void btn_theme_dark_clicked(uint32_t btn_id) {
    (void)btn_id;
    BOTHEME_SetTheme(BOTHEME_DARK);
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Theme");
    Shell_ShowNotification("Personalization", "Theme updated: Dark Slate", 3000);
}

static void btn_theme_light_clicked(uint32_t btn_id) {
    (void)btn_id;
    BOTHEME_SetTheme(BOTHEME_LIGHT);
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Theme");
    Shell_ShowNotification("Personalization", "Theme updated: Light Studio", 3000);
}

static void btn_theme_midnight_clicked(uint32_t btn_id) {
    (void)btn_id;
    BOTHEME_SetTheme(BOTHEME_MIDNIGHT);
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Theme");
    Shell_ShowNotification("Personalization", "Theme updated: Midnight Navy", 3000);
}

static void btn_theme_classic_clicked(uint32_t btn_id) {
    (void)btn_id;
    BOTHEME_SetTheme(BOTHEME_CLASSIC);
    SettingsCtx* ctx = (SettingsCtx*)get_top_parent_ctx(btn_id);
    if (ctx) load_settings_tab(ctx, "Theme");
    Shell_ShowNotification("Personalization", "Theme updated: Classic Workstation", 3000);
}

static void btn_theme_toggle_clicked(uint32_t btn_id) {
    (void)btn_id;
    static bool is_dark = true;
    is_dark = !is_dark;
    
    BOTHEME_SetTheme(is_dark ? BOTHEME_DARK : BOTHEME_LIGHT);
    Shell_ShowNotification("Theme Engine", is_dark ? "Switched to Dark Mode" : "Switched to Light Mode", 3000);
}

static void load_settings_tab(SettingsCtx* ctx, const char* category) {
    if (!ctx) return;
    uint32_t old_right_panel = ctx->right_panel_id;
    if (ctx->right_panel_id != 0) {
        bwe_log_id("INFO", "SETTINGS_TRACE DESTROY requested ID", ctx->right_panel_id);
        BOS_DestroySurface(ctx->right_panel_id);
    }
    ctx->right_panel_id = 0;
    g_mouse_scroll_content_id = 0;
    
    BOS_CreatePanel(ctx->win_id, 140, 0, 380, 320, BOTHEME_GetColor(BOTHEME_SURFACE_PRIMARY), &ctx->right_panel_id);
    BWE_SetAnchorMode(ctx->right_panel_id, BWE_ANCHOR_ALL);

    bwe_log_id("INFO", "SETTINGS_TRACE SETTINGS_CONTEXT ptr", (uint32_t)(uintptr_t)ctx);
    bwe_log_id("INFO", "SETTINGS_TRACE ROOT_ID", ctx->win_id);
    bwe_log_id("INFO", "SETTINGS_TRACE PAGE_SWITCH old_content_parent", old_right_panel);
    bwe_log_id("INFO", "SETTINGS_TRACE PAGE_SWITCH active_page_id", ctx->right_panel_id);
    
    if (ctx->right_panel_id != 0) {
        BWE_Window* panel = BWE_GetWindow(ctx->right_panel_id);
        if (panel) {
            panel->padding.left = 15;
            panel->padding.top = 15;
        }
        
        uint32_t dummy = 0;
        if (strcmp(category, "Display") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "Display Driver Configuration", BOTHEME_GetColor(BOTHEME_TEXT_PRIMARY), &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Active Resolution: 1280x720", BOTHEME_GetColor(BOTHEME_TEXT_SECONDARY), &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 70, "Color Format: 32-bit ARGB", BOTHEME_GetColor(BOTHEME_TEXT_SECONDARY), &dummy);
            
            uint32_t chk_id = 0;
            BOS_CreateCheckbox(ctx->right_panel_id, 15, 110, 200, 30, "Show Performance HUD", chk_hud_toggled, &chk_id);
            if (chk_id != 0) {
                BWE_Window* chk = BWE_GetWindow(chk_id);
                if (chk) chk->control_data.checkbox.checked = g_hud_visible;
            }
            
        } else if (strcmp(category, "Theme") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 12, "Personalization & System Themes", BOTHEME_GetColor(BOTHEME_TEXT_PRIMARY), &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 34, "Select a system theme preset to apply live:", BOTHEME_GetColor(BOTHEME_TEXT_SECONDARY), &dummy);
            
            BOThemeID active_id = BOTHEME_GetTheme();
            BOS_CreateButton(ctx->right_panel_id, 15, 60, 165, 95, (active_id == BOTHEME_DARK) ? "[ACTIVE] Dark Slate" : "Dark Slate", btn_theme_dark_clicked, &dummy);
            BOS_CreateButton(ctx->right_panel_id, 195, 60, 165, 95, (active_id == BOTHEME_LIGHT) ? "[ACTIVE] Light Studio" : "Light Studio", btn_theme_light_clicked, &dummy);
            BOS_CreateButton(ctx->right_panel_id, 15, 170, 165, 95, (active_id == BOTHEME_MIDNIGHT) ? "[ACTIVE] Midnight" : "Midnight Navy", btn_theme_midnight_clicked, &dummy);
            BOS_CreateButton(ctx->right_panel_id, 195, 170, 165, 95, (active_id == BOTHEME_CLASSIC) ? "[ACTIVE] Classic" : "Classic Workstation", btn_theme_classic_clicked, &dummy);
            
        } else if (strcmp(category, "System Info") == 0) {
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "ATOMS OS System Specifications", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Processor: x86_64 Core Preemptive", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 70, "Memory RAM: 512 Megabytes", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 95, "Version: BWE V2.1 Shell Phase 4", 0xFF475569, &dummy);
            
            BOS_CreateLabel(ctx->right_panel_id, 15, 140, "Diagnostic telemetry statistics are", 0xFF94A3B8, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 160, "live on the performance HUD overlays.", 0xFF94A3B8, &dummy);
        } else if (strcmp(category, "Wallpaper") == 0) {
            wallpaper_settings_render(ctx->right_panel_id);
        } else if (strcmp(category, "Network ATOME") == 0) {
            NetInterface* netif = netif_get_default();
            char ip_str[48] = "IPv4 address: 10.0.2.15";
            char dns_str[48] = "DNS server: 10.0.2.3";
            char gw_str[48] = "Default gateway: 10.0.2.2";
            char mask_str[48] = "Subnet mask: 255.255.255.0";
            char mac_str[48] = "Physical address (MAC): 52:54:00:12:34:56";

            if (netif) {
                uint32_t ip = netif->ip_addr;
                strcpy(ip_str, "IPv4 address: ");
                strcat_itoa((ip >> 24) & 0xFF, ip_str); strcat(ip_str, ".");
                strcat_itoa((ip >> 16) & 0xFF, ip_str); strcat(ip_str, ".");
                strcat_itoa((ip >> 8) & 0xFF, ip_str);  strcat(ip_str, ".");
                strcat_itoa(ip & 0xFF, ip_str);

                uint32_t dns = netif->dns_server;
                strcpy(dns_str, "DNS server: ");
                strcat_itoa((dns >> 24) & 0xFF, dns_str); strcat(dns_str, ".");
                strcat_itoa((dns >> 16) & 0xFF, dns_str); strcat(dns_str, ".");
                strcat_itoa((dns >> 8) & 0xFF, dns_str);  strcat(dns_str, ".");
                strcat_itoa(dns & 0xFF, dns_str);

                uint32_t gw = netif->gateway_ip;
                strcpy(gw_str, "Default gateway: ");
                strcat_itoa((gw >> 24) & 0xFF, gw_str); strcat(gw_str, ".");
                strcat_itoa((gw >> 16) & 0xFF, gw_str); strcat(gw_str, ".");
                strcat_itoa((gw >> 8) & 0xFF, gw_str);  strcat(gw_str, ".");
                strcat_itoa(gw & 0xFF, gw_str);

                uint32_t mask = netif->netmask;
                strcpy(mask_str, "Subnet mask: ");
                strcat_itoa((mask >> 24) & 0xFF, mask_str); strcat(mask_str, ".");
                strcat_itoa((mask >> 16) & 0xFF, mask_str); strcat(mask_str, ".");
                strcat_itoa((mask >> 8) & 0xFF, mask_str);  strcat(mask_str, ".");
                strcat_itoa(mask & 0xFF, mask_str);

                const char* hex = "0123456789ABCDEF";
                strcpy(mac_str, "Physical address (MAC): ");
                for (int m = 0; m < 6; m++) {
                    char h[4];
                    h[0] = hex[(netif->mac_addr[m] >> 4) & 0xF];
                    h[1] = hex[netif->mac_addr[m] & 0xF];
                    h[2] = (m < 5) ? ':' : '\0';
                    h[3] = '\0';
                    strcat(mac_str, h);
                }
            }

            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "Network ATOME Details", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 40, "IP assignment: DHCP", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 60, ip_str, 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 80, "DNS server assignment: DHCP", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 100, dns_str, 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 120, gw_str, 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 140, mask_str, 0xFF475569, &dummy);
            
            BOS_CreateLabel(ctx->right_panel_id, 15, 170, "Manufacturer: Intel", 0xFF1E293B, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 190, "Description: Intel E1000 Network Adapter", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 210, "Driver version: ATOMS E1000 Native Driver", 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 230, mac_str, 0xFF475569, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 260, "Connection status: Connected", 0xFF16A34A, &dummy);
        } else if (strcmp(category, "Mouse Behavior") == 0) {
            uint32_t dummy = 0;
            BOS_CreateLabel(ctx->right_panel_id, 15, 15, "Cursor Testing & Preview", 0xFF0F172A, &dummy);
            BOS_CreateLabel(ctx->right_panel_id, 15, 45, "Test actual OS cursor modes in real-time.", 0xFF475569, &dummy);
            
            // Persistent STOP TEST button
            BOS_CreateButton(ctx->right_panel_id, 250, 15, 100, 35, "STOP TEST", btn_stop_test_clicked, &dummy);
            
            // Scroll container setup
            uint32_t scroll_container_id = 0;
            BOS_CreatePanel(ctx->right_panel_id, 0, 80, 380, 240, 0xFFF1F5F9, &scroll_container_id);
            
            if (scroll_container_id != 0) {
                // Determine content height
                int32_t card_height = 80;
                int32_t card_spacing = 10;
                int32_t num_cursors = CURSOR_SHAPE_MAX;
                int32_t content_height = num_cursors * (card_height + card_spacing) + 10;
                
                // Content panel (this moves up and down)
                BOS_CreatePanel(scroll_container_id, 10, 0, 340, content_height, 0xFFF1F5F9, &g_mouse_scroll_content_id);
                
                // Scrollbar to the right
                int32_t scroll_max = content_height - 240;
                if (scroll_max < 0) scroll_max = 0;
                
                uint32_t scrollbar_id = 0;
                BOS_CreateScrollBar(scroll_container_id, 350, 0, 20, 240, true, 0, scroll_max, mouse_scroll_cb, &scrollbar_id);
                
                if (g_mouse_scroll_content_id != 0) {
                    const char* cursor_names[CURSOR_SHAPE_MAX] = {
                        "1 Default Arrow",
                        "2 Text Select",
                        "3 Horizontal Resize",
                        "4 Vertical Resize",
                        "5 Busy",
                        "6 Wait",
                        "7 Crosshair",
                        "8 Link/Hand"
                    };
                    
                    int32_t cy = 10;
                    for (int i = 0; i < num_cursors; i++) {
                        uint32_t card_id = 0;
                        BOS_CreatePanel(g_mouse_scroll_content_id, 0, cy, 330, card_height, 0xFFFFFFFF, &card_id);
                        
                        if (card_id != 0) {
                            // Canvas for sprite preview
                            uint32_t canvas_id = 0;
                            BOS_CreateCanvas(card_id, 10, 16, 48, 48, mouse_canvas_paint_cb, &canvas_id);
                            BWE_Window* cvs = BWE_GetWindow(canvas_id);
                            if (cvs) cvs->user_data = (void*)(uintptr_t)i;
                            
                            // Labels
                            BOS_CreateLabel(card_id, 70, 20, cursor_names[i], 0xFF1E293B, &dummy);
                            BOS_CreateLabel(card_id, 70, 45, "Runtime Native Sprite", 0xFF64748B, &dummy);
                            
                            // Test Button
                            uint32_t btn_id = 0;
                            BOS_CreateButton(card_id, 230, 20, 90, 40, "Test", btn_test_cursor_clicked, &btn_id);
                            BWE_Window* btn = BWE_GetWindow(btn_id);
                            if (btn) btn->user_data = (void*)(uintptr_t)i;
                        }
                        
                        cy += (card_height + card_spacing);
                    }
                }
            }
        }
    }
    
    BWE_InvalidateWindow(ctx->win_id);
}

bwe_error_t settings_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(200, 150, 520, 360, "Settings Control", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    SettingsCtx* ctx = (SettingsCtx*)kcalloc(1, sizeof(SettingsCtx));
    ctx->win_id = win_id;
    win->user_data = ctx;

    bwe_log_id("INFO", "SETTINGS_TRACE SETTINGS_CONTEXT alloc ptr", (uint32_t)(uintptr_t)ctx);
    bwe_log_id("INFO", "SETTINGS_TRACE ROOT_ID", win_id);
    
    // Left sidebar categories panel
    uint32_t sidebar_id = 0;
    BOS_CreatePanel(win_id, 0, 0, 140, 320, 0xFFE2E8F0, &sidebar_id);
    BWE_SetAnchorMode(sidebar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM);
    
    if (sidebar_id != 0) {
        uint32_t dummy = 0;
        BOS_CreateButton(sidebar_id, 10, 10, 120, 35, "Display Settings", btn_category_display_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 55, 120, 35, "Wallpaper", btn_category_wallpaper_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 100, 120, 35, "Theme Style", btn_category_theme_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 145, 120, 35, "System Info", btn_category_system_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 190, 120, 35, "Mouse Behavior", btn_category_mouse_clicked, &dummy);
        BOS_CreateButton(sidebar_id, 10, 235, 120, 35, "Network ATOME", btn_category_network_clicked, &dummy);
    }
    
    // Right panel content space
    BOS_CreatePanel(win_id, 140, 0, 380, 320, 0xFFF1F5F9, &ctx->right_panel_id);
    BWE_SetAnchorMode(ctx->right_panel_id, BWE_ANCHOR_ALL);
    
    load_settings_tab(ctx, "Display");
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Calculator Implementation
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t display_id;
    char display_text[32];
    int32_t value1;
    char op;
    bool new_input;
} CalculatorCtx;

static void update_calc_display(CalculatorCtx* ctx) {
    if (ctx->display_id != 0) {
        BWE_Window* lbl = BWE_GetWindow(ctx->display_id);
        if (lbl) {
#ifndef BWE_ENABLE_RENDER_TRACE
#define BWE_ENABLE_RENDER_TRACE 0
#endif
#if BWE_ENABLE_RENDER_TRACE
            extern void serial_write_direct(const char* str);
            serial_write_direct("[RENDER_TRACE 1] update_calc_display BEFORE text=");
            serial_write_direct(lbl->control_data.label.text);
            serial_write_direct(" AFTER text=");
            serial_write_direct(ctx->display_text);
            serial_write_direct("\n");
#endif

            strcpy(lbl->control_data.label.text, ctx->display_text);
            BWE_InvalidateWindow(ctx->display_id);
        }
    }
}

static void calc_btn_clicked(uint32_t btn_id) {
#if BWE_ENABLE_RENDER_TRACE
    extern void serial_write_direct(const char* str);
    extern void serial_write_dec_direct(int val);
    serial_write_direct("[CALC] calc_btn_clicked ENTERED btn_id=");
    serial_write_dec_direct((int)btn_id);
    serial_write_direct("\n");
#endif

    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) {
        return;
    }

    CalculatorCtx* ctx = (CalculatorCtx*)get_top_parent_ctx(btn_id);
    if (!ctx) {
        return;
    }

    char key = btn->control_data.button.text[0];

    // If it's a number key
    if (key >= '0' && key <= '9') {
        if (ctx->new_input) {
            ctx->display_text[0] = '\0';
            ctx->new_input = false;
        }
        int len = strlen(ctx->display_text);
        if (len < 15) {
            ctx->display_text[len] = key;
            ctx->display_text[len + 1] = '\0';
        }
        update_calc_display(ctx);
    } else if (key == 'C') {
        ctx->display_text[0] = '0';
        ctx->display_text[1] = '\0';
        ctx->value1 = 0;
        ctx->op = '\0';
        ctx->new_input = true;
        update_calc_display(ctx);
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        // Parse current value
        ctx->value1 = shell_atoi(ctx->display_text);
        ctx->op = key;
        ctx->new_input = true;
    } else if (key == '=') {
        if (ctx->op != '\0') {
            int32_t val2 = shell_atoi(ctx->display_text);
            int32_t result = 0;
            if (ctx->op == '+') result = ctx->value1 + val2;
            else if (ctx->op == '-') result = ctx->value1 - val2;
            else if (ctx->op == '*') result = ctx->value1 * val2;
            else if (ctx->op == '/') {
                if (val2 != 0) result = ctx->value1 / val2;
                else result = 0;
            }

            // Format result back to text
            ctx->display_text[0] = '\0';
            strcat_itoa(result, ctx->display_text);
            update_calc_display(ctx);
            ctx->op = '\0';
            ctx->new_input = true;
        }
    }
#if BWE_ENABLE_RENDER_TRACE
    serial_write_direct("[CALC] calc_btn_clicked DONE\n");
#endif
}


bwe_error_t calculator_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(300, 100, 240, 320, "Calculator Grid", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    win->flags &= ~BWE_WINDOW_RESIZABLE; // Disable resize for Calculator
    
    CalculatorCtx* ctx = (CalculatorCtx*)kcalloc(1, sizeof(CalculatorCtx));
    ctx->win_id = win_id;
    win->user_data = ctx;
    
    // Background Panel — all controls are children of this panel so hit-test descends correctly
    uint32_t bg_panel = 0;
    BOS_CreatePanel(win_id, 0, 0, 240, 320, 0xFF0F172A, &bg_panel);
    BWE_SetAnchorMode(bg_panel, BWE_ANCHOR_ALL);

    // Display screen Panel — child of bg_panel
    uint32_t scr_panel = 0;
    BOS_CreatePanel(bg_panel, 10, 10, 220, 40, 0xFFE2E8F0, &scr_panel);
    if (scr_panel != 0) {
        BOS_CreateLabel(scr_panel, 10, 12, "0", 0xFF0F172A, &ctx->display_id);
    }
    
    strcpy(ctx->display_text, "0");
    ctx->value1 = 0;
    ctx->op = '\0';
    ctx->new_input = true;
    
    // Buttons grid
    const char* keys[16] = {
        "7", "8", "9", "/",
        "4", "5", "6", "*",
        "1", "2", "3", "-",
        "C", "0", "=", "+"
    };
    
    // Buttons — children of bg_panel, sized 55x55 to eliminate the 5px dead gap
    // (gap clicks previously fell through to bg_panel which silently discarded events)
    uint32_t dummy = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            BOS_CreateButton(bg_panel, 10 + c * 55, 60 + r * 55, 55, 55, keys[r * 4 + c], calc_btn_clicked, &dummy);
            if (dummy != 0) {
                BWE_Window* btn = BWE_GetWindow(dummy);
                if (btn) {
                    btn->control_data.button.bg_color = 0xFFF1F5F9;
                    btn->control_data.button.text_color = 0xFF0F172A;
                }
            }
        }
    }
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// BWE Stress Test Subsystem
// ============================================================
typedef struct {
    uint32_t win_id;
    uint32_t status_lbl_id;
    uint32_t count_lbl_id;
    uint32_t progress_id;
    uint32_t step_counter;
    bool active;
    uint32_t stress_wins[40];
    uint32_t stress_win_count;
} StressCtx;

static void run_stress_step(StressCtx* ctx) {
    ctx->step_counter++;
    
    // Update progress bar
    if (ctx->progress_id != 0) {
        BWE_Window* pb = BWE_GetWindow(ctx->progress_id);
        if (pb) {
            pb->control_data.progressbar.value = (ctx->step_counter % 100);
            BWE_InvalidateWindow(ctx->progress_id);
        }
    }
    
    // Update status label
    if (ctx->status_lbl_id != 0) {
        BWE_Window* lbl = BWE_GetWindow(ctx->status_lbl_id);
        if (lbl) {
            char buf[128];
            strcpy(buf, "Step: ");
            strcat_itoa(ctx->step_counter, buf);
            if (ctx->stress_win_count < 40) {
                strcat(buf, " - Spawning Window");
            } else {
                strcat(buf, " - Random Ops");
            }
            strcpy(lbl->control_data.label.text, buf);
            BWE_InvalidateWindow(ctx->status_lbl_id);
        }
    }

    if (ctx->stress_win_count < 40) {
        // Spawn a new dummy window
        uint32_t new_win_id = 0;
        int32_t rx = 50 + (ctx->stress_win_count * 10) % 600;
        int32_t ry = 80 + (ctx->stress_win_count * 15) % 300;
        char name[32];
        strcpy(name, "Stress #");
        strcat_itoa(ctx->stress_win_count + 1, name);
        
        bwe_error_t err = BOS_CreateWindow(rx, ry, 280, 200, name, &new_win_id);
        if (err == BWE_SUCCESS && new_win_id != 0) {
            ctx->stress_wins[ctx->stress_win_count++] = new_win_id;
            
            // Mark shown explicitly
            BOS_Show(new_win_id);
            
            // Add some controls to it!
            uint32_t dummy = 0;
            BOS_CreateLabel(new_win_id, 10, 10, "BWE V2.1 Stress Test Client", 0xFF0F172A, &dummy);
            BOS_CreateButton(new_win_id, 10, 40, 100, 30, "Action", 0, &dummy);
            BOS_CreateCheckbox(new_win_id, 10, 80, 120, 25, "Option 1", 0, &dummy);
            
            uint32_t pb_id = 0;
            BOS_CreateProgressBar(new_win_id, 10, 120, 240, 20, 0, 100, &pb_id);
            if (pb_id != 0) {
                BWE_Window* pb_inner = BWE_GetWindow(pb_id);
                if (pb_inner) pb_inner->control_data.progressbar.value = 45;
            }
            
            uint32_t lv_id = 0;
            BOS_CreateListView(new_win_id, 140, 40, 120, 70, &lv_id);
            if (lv_id != 0) {
                BOS_ListView_AddItem(lv_id, "Item 1");
                BOS_ListView_AddItem(lv_id, "Item 2");
            }
        }
    } else {
        // We have 40 windows! Randomize ops!
        // Pick a random window
        uint32_t idx = ctx->step_counter % 40;
        uint32_t target_win = ctx->stress_wins[idx];
        if (target_win != 0) {
            BWE_Window* w = BWE_GetWindow(target_win);
            if (w) {
                uint32_t op = (ctx->step_counter / 40) % 5;
                if (op == 0) {
                    // Move it
                    int32_t nx = 50 + (ctx->step_counter * 25) % 600;
                    int32_t ny = 80 + (ctx->step_counter * 17) % 300;
                    BOS_SetBounds(target_win, nx, ny, w->screen_bounds.width, w->screen_bounds.height);
                } else if (op == 1) {
                    // Focus it
                    BOS_SetFocus(target_win);
                } else if (op == 2) {
                    // Minimize/Hide
                    BOS_Hide(target_win);
                } else if (op == 3) {
                    // Restore/Show
                    BOS_Show(target_win);
                } else {
                    // Close and rebuild it!
                    BOS_DestroySurface(target_win);
                    ctx->stress_wins[idx] = 0;
                    
                    // Spawn a new one at a different spot
                    uint32_t new_win_id = 0;
                    int32_t rx = 50 + (ctx->step_counter * 31) % 600;
                    int32_t ry = 80 + (ctx->step_counter * 23) % 300;
                    char name[32];
                    strcpy(name, "Stress #");
                    strcat_itoa(idx + 1, name);
                    
                    bwe_error_t err = BOS_CreateWindow(rx, ry, 280, 200, name, &new_win_id);
                    if (err == BWE_SUCCESS && new_win_id != 0) {
                        ctx->stress_wins[idx] = new_win_id;
                        BOS_Show(new_win_id);
                        
                        uint32_t dummy = 0;
                        BOS_CreateLabel(new_win_id, 10, 10, "BWE V2.1 Stress Test Client", 0xFF0F172A, &dummy);
                        BOS_CreateButton(new_win_id, 10, 40, 100, 30, "Action", 0, &dummy);
                        BOS_CreateCheckbox(new_win_id, 10, 80, 120, 25, "Option 1", 0, &dummy);
                        
                        uint32_t pb_id = 0;
                        BOS_CreateProgressBar(new_win_id, 10, 120, 240, 20, 0, 100, &pb_id);
                        if (pb_id != 0) {
                            BWE_Window* pb_inner = BWE_GetWindow(pb_id);
                            if (pb_inner) pb_inner->control_data.progressbar.value = 65;
                        }
                    }
                }
            }
        }
    }
    
    // Update count label
    if (ctx->count_lbl_id != 0) {
        BWE_Window* clbl = BWE_GetWindow(ctx->count_lbl_id);
        if (clbl) {
            char buf[64];
            strcpy(buf, "Active Windows: ");
            strcat_itoa(ctx->stress_win_count, buf);
            strcpy(clbl->control_data.label.text, buf);
            BWE_InvalidateWindow(ctx->count_lbl_id);
        }
    }
}

static void btn_stress_toggle_clicked(uint32_t btn_id) {
    StressCtx* ctx = (StressCtx*)get_top_parent_ctx(btn_id);
    if (!ctx) return;
    
    ctx->active = !ctx->active;
    
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (btn) {
        if (ctx->active) {
            strcpy(btn->control_data.button.text, "Stop Stress");
        } else {
            strcpy(btn->control_data.button.text, "Start Stress");
        }
        BWE_InvalidateWindow(btn_id);
    }
}

static void stress_test_paint_handler(BWE_Window* self) {
    StressCtx* ctx = (StressCtx*)self->user_data;
    if (!ctx) return;
    
    // Render default panel background
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (fb) {
        BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFFF1F5F9);
    }
    
    // If active, run step every 15 frames (~250ms)
    static uint32_t frame_tick = 0;
    frame_tick++;
    if (ctx->active && (frame_tick % 15 == 0)) {
        run_stress_step(ctx);
    }
    
    // Invalidate main window to keep composer ticking while active
    if (ctx->active) {
        BWE_InvalidateWindow(self->id);
    }
}

bwe_error_t stress_test_init(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(100, 80, 400, 300, "BWE Stress Test Mode", &win_id);
    if (err != BWE_SUCCESS) return err;
    
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    
    StressCtx* ctx = (StressCtx*)kcalloc(1, sizeof(StressCtx));
    ctx->win_id = win_id;
    ctx->step_counter = 0;
    ctx->active = false;
    ctx->stress_win_count = 0;
    win->user_data = ctx;
    win->on_render = stress_test_paint_handler;
    
    uint32_t dummy = 0;
    BOS_CreateLabel(win_id, 20, 20, "BWE V2.1 Stabilization Stress Tester", 0xFF0F172A, &dummy);
    BOS_CreateButton(win_id, 20, 60, 140, 40, "Start Stress", btn_stress_toggle_clicked, &dummy);
    
    BOS_CreateLabel(win_id, 20, 120, "Step: 0", 0xFF475569, &ctx->status_lbl_id);
    BOS_CreateLabel(win_id, 20, 150, "Active Windows: 0", 0xFF475569, &ctx->count_lbl_id);
    
    BOS_CreateProgressBar(win_id, 20, 190, 360, 25, 0, 100, &ctx->progress_id);
    
    if (out_win) *out_win = win_id;
    return BWE_SUCCESS;
}

// ============================================================
// Music Player Implementation
// ============================================================
static void music_canvas_paint(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)clip;
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;

    BWE_Rect b = self->screen_bounds;
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF0F172A);

    BWE_DrawText(fb, "ATOMS Music Player", b.x + 20, b.y + 30, 0xFF3B82F6, 0);
    BWE_DrawText(fb, "Now Playing: DEMO1.wav", b.x + 20, b.y + 70, 0xFFF1F5F9, 0);
    BWE_DrawText(fb, "Status: Streaming via AC97 DMA", b.x + 20, b.y + 100, 0xFF94A3B8, 0);
}

static void btn_play_clicked(uint32_t btn_id) {
    (void)btn_id;
    audio_player_open("/DEMO1.WAV");
    audio_player_play();
}

static void btn_pause_clicked(uint32_t btn_id) {
    (void)btn_id;
    if (audio_player_is_playing()) {
        audio_player_pause();
    } else {
        audio_player_resume();
    }
}

static void btn_stop_clicked(uint32_t btn_id) {
    (void)btn_id;
    audio_player_stop();
}

bwe_error_t music_init_v2(uint32_t* out_win) {
    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(200, 120, 420, 250, "ATOMS Music", &win_id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0002;
    win->flags &= ~BWE_WINDOW_RESIZABLE; // Disable resize for Music Player

    // Bottom control panel
    uint32_t bottom_panel_id = 0;
    BOS_CreatePanel(win_id, 0, 140, 420, 60, 0xFFE2E8F0, &bottom_panel_id);
    BWE_SetAnchorMode(bottom_panel_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_BOTTOM | BWE_ANCHOR_RIGHT);
    
    // Canvas for player visuals
    uint32_t canvas_id = 0;
    BOS_CreateCanvas(win_id, 0, 0, 420, 140, music_canvas_paint, &canvas_id);
    BWE_SetAnchorMode(canvas_id, BWE_ANCHOR_ALL);

    // Play button
    uint32_t dummy = 0;
    BOS_CreateButton(bottom_panel_id, 20, 10, 100, 40, "Play", btn_play_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFF2563EB;
            btn->control_data.button.text_color = 0xFFFFFFFF;
        }
    }

    // Pause button
    BOS_CreateButton(bottom_panel_id, 140, 10, 100, 40, "Pause", btn_pause_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFFEAB308;
            btn->control_data.button.text_color = 0xFF000000;
        }
    }

    // Stop button
    BOS_CreateButton(bottom_panel_id, 260, 10, 100, 40, "Stop", btn_stop_clicked, &dummy);
    if (dummy != 0) {
        BWE_Window* btn = BWE_GetWindow(dummy);
        if (btn) {
            btn->control_data.button.bg_color = 0xFFEF4444;
            btn->control_data.button.text_color = 0xFFFFFFFF;
        }
    }

    if (out_win) *out_win = win_id;
    
    // Auto play for forensic test
    btn_play_clicked(0);
    
    return BWE_SUCCESS;
}
