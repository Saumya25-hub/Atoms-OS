#include "kernel/wm/botheme/botheme.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static BOThemeID              s_current_theme_id = BOTHEME_DARK;
static const BOThemePalette*  s_active_palette = 0;

// 1. DARK PRESET (Clean neutral dark slate theme)
static const BOThemePalette g_palette_dark = {
    .id = BOTHEME_DARK,
    .name = "Dark Slate",
    .is_dark = true,
    .colors = {
        [BOTHEME_TITLE_ACTIVE_TOP]      = 0xFF1E293B, // Slate-800
        [BOTHEME_TITLE_ACTIVE_BOTTOM]   = 0xFF0F172A, // Slate-900
        [BOTHEME_TITLE_INACTIVE_TOP]    = 0xFF334155, // Slate-700
        [BOTHEME_TITLE_INACTIVE_BOTTOM] = 0xFF1E293B, // Slate-800
        [BOTHEME_WINDOW_BORDER_ACTIVE]  = 0xFF3B82F6, // Vibrant Blue
        [BOTHEME_WINDOW_BORDER_INACTIVE]= 0xFF475569, // Slate-600
        [BOTHEME_FRAME_BG_ACTIVE]       = 0xFF0F172A,
        [BOTHEME_FRAME_BG_INACTIVE]     = 0xFF1E293B,
        [BOTHEME_TITLE_TEXT_ACTIVE]     = 0xFFF1F5F9, // Slate-100 White
        [BOTHEME_TITLE_TEXT_INACTIVE]   = 0xFF94A3B8, // Muted Slate Text
        [BOTHEME_SHADOW_COLOR]          = 0x2E000000,
        [BOTHEME_ACCENT_LINE]           = 0xFF3B82F6,

        [BOTHEME_SURFACE_PRIMARY]       = 0xFF0F172A, // Slate-900 Main Client
        [BOTHEME_SURFACE_SECONDARY]     = 0xFF1E293B, // Sidebar Background
        [BOTHEME_SURFACE_TERTIARY]      = 0xFF334155, // Header Toolbar
        [BOTHEME_SURFACE_ELEVATED]      = 0xFF1E293B,

        [BOTHEME_CONTROL_BG]            = 0xFF1E293B,
        [BOTHEME_CONTROL_BORDER]        = 0xFF475569,
        [BOTHEME_CONTROL_HOVER]         = 0xFF334155,
        [BOTHEME_CONTROL_PRESSED]       = 0xFF0F172A,
        [BOTHEME_CONTROL_DISABLED]      = 0xFF334155,
        [BOTHEME_INPUT_BG]              = 0xFF0F172A,
        [BOTHEME_INPUT_BORDER]          = 0xFF475569,

        [BOTHEME_TEXT_PRIMARY]          = 0xFFF8FAFC, // Slate-50 White
        [BOTHEME_TEXT_SECONDARY]        = 0xFF94A3B8, // Muted Gray
        [BOTHEME_TEXT_DISABLED]         = 0xFF64748B,
        [BOTHEME_TEXT_INVERSE]          = 0xFF0F172A,

        [BOTHEME_ACCENT_PRIMARY]        = 0xFF3B82F6, // Blue-500
        [BOTHEME_ACCENT_HOVER]          = 0xFF60A5FA,
        [BOTHEME_ACCENT_PRESSED]        = 0xFF1D4ED8,
        [BOTHEME_SELECTION_BG]          = 0xFF1E40AF, // Blue-800
        [BOTHEME_SELECTION_TEXT]        = 0xFFF8FAFC,
        [BOTHEME_DESKTOP_ICON_SELECT]   = 0x3D3B82F6,
        [BOTHEME_DESKTOP_ICON_HOVER]    = 0x1AFFFFFF,

        [BOTHEME_TASKBAR_BG]            = 0xFF1E293B,
        [BOTHEME_TASKBAR_BORDER]        = 0xFF334155,
    }
};

