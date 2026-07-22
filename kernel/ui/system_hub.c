#include "system_hub.h"
#include "bos_shell_panel.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/audio/volume/audio_volume.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/core/lib/include/string.h"

// System Hub Windows & Overlays
static uint32_t g_system_hub_win_id = 0;
static SystemHubPanelType s_active_panel = SYSTEM_HUB_PANEL_NONE;

// Hardware & System States
static uint8_t s_brightness_percent = 85;
static bool s_wifi_on = true;
static bool s_bluetooth_on = true;
static bool s_night_light_on = false;
static int32_t s_cal_view_month = 7; // 1..12
static int32_t s_cal_view_year = 2026;

// Dragging States for Sliders
static bool s_dragging_volume = false;
static bool s_dragging_brightness = false;
static int32_t s_hover_control = -1;

// Notification Ring Buffer Queue
#define MAX_NOTIFICATIONS 16
static BOSNotification s_notifications[MAX_NOTIFICATIONS];
static uint32_t s_notification_count = 0;
static uint32_t s_next_notif_id = 1;
static uint32_t s_unread_count = 0;

// Calendar calculation helpers
static const int s_days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static bool is_leap_year(int y) {
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

static int get_days_in_month(int m, int y) {
    if (m == 2 && is_leap_year(y)) return 29;
    if (m >= 1 && m <= 12) return s_days_per_month[m - 1];
    return 31;
}

static int get_start_day_of_week(int m, int y) {
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + 1) % 7;
}

// Notification Service API
void bos_notify_send(uint32_t app_id, const char* title, const char* message, uint32_t icon_id) {
    if (s_notification_count >= MAX_NOTIFICATIONS) {
        for (uint32_t i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            s_notifications[i] = s_notifications[i + 1];
        }
        s_notification_count = MAX_NOTIFICATIONS - 1;
    }

    BOSNotification* n = &s_notifications[s_notification_count];
    n->id = s_next_notif_id++;
    n->app_id = app_id;
    strncpy(n->title, title ? title : "Notification", 30);
    n->title[30] = '\0';
    strncpy(n->message, message ? message : "", 60);
    n->message[60] = '\0';
    strncpy(n->time_str, "now", 15);
    n->time_str[15] = '\0';
    n->icon_id = icon_id;
    n->is_read = false;

    s_notification_count++;
    s_unread_count++;

    if (g_system_hub_win_id) {
        BWE_InvalidateWindow(g_system_hub_win_id);
    }
}

uint32_t bos_notify_get_unread_count(void) {
    return s_unread_count;
}

void bos_notify_clear_all(void) {
    s_notification_count = 0;
    s_unread_count = 0;
    if (g_system_hub_win_id) {
        BWE_InvalidateWindow(g_system_hub_win_id);
    }
}

// Reposition Flyout Window dynamically
static void update_flyout_window_bounds(void) {
    if (!g_system_hub_win_id) return;
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    int32_t sw = metrics->desktop_rect.width;

    int32_t cap_w = 210;
    int32_t cap_x = sw - cap_w - 16;
    int32_t panel_y = 10 + 32 + BOS_METRIC_FLYOUT_GAP; // 10 + 32 + 12 = 54px

    int32_t panel_w = 310;
    int32_t panel_h = 300;

    if (s_active_panel == SYSTEM_HUB_PANEL_QUICK_SETTINGS) {
        panel_w = 300;
        panel_h = 260; // Content-driven compact height (no wasted empty space!)
    } else if (s_active_panel == SYSTEM_HUB_PANEL_CALENDAR) {
        panel_w = 320;
        RTCDateTime dt;
        if (!rtc_read_datetime(&dt)) { dt.month = 7; dt.year = 2026; }
        int start_day = get_start_day_of_week(s_cal_view_month, s_cal_view_year);
        int total_days = get_days_in_month(s_cal_view_month, s_cal_view_year);
        int num_weeks = (start_day + total_days + 6) / 7;
        panel_h = 144 + num_weeks * 24 + 24; // Content-driven dynamic height with 16px bottom padding!
    } else if (s_active_panel == SYSTEM_HUB_PANEL_NOTIFICATIONS) {
        panel_w = 310;
        panel_h = (s_notification_count == 0) ? 130 : (90 + s_notification_count * 60);
        if (panel_h > 350) panel_h = 350;
    }

    int32_t panel_x = cap_x + cap_w - panel_w;
    if (panel_x < 16) panel_x = 16;

    BOS_SetBounds(g_system_hub_win_id, panel_x, panel_y, panel_w, panel_h);
}

