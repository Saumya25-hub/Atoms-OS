#ifndef BOS_UI_THEME_H
#define BOS_UI_THEME_H

#include "platform/include/bos_types.h"

typedef enum {
    BOS_THEME_TOKEN_WINDOW_BG = 0,
    BOS_THEME_TOKEN_SURFACE_BG,
    BOS_THEME_TOKEN_CONTROL_BG,
    BOS_THEME_TOKEN_CONTROL_HOVER_BG,
    BOS_THEME_TOKEN_CONTROL_PRESS_BG,
    BOS_THEME_TOKEN_TEXT_PRIMARY,
    BOS_THEME_TOKEN_TEXT_SECONDARY,
    BOS_THEME_TOKEN_BORDER,
    BOS_THEME_TOKEN_ACCENT,
    BOS_THEME_TOKEN_SELECTION
} BOS_ThemeToken;

typedef enum {
    BOS_THEME_MODE_DARK = 0,
    BOS_THEME_MODE_LIGHT
} BOS_ThemeMode;

void          BOS_Theme_Init(void);
void          BOS_Theme_SetMode(BOS_ThemeMode mode);
BOS_ThemeMode BOS_Theme_GetMode(void);
uint32_t      BOS_Theme_GetColor(BOS_ThemeToken token);

#endif /* BOS_UI_THEME_H */