// 2. LIGHT PRESET (Coherent genuine light desktop theme)
static const BOThemePalette g_palette_light = {
    .id = BOTHEME_LIGHT,
    .name = "Light Studio",
    .is_dark = false,
    .colors = {
        [BOTHEME_TITLE_ACTIVE_TOP]      = 0xFFE2E8F0, // Slate-200 Light
        [BOTHEME_TITLE_ACTIVE_BOTTOM]   = 0xFFCBD5E1, // Slate-300
        [BOTHEME_TITLE_INACTIVE_TOP]    = 0xFFF1F5F9, // Slate-100
        [BOTHEME_TITLE_INACTIVE_BOTTOM] = 0xFFE2E8F0,
        [BOTHEME_WINDOW_BORDER_ACTIVE]  = 0xFF2563EB, // Deep Blue Accent
        [BOTHEME_WINDOW_BORDER_INACTIVE]= 0xFF94A3B8, // Slate-400
        [BOTHEME_FRAME_BG_ACTIVE]       = 0xFFE2E8F0,
        [BOTHEME_FRAME_BG_INACTIVE]     = 0xFFF1F5F9,
        [BOTHEME_TITLE_TEXT_ACTIVE]     = 0xFF0F172A, // Slate-900 Dark Text
        [BOTHEME_TITLE_TEXT_INACTIVE]   = 0xFF64748B, // Muted Text
        [BOTHEME_SHADOW_COLOR]          = 0x1F000000,
        [BOTHEME_ACCENT_LINE]           = 0xFF2563EB,

        [BOTHEME_SURFACE_PRIMARY]       = 0xFFF8FAFC, // Crisp Light Surface
        [BOTHEME_SURFACE_SECONDARY]     = 0xFFF1F5F9, // Off-white sidebar
        [BOTHEME_SURFACE_TERTIARY]      = 0xFFE2E8F0, // Light toolbar
        [BOTHEME_SURFACE_ELEVATED]      = 0xFFFFFFFF,

        [BOTHEME_CONTROL_BG]            = 0xFFFFFFFF, // Pure white inputs/buttons
        [BOTHEME_CONTROL_BORDER]        = 0xFFCBD5E1, // Slate-300 border
        [BOTHEME_CONTROL_HOVER]         = 0xFFF1F5F9,
        [BOTHEME_CONTROL_PRESSED]       = 0xFFE2E8F0,
        [BOTHEME_CONTROL_DISABLED]      = 0xFFE2E8F0,
        [BOTHEME_INPUT_BG]              = 0xFFFFFFFF,
        [BOTHEME_INPUT_BORDER]          = 0xFF94A3B8,

        [BOTHEME_TEXT_PRIMARY]          = 0xFF0F172A, // Dark text for readability
        [BOTHEME_TEXT_SECONDARY]        = 0xFF475569,
        [BOTHEME_TEXT_DISABLED]         = 0xFF94A3B8,
        [BOTHEME_TEXT_INVERSE]          = 0xFFF8FAFC,

        [BOTHEME_ACCENT_PRIMARY]        = 0xFF2563EB, // Blue-600
        [BOTHEME_ACCENT_HOVER]          = 0xFF3B82F6,
        [BOTHEME_ACCENT_PRESSED]        = 0xFF1D4ED8,
        [BOTHEME_SELECTION_BG]          = 0xFFBFDBFE, // Soft Light Blue
        [BOTHEME_SELECTION_TEXT]        = 0xFF1E3A8A, // Deep Navy Text
        [BOTHEME_DESKTOP_ICON_SELECT]   = 0x3D2563EB,
        [BOTHEME_DESKTOP_ICON_HOVER]    = 0x1A000000,

        [BOTHEME_TASKBAR_BG]            = 0xFFE2E8F0,
        [BOTHEME_TASKBAR_BORDER]        = 0xFFCBD5E1,
    }
};

