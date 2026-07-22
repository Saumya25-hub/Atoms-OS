#ifndef BOTHEME_H
#define BOTHEME_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

// Available System Theme Presets
typedef enum {
    BOTHEME_DARK = 0,
    BOTHEME_LIGHT,
    BOTHEME_MIDNIGHT,
    BOTHEME_CLASSIC,
    BOTHEME_COUNT
} BOThemeID;

// Semantic UI Theme Tokens
typedef enum {
    // Window Chrome Tokens
    BOTHEME_TITLE_ACTIVE_TOP = 0,
    BOTHEME_TITLE_ACTIVE_BOTTOM,
    BOTHEME_TITLE_INACTIVE_TOP,
    BOTHEME_TITLE_INACTIVE_BOTTOM,
    BOTHEME_WINDOW_BORDER_ACTIVE,
    BOTHEME_WINDOW_BORDER_INACTIVE,
    BOTHEME_FRAME_BG_ACTIVE,
    BOTHEME_FRAME_BG_INACTIVE,
    BOTHEME_TITLE_TEXT_ACTIVE,
    BOTHEME_TITLE_TEXT_INACTIVE,
    BOTHEME_SHADOW_COLOR,
    BOTHEME_ACCENT_LINE,

    // Surface & Client Background Tokens
    BOTHEME_SURFACE_PRIMARY,       // Main window client background
    BOTHEME_SURFACE_SECONDARY,     // Sidebar / panel background
    BOTHEME_SURFACE_TERTIARY,      // Toolbar / header background
    BOTHEME_SURFACE_ELEVATED,      // Dialog / card background

    // Control Tokens
    BOTHEME_CONTROL_BG,
    BOTHEME_CONTROL_BORDER,
    BOTHEME_CONTROL_HOVER,
    BOTHEME_CONTROL_PRESSED,
    BOTHEME_CONTROL_DISABLED,
    BOTHEME_INPUT_BG,
    BOTHEME_INPUT_BORDER,

    // Typography Tokens
    BOTHEME_TEXT_PRIMARY,
    BOTHEME_TEXT_SECONDARY,
    BOTHEME_TEXT_DISABLED,
    BOTHEME_TEXT_INVERSE,

    // Accent & Selection Tokens
    BOTHEME_ACCENT_PRIMARY,
    BOTHEME_ACCENT_HOVER,
    BOTHEME_ACCENT_PRESSED,
    BOTHEME_SELECTION_BG,
    BOTHEME_SELECTION_TEXT,
    BOTHEME_DESKTOP_ICON_SELECT,
    BOTHEME_DESKTOP_ICON_HOVER,

    // Taskbar & Shell Tokens
    BOTHEME_TASKBAR_BG,
    BOTHEME_TASKBAR_BORDER,

    BOTHEME_TOKEN_COUNT
} BOThemeToken;

// Immutable Static Theme Palette Definition
typedef struct {
    BOThemeID   id;
    const char* name;
    bool        is_dark;
    uint32_t    colors[BOTHEME_TOKEN_COUNT];
} BOThemePalette;

// Public BOTHEME Engine API
void                  BOTHEME_Initialize(void);
bwe_error_t           BOTHEME_SetTheme(BOThemeID id);
BOThemeID             BOTHEME_GetTheme(void);
const BOThemePalette* BOTHEME_GetPalette(void);
uint32_t              BOTHEME_GetColor(BOThemeToken token);
bool                  BOTHEME_IsDark(void);
const char*           BOTHEME_GetThemeName(BOThemeID id);

#endif // BOTHEME_H
