#include "theme_engine.h"

// Hardcoded for now, can be loaded dynamically later
#define COLOR_ACTIVE_TITLEBAR   0xFF0055AA // Blue
#define COLOR_INACTIVE_TITLEBAR 0xFF888888 // Gray
#define COLOR_ACTIVE_BORDER     0xFF003366 // Dark Blue
#define COLOR_INACTIVE_BORDER   0xFF555555 // Dark Gray
#define COLOR_TITLE_TEXT        0xFFFFFFFF // White
static const BOVISUAL_Color THEME_COLOR_DESKTOP_BG = {25, 25, 50, 255};
static const BOVISUAL_Color THEME_COLOR_START_MENU = {40, 40, 40, 255};

BOVISUAL_Color theme_get_control_bg(bool is_focused, bool is_hovered, bool is_pressed) {
    if (is_pressed) return (BOVISUAL_Color){50, 150, 250, 255};
    if (is_hovered) return (BOVISUAL_Color){80, 180, 255, 255};
    if (is_focused) return (BOVISUAL_Color){70, 70, 70, 255};
    return (BOVISUAL_Color){60, 60, 60, 255};
}

BOVISUAL_Color theme_get_control_fg(bool is_disabled) {
    if (is_disabled) return (BOVISUAL_Color){150, 150, 150, 255};
    return (BOVISUAL_Color){255, 255, 255, 255};
}

BOVISUAL_Color theme_get_control_border(bool is_focused) {
    if (is_focused) return (BOVISUAL_Color){100, 200, 255, 255};
    return (BOVISUAL_Color){0, 0, 0, 255};
}

void theme_draw_window_frame(struct BOSSurface* surface, const char* title, bool is_focused, const BVRect* clip) {
    if (!surface) return;

    BOVISUAL_Color border_color = is_focused ? COLOR_ACTIVE_BORDER : COLOR_INACTIVE_BORDER;
    BOVISUAL_Color titlebar_color = is_focused ? COLOR_ACTIVE_TITLEBAR : COLOR_INACTIVE_TITLEBAR;

    // Draw borders
    BVRect full_rect = {0, 0, surface->width, surface->height};
    
    // Top border (titlebar)
    BVRect title_rect = {0, 0, surface->width, THEME_TITLEBAR_HEIGHT};
    painter_fill_rect(surface, &title_rect, titlebar_color, clip);
    
    // Left, Right, Bottom borders
    BVRect left_border = {0, THEME_TITLEBAR_HEIGHT, THEME_BORDER_WIDTH, surface->height - THEME_TITLEBAR_HEIGHT};
    BVRect right_border = {surface->width - THEME_BORDER_WIDTH, THEME_TITLEBAR_HEIGHT, THEME_BORDER_WIDTH, surface->height - THEME_TITLEBAR_HEIGHT};
    BVRect bottom_border = {0, surface->height - THEME_BORDER_WIDTH, surface->width, THEME_BORDER_WIDTH};
    
    painter_fill_rect(surface, &left_border, border_color, clip);
    painter_fill_rect(surface, &right_border, border_color, clip);
    painter_fill_rect(surface, &bottom_border, border_color, clip);
    
    // Draw Close Button
    BVRect close_rect = {
        surface->width - THEME_BUTTON_SIZE - 4,
        4,
        THEME_BUTTON_SIZE,
        THEME_BUTTON_SIZE
    };
    painter_fill_rect(surface, &close_rect, COLOR_CLOSE_BTN, clip);
    
    // TODO: Draw title text (requires font engine integration)
    (void)title;
}

void theme_draw_client_background(struct BOSSurface* surface, bool is_focused, const BVRect* clip) {
    if (!surface) return;
    (void)is_focused; // Unused for now
    
    BVRect client_rect = {
        THEME_BORDER_WIDTH,
        THEME_TITLEBAR_HEIGHT,
        surface->width - (THEME_BORDER_WIDTH * 2),
        surface->height - THEME_TITLEBAR_HEIGHT - THEME_BORDER_WIDTH
    };
    
    painter_fill_rect(surface, &client_rect, COLOR_CLIENT_BG, clip);
}

// Shell colors
#define COLOR_TASKBAR_BG        0xFF202020 // Dark Gray
#define COLOR_START_NORMAL      0xFF0078D7 // Windows Blue-ish
#define COLOR_START_HOVER       0xFF005A9E // Darker Blue
#define COLOR_START_PRESSED     0xFF004070
#define COLOR_DESKTOP_BG        0xFF008080 // Teal (ATOMS classic)
#define COLOR_ICON_TEXT         0xFFFFFFFF
#define COLOR_ICON_SELECTED_BG  0x800078D7 // Semi-transparent blue
#define COLOR_ICON_HOVER_BG     0x40FFFFFF // Semi-transparent white

void theme_draw_taskbar(struct BOSSurface* surface, const BVRect* clip) {
    if (!surface) return;
    BVRect taskbar_rect = {0, 0, surface->width, surface->height};
    painter_fill_rect(surface, &taskbar_rect, COLOR_TASKBAR_BG, clip);
}