// 3. MIDNIGHT PRESET (Deep navy near-black theme)
static const BOThemePalette g_palette_midnight = {
    .id = BOTHEME_MIDNIGHT,
    .name = "Midnight Navy",
    .is_dark = true,
    .colors = {
        [BOTHEME_TITLE_ACTIVE_TOP]      = 0xFF0F172A, // Deep Navy
        [BOTHEME_TITLE_ACTIVE_BOTTOM]   = 0xFF050B14, // Near Black Navy
        [BOTHEME_TITLE_INACTIVE_TOP]    = 0xFF1E293B,
        [BOTHEME_TITLE_INACTIVE_BOTTOM] = 0xFF0F172A,
        [BOTHEME_WINDOW_BORDER_ACTIVE]  = 0xFF06B6D4, // Cyan-500 Neon Accent
        [BOTHEME_WINDOW_BORDER_INACTIVE]= 0xFF1E293B,
        [BOTHEME_FRAME_BG_ACTIVE]       = 0xFF050B14,
        [BOTHEME_FRAME_BG_INACTIVE]     = 0xFF0F172A,
        [BOTHEME_TITLE_TEXT_ACTIVE]     = 0xFFE0F2FE, // Ice Cyan White
        [BOTHEME_TITLE_TEXT_INACTIVE]   = 0xFF64748B,
        [BOTHEME_SHADOW_COLOR]          = 0x3A000000,
        [BOTHEME_ACCENT_LINE]           = 0xFF06B6D4,

        [BOTHEME_SURFACE_PRIMARY]       = 0xFF050B14, // Deep Midnight Canvas
        [BOTHEME_SURFACE_SECONDARY]     = 0xFF0F172A,
        [BOTHEME_SURFACE_TERTIARY]      = 0xFF1E293B,
        [BOTHEME_SURFACE_ELEVATED]      = 0xFF0F172A,

        [BOTHEME_CONTROL_BG]            = 0xFF0F172A,
        [BOTHEME_CONTROL_BORDER]        = 0xFF1E293B,
        [BOTHEME_CONTROL_HOVER]         = 0xFF1E293B,
        [BOTHEME_CONTROL_PRESSED]       = 0xFF050B14,
        [BOTHEME_CONTROL_DISABLED]      = 0xFF1E293B,
        [BOTHEME_INPUT_BG]              = 0xFF050B14,
        [BOTHEME_INPUT_BORDER]          = 0xFF06B6D4,

        [BOTHEME_TEXT_PRIMARY]          = 0xFFF0F9FF,
        [BOTHEME_TEXT_SECONDARY]        = 0xFF7DD3FC, // Cyan Subtext
        [BOTHEME_TEXT_DISABLED]         = 0xFF475569,
        [BOTHEME_TEXT_INVERSE]          = 0xFF050B14,

        [BOTHEME_ACCENT_PRIMARY]        = 0xFF06B6D4, // Cyan-500
        [BOTHEME_ACCENT_HOVER]          = 0xFF22D3EE,
        [BOTHEME_ACCENT_PRESSED]        = 0xFF0891B2,
        [BOTHEME_SELECTION_BG]          = 0xFF164E63, // Dark Cyan Selection
        [BOTHEME_SELECTION_TEXT]        = 0xFFECFEFF,
        [BOTHEME_DESKTOP_ICON_SELECT]   = 0x3D06B6D4,
        [BOTHEME_DESKTOP_ICON_HOVER]    = 0x1A06B6D4,

        [BOTHEME_TASKBAR_BG]            = 0xFF0A1120,
        [BOTHEME_TASKBAR_BORDER]        = 0xFF1E293B,
    }
};

