#include "../include/bwe.h"
#include "kernel/wm/botheme/botheme.h"

// Compatibility wrapper around BOTHEME single source of truth
uint32_t BWE_ThemeGetColor(BWE_ThemeToken token) {
    switch (token) {
        case BWE_THEME_WINDOW_BG:              return BOTHEME_GetColor(BOTHEME_SURFACE_PRIMARY);
        case BWE_THEME_WINDOW_BORDER_ACTIVE:   return BOTHEME_GetColor(BOTHEME_WINDOW_BORDER_ACTIVE);
        case BWE_THEME_WINDOW_BORDER_INACTIVE: return BOTHEME_GetColor(BOTHEME_WINDOW_BORDER_INACTIVE);
        case BWE_THEME_TITLEBAR_ACTIVE:        return BOTHEME_GetColor(BOTHEME_TITLE_ACTIVE_TOP);
        case BWE_THEME_TITLEBAR_INACTIVE:      return BOTHEME_GetColor(BOTHEME_TITLE_INACTIVE_TOP);
        case BWE_THEME_TEXT:                   return BOTHEME_GetColor(BOTHEME_TEXT_PRIMARY);
        case BWE_THEME_CONTROL_BG:             return BOTHEME_GetColor(BOTHEME_CONTROL_BG);
        case BWE_THEME_CONTROL_BORDER:         return BOTHEME_GetColor(BOTHEME_CONTROL_BORDER);
        case BWE_THEME_SELECTION_BG:           return BOTHEME_GetColor(BOTHEME_SELECTION_BG);
        case BWE_THEME_SELECTION_TEXT:         return BOTHEME_GetColor(BOTHEME_SELECTION_TEXT);
        case BWE_THEME_ACCENT:                 return BOTHEME_GetColor(BOTHEME_ACCENT_PRIMARY);
        default: return BOTHEME_GetColor(BOTHEME_TEXT_PRIMARY);
    }
}

void BWE_ThemeSetDark(bool dark) {
    BOTHEME_SetTheme(dark ? BOTHEME_DARK : BOTHEME_LIGHT);
}