// Public System Hub API
void SystemHub_TogglePanel(SystemHubPanelType panel) {
    if (s_active_panel == panel) {
        s_active_panel = SYSTEM_HUB_PANEL_NONE;
        BOS_Hide(g_system_hub_win_id);
    } else {
        s_active_panel = panel;
        if (panel == SYSTEM_HUB_PANEL_NOTIFICATIONS) {
            s_unread_count = 0;
        }
        update_flyout_window_bounds();
        BOS_Show(g_system_hub_win_id);
        BOS_SetFocus(g_system_hub_win_id);
    }
    if (g_system_hub_win_id) {
        BWE_InvalidateWindow(g_system_hub_win_id);
    }
}

void SystemHub_ClosePanel(void) {
    if (s_active_panel != SYSTEM_HUB_PANEL_NONE) {
        s_active_panel = SYSTEM_HUB_PANEL_NONE;
        BOS_Hide(g_system_hub_win_id);
        if (g_system_hub_win_id) {
            BWE_InvalidateWindow(g_system_hub_win_id);
        }
    }
}

SystemHubPanelType SystemHub_GetActivePanel(void) {
    return s_active_panel;
}

// Render Top-Right Jewelry Status Capsule (Shorter, Thicker, Optically Centered)
void SystemHub_RenderCapsule(const BVFramebuffer* fb, int32_t screen_w, int32_t screen_h) {
    (void)screen_h;
    int32_t cap_w = 210;
    int32_t cap_h = 32;
    int32_t cap_x = screen_w - cap_w - 16;
    int32_t cap_y = 10;

    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    bool is_hovered = (s_hover_control == 100);

    // 1. Draw Premium Glassmorphic Capsule (Background, Border 0.16, Inner Glow 0.06, Blur Approx)
    draw_bos_glass_capsule(fb, cap_x, cap_y, cap_w, cap_h, 16, is_hovered, &clip);

    // Optical Centering
    int32_t icon_y = cap_y + 8; // 16px icon centered vertically in 32px capsule

    // 2. State-Based Status PNG Icons (16px visual size, 24px+ hit areas)
    uint32_t wifi_asset = s_wifi_on ? ICON_SYS_WIFI_CONN : ICON_SYS_WIFI_DISC;
    BOAsset_DrawAsset(wifi_asset, cap_x + 12, icon_y, 16, 16);

    uint8_t cur_vol = audio_volume_get_master();
    bool cur_mute = audio_volume_get_mute();
    uint32_t vol_asset = (cur_mute || cur_vol == 0) ? ICON_SYS_VOL_MUTE : ((cur_vol < 100) ? ICON_SYS_VOL_LOW : ICON_SYS_VOL_NORM);
    BOAsset_DrawAsset(vol_asset, cap_x + 40, icon_y, 16, 16);

    uint32_t bat_asset = ICON_SYS_BAT_NORM;
    BOAsset_DrawAsset(bat_asset, cap_x + 66, icon_y, 16, 16);

    // 3. Vertical Separator Line (1px wide, soft white @ 20% opacity, 18px height)
    draw_bos_separator(fb, cap_x + 94, cap_y + 7, cap_x + 94, cap_y + cap_h - 7, 0x33FFFFFF, &clip);

    // 4. RTC Live Time Anchor ("3:54 PM" - Bold White 100% Opacity)
    RTCDateTime dt;
    if (!rtc_read_datetime(&dt)) {
        dt.hour = 11; dt.minute = 21; dt.second = 45;
        dt.month = 7; dt.day = 22; dt.year = 2026;
    }

    char time_str[12];
    int display_hour = dt.hour % 12;
    if (display_hour == 0) display_hour = 12;
    const char* ampm = (dt.hour >= 12) ? "PM" : "AM";

    time_str[0] = (display_hour / 10) ? ('0' + (display_hour / 10)) : ' ';
    time_str[1] = '0' + (display_hour % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (dt.minute / 10);
    time_str[4] = '0' + (dt.minute % 10);
    time_str[5] = ' ';
    time_str[6] = ampm[0];
    time_str[7] = ampm[1];
    time_str[8] = '\0';

    BWE_DrawText(fb, time_str, cap_x + 108, cap_y + 8, 0xFFFFFFFF, 0);

    // 5. Right Notification Bell Icon (State: Normal / Unread Dot Badge)
    uint32_t bell_asset = (s_unread_count > 0) ? ICON_SYS_BELL_UNREAD : ICON_SYS_BELL_NORM;
    BOAsset_DrawAsset(bell_asset, cap_x + cap_w - 28, icon_y, 16, 16);
}

// Handle Capsule Click
bool SystemHub_HandleCapsuleClick(int32_t mx, int32_t my, int32_t screen_w, int32_t screen_h) {
    (void)screen_h;
    int32_t cap_w = 210;
    int32_t cap_h = 32;
    int32_t cap_x = screen_w - cap_w - 16;
    int32_t cap_y = 10;

    if (mx >= cap_x && mx < cap_x + cap_w && my >= cap_y && my < cap_y + cap_h) {
        int32_t rel_x = mx - cap_x;

        if (rel_x < 92) {
            SystemHub_TogglePanel(SYSTEM_HUB_PANEL_QUICK_SETTINGS);
        } else if (rel_x < cap_w - 34) {
            SystemHub_TogglePanel(SYSTEM_HUB_PANEL_CALENDAR);
        } else {
            SystemHub_TogglePanel(SYSTEM_HUB_PANEL_NOTIFICATIONS);
        }
        return true;
    }
    return false;
}

// Render Quick Settings Control Panel (V1.1 Focused Polish Pass)
static void render_quick_settings_panel(const BVFramebuffer* fb, BWE_Window* self) {
    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    int32_t px = self->screen_bounds.x;
    int32_t py = self->screen_bounds.y;
    int32_t pw = self->screen_bounds.width;
    int32_t ph = self->screen_bounds.height;

    uint32_t bg_col     = BOTHEME_GetColor(BOTHEME_TASKBAR_BG);
    uint32_t border_col = BOTHEME_GetColor(BOTHEME_TASKBAR_BORDER);
    if ((bg_col & 0x00FFFFFF) != 0) {
        bg_col = 0xFA000000 | (bg_col & 0x00FFFFFF);
    } else {
        bg_col = 0xFA0F172A;
    }
    if ((border_col & 0x00FFFFFF) == 0) border_col = 0xFF334155;

    // Outer Panel Container
    draw_bos_panel(fb, px, py, pw, ph, 14, bg_col, border_col, &clip);

    // Header
    BWE_DrawText(fb, "Quick Settings", px + 14, py + 12, 0xFFF1F5F9, 0);

    // 1. Brightness Control Row Container
    draw_bos_rounded_box(fb, px + 12, py + 32, pw - 24, 30, 6, 0x1A1E293B, &clip);
    draw_vector_sun(fb, px + 24, py + 47, 0xFF38BDF8, &clip);
    draw_bos_slider(fb, px + 44, py + 37, pw - 66, 20, s_brightness_percent, (s_hover_control == 1), s_dragging_brightness, 0xFF38BDF8, &clip);

    // 2. Volume Control Row Container + Integrated Mute Pill
    uint8_t cur_vol_pct = (uint32_t)audio_volume_get_master() * 100 / 255;
    bool cur_mute = audio_volume_get_mute();

    draw_bos_rounded_box(fb, px + 12, py + 66, pw - 24, 30, 6, 0x1A1E293B, &clip);
    draw_vector_volume(fb, px + 24, py + 81, cur_mute, cur_mute ? 0xFFEF4444 : 0xFF3B82F6, &clip);
    draw_bos_slider(fb, px + 44, py + 71, pw - 98, 20, cur_vol_pct, (s_hover_control == 2), s_dragging_volume, cur_mute ? 0xFFEF4444 : 0xFF3B82F6, &clip);

    // Integrated Mute Pill Button
    draw_bos_rounded_box(fb, px + pw - 48, py + 70, 30, 22, 5, cur_mute ? 0xFFEF4444 : (s_hover_control == 3 ? 0x33FFFFFF : 0x1AFFFFFF), &clip);
    BWE_DrawText(fb, "M", px + pw - 38, py + 73, 0xFFFFFFFF, 0);

    // 3. Quick Action Tiles Grid (2x2)
    int32_t tile_w = (pw - 34) / 2;
    int32_t tile_h = 48;
    int32_t grid_y = py + 104;

    draw_bos_tile(fb, px + 12, grid_y, tile_w, tile_h, "Wi-Fi", s_wifi_on ? "Connected" : "Off", ICON_SYS_WIFI_CONN, s_wifi_on, (s_hover_control == 10), false, &clip);
    draw_bos_tile(fb, px + 22 + tile_w, grid_y, tile_w, tile_h, "Bluetooth", s_bluetooth_on ? "On" : "Off", 0, s_bluetooth_on, (s_hover_control == 11), false, &clip);
    draw_bos_tile(fb, px + 12, grid_y + 54, tile_w, tile_h, "Battery", "87%", ICON_SYS_BAT_NORM, false, (s_hover_control == 12), false, &clip);
    draw_bos_tile(fb, px + 22 + tile_w, grid_y + 54, tile_w, tile_h, "Night Light", s_night_light_on ? "On" : "Off", 0, s_night_light_on, (s_hover_control == 13), false, &clip);

    // 4. Compact Footer with Settings Shortcut Button
    int32_t footer_y = py + ph - 34;
    draw_bos_separator(fb, px + 14, footer_y, px + pw - 14, footer_y, 0x33FFFFFF, &clip);

    draw_bos_rounded_box(fb, px + pw - 96, footer_y + 5, 82, 24, 6, (s_hover_control == 99) ? 0x33FFFFFF : 0x1AFFFFFF, &clip);
    BOAsset_DrawAsset(ICON_SETTINGS, px + pw - 92, footer_y + 8, 18, 18);
    BWE_DrawText(fb, "Settings", px + pw - 70, footer_y + 9, 0xFFF1F5F9, 0);
}

// Render Calendar & Clock Panel (V1.1 Focused Polish Pass)
static void render_calendar_panel(const BVFramebuffer* fb, BWE_Window* self) {
    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    int32_t px = self->screen_bounds.x;
    int32_t py = self->screen_bounds.y;
    int32_t pw = self->screen_bounds.width;
    int32_t ph = self->screen_bounds.height;

    uint32_t bg_col     = BOTHEME_GetColor(BOTHEME_TASKBAR_BG);
    uint32_t border_col = BOTHEME_GetColor(BOTHEME_TASKBAR_BORDER);
    if ((bg_col & 0x00FFFFFF) != 0) {
        bg_col = 0xFA000000 | (bg_col & 0x00FFFFFF);
    } else {
        bg_col = 0xFA0F172A;
    }
    if ((border_col & 0x00FFFFFF) == 0) border_col = 0xFF334155;

    // Outer Panel Container
    draw_bos_panel(fb, px, py, pw, ph, 14, bg_col, border_col, &clip);

    // 1. Live RTC Time & Full Date Header
    RTCDateTime dt;
    if (!rtc_read_datetime(&dt)) {
        dt.hour = 11; dt.minute = 21; dt.second = 45;
        dt.month = 7; dt.day = 22; dt.year = 2026;
    }

    char time_buf[16];
    int display_hour = dt.hour % 12;
    if (display_hour == 0) display_hour = 12;
    const char* ampm = (dt.hour >= 12) ? "PM" : "AM";

    time_buf[0] = (display_hour / 10) ? ('0' + (display_hour / 10)) : ' ';
    time_buf[1] = '0' + (display_hour % 10);
    time_buf[2] = ':';
    time_buf[3] = '0' + (dt.minute / 10);
    time_buf[4] = '0' + (dt.minute % 10);
    time_buf[5] = ':';
    time_buf[6] = '0' + (dt.second / 10);
    time_buf[7] = '0' + (dt.second % 10);
    time_buf[8] = ' ';
    time_buf[9] = ampm[0];
    time_buf[10] = ampm[1];
    time_buf[11] = '\0';

    BWE_DrawText(fb, time_buf, px + 16, py + 14, 0xFF38BDF8, 0);

    const char* day_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    int wday = get_start_day_of_week(dt.month, dt.year);
    const char* month_names[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

    char date_buf[36];
    strncpy(date_buf, day_names[wday % 7], 4);
    strcat(date_buf, ", ");
    const char* cur_mname = (dt.month >= 1 && dt.month <= 12) ? month_names[dt.month - 1] : "July";
    strcat(date_buf, cur_mname);
    char day_yr_str[16];
    day_yr_str[0] = ' ';
    day_yr_str[1] = (dt.day >= 10) ? ('0' + (dt.day / 10)) : ('0' + dt.day);
    day_yr_str[2] = (dt.day >= 10) ? ('0' + (dt.day % 10)) : '\0';
    if (dt.day >= 10) day_yr_str[3] = '\0';
    strcat(day_yr_str, ", 2026");
    strcat(date_buf, day_yr_str);

    // Measure exact text width to prevent right-edge clipping completely!
    int date_len = strlen(date_buf);
    int32_t date_x = px + pw - 16 - (date_len * 8);
    BWE_DrawText(fb, date_buf, date_x, py + 14, 0xFF94A3B8, 0);

    draw_bos_separator(fb, px + 16, py + 36, px + pw - 16, py + 36, 0x33FFFFFF, &clip);

    // 2. Month Navigation Header Row
    const char* view_mname = (s_cal_view_month >= 1 && s_cal_view_month <= 12) ? month_names[s_cal_view_month - 1] : "July";
    BWE_DrawText(fb, view_mname, px + 16, py + 46, 0xFFF1F5F9, 0);

    // Touch-friendly navigation buttons with clear hover targets inside safe margins
    draw_bos_rounded_box(fb, px + pw - 52, py + 44, 20, 20, 5, (s_hover_control == 30) ? 0x33FFFFFF : 0x1AFFFFFF, &clip);
    BWE_DrawText(fb, "<", px + pw - 46, py + 47, 0xFFF1F5F9, 0);

    draw_bos_rounded_box(fb, px + pw - 28, py + 44, 20, 20, 5, (s_hover_control == 31) ? 0x33FFFFFF : 0x1AFFFFFF, &clip);
    BWE_DrawText(fb, ">", px + pw - 22, py + 47, 0xFFF1F5F9, 0);

    // 3. 7-Column Mathematically Centered Grid
    const char* day_headers[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
    int32_t col_w = (pw - 32) / 7;
    int32_t grid_x = px + 16;
    int32_t grid_y = py + 72;

    for (int i = 0; i < 7; i++) {
        int32_t hx = grid_x + i * col_w + (col_w - 16) / 2;
        BWE_DrawText(fb, day_headers[i], hx, grid_y, 0xFF64748B, 0);
    }

    // Days Grid
    int start_day = get_start_day_of_week(s_cal_view_month, s_cal_view_year);
    int total_days = get_days_in_month(s_cal_view_month, s_cal_view_year);
    int current_day_num = (dt.month == s_cal_view_month && dt.year == s_cal_view_year) ? dt.day : -1;

    int32_t days_y = grid_y + 20;
    int day_counter = 1;

    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            int cell_index = row * 7 + col;
            if (cell_index >= start_day && day_counter <= total_days) {
                int32_t cx = grid_x + col * col_w;
                int32_t cy = days_y + row * 24;

                bool is_today = (day_counter == current_day_num);
                if (is_today) {
                    draw_bos_rounded_box(fb, cx + (col_w - 22) / 2, cy - 2, 22, 22, 11, 0xFF2563EB, &clip);
                }

                char num_buf[4];
                num_buf[0] = (day_counter >= 10) ? ('0' + (day_counter / 10)) : ('0' + day_counter);
                num_buf[1] = (day_counter >= 10) ? ('0' + (day_counter % 10)) : '\0';
                num_buf[2] = '\0';

                int32_t tx = cx + (col_w - (strlen(num_buf) * 8)) / 2;
                BWE_DrawText(fb, num_buf, tx, cy + 2, is_today ? 0xFFFFFFFF : 0xFFF1F5F9, 0);

                day_counter++;
            }
        }
    }

    // 4. Content-Driven Footer (16px bottom padding, zero clipping)
    int num_weeks = (start_day + total_days + 6) / 7;
    int32_t footer_y = days_y + num_weeks * 24 + 8;

    draw_bos_separator(fb, px + 16, footer_y, px + pw - 16, footer_y, 0x33FFFFFF, &clip);
    BWE_DrawText(fb, "Today - No scheduled events", px + 16, footer_y + 10, 0xFF64748B, 0);
}

// Render Notification Center Panel
static void render_notification_panel(const BVFramebuffer* fb, BWE_Window* self) {
    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    int32_t px = self->screen_bounds.x;
    int32_t py = self->screen_bounds.y;
    int32_t pw = self->screen_bounds.width;
    int32_t ph = self->screen_bounds.height;

    uint32_t bg_col     = BOTHEME_GetColor(BOTHEME_TASKBAR_BG);
    uint32_t border_col = BOTHEME_GetColor(BOTHEME_TASKBAR_BORDER);
    if ((bg_col & 0x00FFFFFF) != 0) {
        bg_col = 0xFA000000 | (bg_col & 0x00FFFFFF);
    } else {
        bg_col = 0xFA0F172A;
    }
    if ((border_col & 0x00FFFFFF) == 0) border_col = 0xFF334155;

    draw_bos_panel(fb, px, py, pw, ph, 14, bg_col, border_col, &clip);

    // Header & Clear All button
    BWE_DrawText(fb, "Notifications", px + 14, py + 12, 0xFFF1F5F9, 0);

    if (s_notification_count > 0) {
        draw_bos_rounded_box(fb, px + pw - 76, py + 8, 62, 20, 5, (s_hover_control == 40) ? 0x33FFFFFF : 0x1AFFFFFF, &clip);
        BWE_DrawText(fb, "Clear All", px + pw - 70, py + 11, 0xFF38BDF8, 0);
    }

    draw_bos_separator(fb, px + 14, py + 34, px + pw - 14, py + 34, 0x33FFFFFF, &clip);

    // Notification Cards or Empty State
    if (s_notification_count == 0) {
        draw_vector_bell(fb, px + pw / 2, py + 65, 0xFF475569, &clip);
        BWE_DrawText(fb, "No new notifications", px + (pw - 160) / 2, py + 85, 0xFF64748B, 0);
    } else {
        int32_t card_y = py + 44;
        int32_t card_h = 48;
        for (int i = (int)s_notification_count - 1; i >= 0 && card_y + card_h < py + ph - 8; i--) {
            bool is_hovered = (s_hover_control == (50 + i));
            draw_bos_notification_card(fb, px + 14, card_y, pw - 28, card_h, s_notifications[i].title, s_notifications[i].message, s_notifications[i].time_str, s_notifications[i].icon_id, is_hovered, &clip);
            card_y += card_h + 8;
        }
    }
}

// Master Render Callback
static void system_hub_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    if (s_active_panel == SYSTEM_HUB_PANEL_QUICK_SETTINGS) {
        render_quick_settings_panel(fb, self);
    } else if (s_active_panel == SYSTEM_HUB_PANEL_CALENDAR) {
        render_calendar_panel(fb, self);
    } else if (s_active_panel == SYSTEM_HUB_PANEL_NOTIFICATIONS) {
        render_notification_panel(fb, self);
    }
}