void theme_draw_start_button(struct BOSSurface* surface, int x, int y, int width, int height, int state, const BVRect* clip) {
    if (!surface) return;
    
    BOVISUAL_Color btn_color = COLOR_START_NORMAL;
    if (state == 1) btn_color = COLOR_START_HOVER;       // 1 = Hover
    else if (state == 2) btn_color = COLOR_START_PRESSED; // 2 = Pressed
    
    BVRect btn_rect = {x, y, width, height};
    painter_fill_rect(surface, &btn_rect, btn_color, clip);
    
    // Stub for drawing "Start" text
    // painter_draw_text(surface, x + 10, y + 4, "Start", COLOR_ICON_TEXT, clip);
}

void theme_draw_desktop(struct BOSSurface* surface, const BVRect* clip) {
    if (!surface) return;
    BVRect bg_rect = {0, 0, surface->width, surface->height};
    painter_fill_rect(surface, &bg_rect, COLOR_DESKTOP_BG, clip);
}

#include "kernel/ui/icon_engine/include/icon_engine.h"

void theme_draw_icon(struct BOSSurface* surface, int x, int y, const char* label, int icon_id, bool is_selected, bool is_hovered, const BVRect* clip) {
    if (!surface || !surface->framebuffer) return;
    
    (void)icon_id;
    
    BVRect icon_rect = {x, y, 64, 64}; // 64x64 icon box
    
    if (is_selected) {
        painter_fill_rect(surface, &icon_rect, COLOR_ICON_SELECTED_BG, clip);
    } else if (is_hovered) {
        painter_fill_rect(surface, &icon_rect, COLOR_ICON_HOVER_BG, clip);
    }
    
    // Resolve canonical IconId
    IconId ico = ICON_ID_FOLDER;
    if (label) {
        if (strstr(label, "Terminal")) ico = ICON_ID_TERMINAL;
        else if (strstr(label, "Music") || strstr(label, "Media")) ico = ICON_ID_MEDIA_PLAYER;
        else if (strstr(label, "Settings")) ico = ICON_ID_SETTINGS;
        else if (strstr(label, "Files") || strstr(label, "Explorer")) ico = ICON_ID_EXPLORER;
        else if (strstr(label, "Calculator")) ico = ICON_ID_CALCULATOR;
    }

    BVFramebuffer fb;
    fb.buffer = surface->framebuffer;
    fb.width = (uint32_t)surface->width;
    fb.height = (uint32_t)surface->height;
    fb.pitch = (uint32_t)surface->width * 4;

    IconRenderContext ctx;
    ctx.x = x + 10;
    ctx.y = y + 4;
    ctx.width = 44;
    ctx.height = 44;
    ctx.state = is_selected ? ICON_STATE_ACTIVE : (is_hovered ? ICON_STATE_HOVER : ICON_STATE_NORMAL);
    ctx.accent_color = 0;
    ctx.clip = NULL;

    if (!IconEngine_Render(&fb, ico, &ctx)) {
        // Fallback inner box
        BVRect graphic_rect = {x + 16, y + 8, 32, 32};
        painter_fill_rect(surface, &graphic_rect, 0xFFFFFFFF, clip);
    }
}

// Start Menu Colors
#define COLOR_STARTMENU_BG      0xFF1C1C1C // Very dark gray
#define COLOR_STARTMENU_BORDER  0xFF404040 // Medium gray
#define COLOR_MENU_ITEM_HOVER   0xFF303030
#define COLOR_MENU_TEXT         0xFFFFFFFF

void theme_draw_start_menu(struct BOSSurface* surface, const BVRect* clip) {
    if (!surface) return;
    
    BVRect full_rect = {0, 0, surface->width, surface->height};
    painter_fill_rect(surface, &full_rect, COLOR_STARTMENU_BG, clip);
    
    // Draw border
    BVRect top_border = {0, 0, surface->width, 1};
    BVRect right_border = {surface->width - 1, 0, 1, surface->height};
    painter_fill_rect(surface, &top_border, COLOR_STARTMENU_BORDER, clip);
    painter_fill_rect(surface, &right_border, COLOR_STARTMENU_BORDER, clip);
}

void theme_draw_menu_item(struct BOSSurface* surface, int x, int y, int width, int height, const char* label, int icon_id, bool is_hovered, const BVRect* clip) {
    if (!surface) return;
    
    (void)label;
    (void)icon_id;
    
    BVRect item_rect = {x, y, width, height};
    
    if (is_hovered) {
        painter_fill_rect(surface, &item_rect, COLOR_MENU_ITEM_HOVER, clip);
    }
    
    // Placeholder icon
    BVRect icon_rect = {x + 4, y + 4, 32, 32};
    painter_fill_rect(surface, &icon_rect, 0xFF888888, clip);
    
    // Stub for text
    // painter_draw_text(surface, x + 40, y + 10, label, COLOR_MENU_TEXT, clip);
}