// 4. CLASSIC PRESET (Restrained traditional silver/workstation desktop theme)
static const BOThemePalette g_palette_classic = {
    .id = BOTHEME_CLASSIC,
    .name = "Classic Workstation",
    .is_dark = false,
    .colors = {
        [BOTHEME_TITLE_ACTIVE_TOP]      = 0xFFCBD5E1, // Silver-Slate Top
        [BOTHEME_TITLE_ACTIVE_BOTTOM]   = 0xFF94A3B8, // Metallic Gray Bottom
        [BOTHEME_TITLE_INACTIVE_TOP]    = 0xFFE2E8F0,
        [BOTHEME_TITLE_INACTIVE_BOTTOM] = 0xFFCBD5E1,
        [BOTHEME_WINDOW_BORDER_ACTIVE]  = 0xFF1D4ED8, // Classic Deep Blue Edge
        [BOTHEME_WINDOW_BORDER_INACTIVE]= 0xFF64748B,
        [BOTHEME_FRAME_BG_ACTIVE]       = 0xFFCBD5E1,
        [BOTHEME_FRAME_BG_INACTIVE]     = 0xFFE2E8F0,
        [BOTHEME_TITLE_TEXT_ACTIVE]     = 0xFF0F172A,
        [BOTHEME_TITLE_TEXT_INACTIVE]   = 0xFF475569,
        [BOTHEME_SHADOW_COLOR]          = 0x22000000,
        [BOTHEME_ACCENT_LINE]           = 0xFF1D4ED8,

        [BOTHEME_SURFACE_PRIMARY]       = 0xFFE2E8F0, // Neutral Workstation Gray
        [BOTHEME_SURFACE_SECONDARY]     = 0xFFCBD5E1,
        [BOTHEME_SURFACE_TERTIARY]      = 0xFF94A3B8,
        [BOTHEME_SURFACE_ELEVATED]      = 0xFFF1F5F9,

        [BOTHEME_CONTROL_BG]            = 0xFFF1F5F9,
        [BOTHEME_CONTROL_BORDER]        = 0xFF64748B,
        [BOTHEME_CONTROL_HOVER]         = 0xFFE2E8F0,
        [BOTHEME_CONTROL_PRESSED]       = 0xFFCBD5E1,
        [BOTHEME_CONTROL_DISABLED]      = 0xFFCBD5E1,
        [BOTHEME_INPUT_BG]              = 0xFFFFFFFF,
        [BOTHEME_INPUT_BORDER]          = 0xFF475569,

        [BOTHEME_TEXT_PRIMARY]          = 0xFF0F172A,
        [BOTHEME_TEXT_SECONDARY]        = 0xFF334155,
        [BOTHEME_TEXT_DISABLED]         = 0xFF64748B,
        [BOTHEME_TEXT_INVERSE]          = 0xFFFFFFFF,

        [BOTHEME_ACCENT_PRIMARY]        = 0xFF1D4ED8, // Blue-700
        [BOTHEME_ACCENT_HOVER]          = 0xFF2563EB,
        [BOTHEME_ACCENT_PRESSED]        = 0xFF1E40AF,
        [BOTHEME_SELECTION_BG]          = 0xFF93C5FD,
        [BOTHEME_SELECTION_TEXT]        = 0xFF1E3A8A,
        [BOTHEME_DESKTOP_ICON_SELECT]   = 0x3D1D4ED8,
        [BOTHEME_DESKTOP_ICON_HOVER]    = 0x1A000000,

        [BOTHEME_TASKBAR_BG]            = 0xFFCBD5E1,
        [BOTHEME_TASKBAR_BORDER]        = 0xFF64748B,
    }
};

static const BOThemePalette* g_all_palettes[BOTHEME_COUNT] = {
    &g_palette_dark,
    &g_palette_light,
    &g_palette_midnight,
    &g_palette_classic
};

void BOTHEME_Initialize(void) {
    s_current_theme_id = BOTHEME_DARK;
    s_active_palette = &g_palette_dark;
    
    display_print("[BOTHEME] Subsystem Initialized: Default Preset [Dark Slate]\n");
}

bwe_error_t BOTHEME_SetTheme(BOThemeID id) {
    if (id >= BOTHEME_COUNT) {
        display_print("[BOTHEME_ERROR] Invalid Theme ID requested\n");
        return BWE0001;
    }

    if (s_current_theme_id == id && s_active_palette != 0) {
        return BWE_SUCCESS; // Already active, no redundant work
    }

    s_current_theme_id = id;
    s_active_palette = g_all_palettes[id];

    display_print("[BOTHEME] Theme Changed: ");
    display_print(s_active_palette->name);
    display_print("\n");

    // Invalidate all active windows to trigger live repaint pass
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_InvalidateWindow(i);
    }
    BWE_InvalidateWindow(BWE_DESKTOP_ID);

    return BWE_SUCCESS;
}

BOThemeID BOTHEME_GetTheme(void) {
    return s_current_theme_id;
}

const BOThemePalette* BOTHEME_GetPalette(void) {
    if (!s_active_palette) s_active_palette = &g_palette_dark;
    return s_active_palette;
}

uint32_t BOTHEME_GetColor(BOThemeToken token) {
    if (token >= BOTHEME_TOKEN_COUNT) return 0xFF000000;
    if (!s_active_palette) s_active_palette = &g_palette_dark;
    return s_active_palette->colors[token];
}

bool BOTHEME_IsDark(void) {
    if (!s_active_palette) s_active_palette = &g_palette_dark;
    return s_active_palette->is_dark;
}

const char* BOTHEME_GetThemeName(BOThemeID id) {
    if (id >= BOTHEME_COUNT) return "Unknown";
    return g_all_palettes[id]->name;
}
