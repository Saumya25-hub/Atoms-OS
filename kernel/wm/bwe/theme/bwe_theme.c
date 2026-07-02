#include "../include/bwe.h"

static bool s_dark_mode = false;

// Theme color arrays
static uint32_t s_light_theme[] = {
    [BWE_THEME_WINDOW_BG]               = 0xFFF1F5F9, // Slate 100
    [BWE_THEME_WINDOW_BORDER_ACTIVE]    = 0xFF3B82F6, // Blue 500
    [BWE_THEME_WINDOW_BORDER_INACTIVE]  = 0xFF94A3B8, // Slate 400
    [BWE_THEME_TITLEBAR_ACTIVE]         = 0xFF3B82F6,
    [BWE_THEME_TITLEBAR_INACTIVE]       = 0xFF64748B,
    [BWE_THEME_TEXT]                    = 0xFF0F172A, // Slate 900
    [BWE_THEME_CONTROL_BG]              = 0xFFFFFFFF,
    [BWE_THEME_CONTROL_BORDER]          = 0xFFCBD5E1, // Slate 300
    [BWE_THEME_SELECTION_BG]            = 0xFFBFDBFE, // Blue 200
    [BWE_THEME_SELECTION_TEXT]          = 0xFF1E3A8A, // Blue 900
    [BWE_THEME_ACCENT]                  = 0xFF2563EB  // Blue 600
};

static uint32_t s_dark_theme[] = {
    [BWE_THEME_WINDOW_BG]               = 0xFF0F172A, // Slate 900
    [BWE_THEME_WINDOW_BORDER_ACTIVE]    = 0xFF3B82F6, // Blue 500
    [BWE_THEME_WINDOW_BORDER_INACTIVE]  = 0xFF334155, // Slate 700
    [BWE_THEME_TITLEBAR_ACTIVE]         = 0xFF1E293B, // Slate 800
    [BWE_THEME_TITLEBAR_INACTIVE]       = 0xFF1E293B,
    [BWE_THEME_TEXT]                    = 0xFFF8FAFC, // Slate 50
    [BWE_THEME_CONTROL_BG]              = 0xFF1E293B,
    [BWE_THEME_CONTROL_BORDER]          = 0xFF475569, // Slate 600
    [BWE_THEME_SELECTION_BG]            = 0xFF1E40AF, // Blue 800
    [BWE_THEME_SELECTION_TEXT]          = 0xFFF8FAFC,
    [BWE_THEME_ACCENT]                  = 0xFF3B82F6  // Blue 500
};

uint32_t BWE_ThemeGetColor(BWE_ThemeToken token) {
    if (s_dark_mode) {
        return s_dark_theme[token];
    }
    return s_light_theme[token];
}

void BWE_ThemeSetDark(bool dark) {
    s_dark_mode = dark;
}