// Master Event Callback
static void system_hub_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t px = self->screen_bounds.x;
    int32_t py = self->screen_bounds.y;
    int32_t pw = self->screen_bounds.width;
    int32_t ph = self->screen_bounds.height;

    if (event->type == BWE_EVENT_MOUSE_MOVE) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        if (s_dragging_volume) {
            int32_t rel_x = mx - (px + 40);
            int32_t slider_w = pw - 84;
            if (rel_x < 0) rel_x = 0;
            if (rel_x > slider_w) rel_x = slider_w;
            uint8_t new_vol = (rel_x * 255) / slider_w;
            audio_volume_set_master(new_vol);
            audio_volume_set_mute(false);
            BWE_InvalidateWindow(window_id);
            return;
        }

        if (s_dragging_brightness) {
            int32_t rel_x = mx - (px + 40);
            int32_t slider_w = pw - 54;
            if (rel_x < 0) rel_x = 0;
            if (rel_x > slider_w) rel_x = slider_w;
            s_brightness_percent = (rel_x * 100) / slider_w;
            BWE_InvalidateWindow(window_id);
            return;
        }

        int32_t new_hover = -1;
        if (s_active_panel == SYSTEM_HUB_PANEL_QUICK_SETTINGS) {
            if (mx >= px + 44 && mx < px + pw - 22 && my >= py + 32 && my < py + 62) new_hover = 1;
            else if (mx >= px + 44 && mx < px + pw - 50 && my >= py + 66 && my < py + 96) new_hover = 2;
            else if (mx >= px + pw - 48 && mx < px + pw - 18 && my >= py + 70 && my < py + 92) new_hover = 3;
            else if (mx >= px + pw - 96 && mx < px + pw - 14 && my >= py + ph - 29 && my < py + ph - 5) new_hover = 99;
            else {
                int32_t tile_w = (pw - 34) / 2;
                int32_t tile_h = 48;
                int32_t grid_y = py + 104;
                if (mx >= px + 12 && mx < px + 12 + tile_w && my >= grid_y && my < grid_y + tile_h) new_hover = 10;
                else if (mx >= px + 22 + tile_w && mx < px + 22 + 2 * tile_w && my >= grid_y && my < grid_y + tile_h) new_hover = 11;
                else if (mx >= px + 12 && mx < px + 12 + tile_w && my >= grid_y + 54 && my < grid_y + 54 + tile_h) new_hover = 12;
                else if (mx >= px + 22 + tile_w && mx < px + 22 + 2 * tile_w && my >= grid_y + 54 && my < grid_y + 54 + tile_h) new_hover = 13;
            }
        } else if (s_active_panel == SYSTEM_HUB_PANEL_CALENDAR) {
            if (mx >= px + pw - 52 && mx < px + pw - 32 && my >= py + 44 && my < py + 64) new_hover = 30;
            else if (mx >= px + pw - 28 && mx < px + pw - 8 && my >= py + 44 && my < py + 64) new_hover = 31;
        } else if (s_active_panel == SYSTEM_HUB_PANEL_NOTIFICATIONS) {
            if (mx >= px + pw - 76 && mx < px + pw - 14 && my >= py + 8 && my < py + 28) new_hover = 40;
            else {
                int32_t card_y = py + 44;
                int32_t card_h = 48;
                for (int i = (int)s_notification_count - 1; i >= 0; i--) {
                    if (mx >= px + 14 && mx < px + pw - 14 && my >= card_y && my < card_y + card_h) {
                        new_hover = 50 + i;
                        break;
                    }
                    card_y += card_h + 8;
                }
            }
        }

        if (new_hover != s_hover_control) {
            s_hover_control = new_hover;
            BWE_InvalidateWindow(window_id);
        }
        return;
    }

    if (event->type == BWE_EVENT_MOUSE_UP) {
        if (s_dragging_volume || s_dragging_brightness) {
            s_dragging_volume = false;
            s_dragging_brightness = false;
            BWE_InvalidateWindow(window_id);
        }
        return;
    }

    if (event->type == BWE_EVENT_KEY_DOWN) {
        if (event->data.key.key_code == 27) { // ESC
            SystemHub_ClosePanel();
            return;
        }
    }

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        if (mx < px || mx >= px + pw || my < py || my >= py + ph) {
            SystemHub_ClosePanel();
            return;
        }

        if (s_active_panel == SYSTEM_HUB_PANEL_QUICK_SETTINGS) {
            if (mx >= px + 44 && mx < px + pw - 22 && my >= py + 32 && my < py + 62) {
                int32_t rel_x = mx - (px + 44);
                s_brightness_percent = (rel_x * 100) / (pw - 66);
                s_dragging_brightness = true;
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + 44 && mx < px + pw - 50 && my >= py + 66 && my < py + 96) {
                int32_t rel_x = mx - (px + 44);
                uint8_t new_vol = (rel_x * 255) / (pw - 98);
                audio_volume_set_master(new_vol);
                audio_volume_set_mute(false);
                s_dragging_volume = true;
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + pw - 48 && mx < px + pw - 18 && my >= py + 66 && my < py + 96) {
                bool cur_mute = audio_volume_get_mute();
                audio_volume_set_mute(!cur_mute);
                BWE_InvalidateWindow(window_id);
                return;
            }
            int32_t tile_w = (pw - 34) / 2;
            int32_t tile_h = 48;
            int32_t grid_y = py + 104;

            if (mx >= px + 12 && mx < px + 12 + tile_w && my >= grid_y && my < grid_y + tile_h) {
                s_wifi_on = !s_wifi_on;
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + 22 + tile_w && mx < px + 22 + 2 * tile_w && my >= grid_y && my < grid_y + tile_h) {
                s_bluetooth_on = !s_bluetooth_on;
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + 22 + tile_w && mx < px + 22 + 2 * tile_w && my >= grid_y + 54 && my < grid_y + 54 + tile_h) {
                s_night_light_on = !s_night_light_on;
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + pw - 96 && mx < px + pw - 14 && my >= py + ph - 29 && my < py + ph - 5) {
                SystemHub_ClosePanel();
                horse_launch(APP_ID_SETTINGS);
                return;
            }
        } else if (s_active_panel == SYSTEM_HUB_PANEL_CALENDAR) {
            if (mx >= px + pw - 52 && mx < px + pw - 32 && my >= py + 44 && my < py + 64) {
                s_cal_view_month--;
                if (s_cal_view_month < 1) { s_cal_view_month = 12; s_cal_view_year--; }
                update_flyout_window_bounds();
                BWE_InvalidateWindow(window_id);
                return;
            }
            if (mx >= px + pw - 28 && mx < px + pw - 8 && my >= py + 44 && my < py + 64) {
                s_cal_view_month++;
                if (s_cal_view_month > 12) { s_cal_view_month = 1; s_cal_view_year++; }
                update_flyout_window_bounds();
                BWE_InvalidateWindow(window_id);
                return;
            }
        } else if (s_active_panel == SYSTEM_HUB_PANEL_NOTIFICATIONS) {
            if (mx >= px + pw - 76 && mx < px + pw - 14 && my >= py + 8 && my < py + 28) {
                bos_notify_clear_all();
                update_flyout_window_bounds();
                return;
            }
            int32_t card_y = py + 44;
            int32_t card_h = 48;
            for (int i = (int)s_notification_count - 1; i >= 0; i--) {
                if (mx >= px + pw - 28 && mx < px + pw - 14 && my >= card_y + card_h - 18 && my < card_y + card_h) {
                    for (uint32_t k = i; k < s_notification_count - 1; k++) {
                        s_notifications[k] = s_notifications[k + 1];
                    }
                    s_notification_count--;
                    update_flyout_window_bounds();
                    BWE_InvalidateWindow(window_id);
                    return;
                }
                card_y += card_h + 8;
            }
        }
    }
}

void SystemHub_Initialize(void) {
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    int32_t sw = metrics->desktop_rect.width;

    int32_t cap_w = 210;
    int32_t cap_x = sw - cap_w - 16;
    int32_t panel_y = 10 + 32 + BOS_METRIC_FLYOUT_GAP; // 54px

    int32_t panel_w = 310;
    int32_t panel_h = 295;
    int32_t panel_x = cap_x + cap_w - panel_w;

    BOS_CreatePanel(BWE_DESKTOP_ID, panel_x, panel_y, panel_w, panel_h, 0x00000000, &g_system_hub_win_id);
    BWE_Window* hub = BWE_GetWindow(g_system_hub_win_id);
    if (hub) {
        hub->type = BWE_TYPE_PANEL;
        hub->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST | BWE_WINDOW_TRANSPARENT;
        hub->on_render = system_hub_render_callback;
        hub->on_event = system_hub_event_callback;
        BOS_Hide(g_system_hub_win_id);
        s_active_panel = SYSTEM_HUB_PANEL_NONE;
    }
}
