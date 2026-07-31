#include "framework/include/bos_ui_theme.h"

static BOS_ThemeMode g_current_theme_mode = BOS_THEME_MODE_DARK;

static uint32_t g_dark_colors[] = {
    [BOS_THEME_TOKEN_WINDOW_BG]        = 0xFF1E1E1EU,
    [BOS_THEME_TOKEN_SURFACE_BG]       = 0xFF252526U,
    [BOS_THEME_TOKEN_CONTROL_BG]       = 0xFF333333U,
    [BOS_THEME_TOKEN_CONTROL_HOVER_BG] = 0xFF3E3E42U,
    [BOS_THEME_TOKEN_CONTROL_PRESS_BG] = 0xFF007ACCU,
    [BOS_THEME_TOKEN_TEXT_PRIMARY]     = 0xFFFFFFFFU,
    [BOS_THEME_TOKEN_TEXT_SECONDARY]   = 0xFFAAAAAAU,
    [BOS_THEME_TOKEN_BORDER]           = 0xFF434346U,
    [BOS_THEME_TOKEN_ACCENT]           = 0xFF007ACCU,
    [BOS_THEME_TOKEN_SELECTION]        = 0xFF094771U
};

static uint32_t g_light_colors[] = {
    [BOS_THEME_TOKEN_WINDOW_BG]        = 0xFFF3F3F3U,
    [BOS_THEME_TOKEN_SURFACE_BG]       = 0xFFFFFFFFU,
    [BOS_THEME_TOKEN_CONTROL_BG]       = 0xFFE1E1E1U,
    [BOS_THEME_TOKEN_CONTROL_HOVER_BG] = 0xFFD0D0D0U,
    [BOS_THEME_TOKEN_CONTROL_PRESS_BG] = 0xFF005FB8U,
    [BOS_THEME_TOKEN_TEXT_PRIMARY]     = 0xFF000000U,
    [BOS_THEME_TOKEN_TEXT_SECONDARY]   = 0xFF666666U,
    [BOS_THEME_TOKEN_BORDER]           = 0xFFCCCCCCU,
    [BOS_THEME_TOKEN_ACCENT]           = 0xFF005FB8U,
    [BOS_THEME_TOKEN_SELECTION]        = 0xFFA6D2FFU
};

void BOS_Theme_Init(void) {
    g_current_theme_mode = BOS_THEME_MODE_DARK;
}

void BOS_Theme_SetMode(BOS_ThemeMode mode) {
    g_current_theme_mode = mode;
}

BOS_ThemeMode BOS_Theme_GetMode(void) {
    return g_current_theme_mode;
}

uint32_t BOS_Theme_GetColor(BOS_ThemeToken token) {
    if (token > BOS_THEME_TOKEN_SELECTION) return 0xFF000000U;

    if (g_current_theme_mode == BOS_THEME_MODE_DARK) {
        return g_dark_colors[token];
    } else {
        return g_light_colors[token];
    }
}
