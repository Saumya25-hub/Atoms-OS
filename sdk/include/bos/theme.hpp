#ifndef BOS_UI_THEME_HPP
#define BOS_UI_THEME_HPP

#include <stdint.h>
#include "types.hpp"
#include "state.hpp"

namespace bos {

// ============================================================================
// Theme Mode Selection
// ============================================================================
enum class ThemeMode : uint32_t {
    Dark = 0,
    Light = 1
};

// ============================================================================
// ATOMS OS Modern Design System & Theme Foundation (Phase 4 Dynamic Palette)
// ============================================================================
class Theme {
public:
    static ThemeMode mode();
    static void set_mode(ThemeMode m);
    static void toggle_mode();

    // Color & Surface Tokens (Mode-dependent dynamic tokens)
    static Color Background();
    static Color Surface();
    static Color SurfaceCard();
    static Color SurfaceElevated();
    static Color SurfaceSubtle();
    static Color Divider();
    static Color Border();
    static Color BorderHighlight();
    static Color TextPrimary();
    static Color TextSecondary();
    static Color TextMuted();
    static Color Accent();
    static Color AccentHover();
    static Color AccentActive();
    static Color Success();
    static Color Warning();
    static Color Danger();
    static Color FocusRing();

    // Corner Radii Scale
    static constexpr uint32_t RadiusSmall()   { return 4; }
    static constexpr uint32_t RadiusControl() { return 6; }
    static constexpr uint32_t RadiusMedium()  { return 6; }
    static constexpr uint32_t RadiusCard()    { return 10; }
    static constexpr uint32_t RadiusLarge()   { return 10; }
    static constexpr uint32_t RadiusWindow()  { return 10; }
    static constexpr uint32_t RadiusPill()    { return 16; }

    // Metrics Scale
    static constexpr uint32_t ControlHeight()    { return 32; }
    static constexpr uint32_t ButtonHeight()     { return 32; }
    static constexpr uint32_t InputHeight()      { return 32; }
    static constexpr uint32_t HeaderHeight()     { return 44; }
    static constexpr uint32_t PanelPadding()     { return 12; }
    static constexpr uint32_t PanelSpacing()     { return 8; }
    static constexpr uint32_t FocusRingWidth()   { return 2; }
    static constexpr uint32_t FocusRingOffset()  { return 2; }
    static constexpr uint32_t Spacing2()         { return 2; }
    static constexpr uint32_t Spacing4()         { return 4; }
    static constexpr uint32_t SpacingSmall()     { return 6; }
    static constexpr uint32_t SpacingMedium()    { return 12; }
    static constexpr uint32_t SpacingLarge()     { return 20; }
    static constexpr uint32_t SpacingXLarge()    { return 28; }

    // Pre-Configured Modern Control Styles
    static ControlStyle make_button_style();
    static ControlStyle make_primary_button_style();
    static ControlStyle make_secondary_button_style();
    static ControlStyle make_ghost_button_style();
    static ControlStyle make_accent_button_style();
    static ControlStyle make_card_style();
    static ControlStyle make_textbox_style();
};

} // namespace bos

#endif // BOS_UI_THEME_HPP
