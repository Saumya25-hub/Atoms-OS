#include "bos/theme.hpp"

namespace bos {

static ThemeMode s_current_theme_mode = ThemeMode::Dark;

ThemeMode Theme::mode() {
    return s_current_theme_mode;
}

void Theme::set_mode(ThemeMode m) {
    s_current_theme_mode = m;
}

void Theme::toggle_mode() {
    s_current_theme_mode = (s_current_theme_mode == ThemeMode::Dark) ? ThemeMode::Light : ThemeMode::Dark;
}

Color Theme::Background() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF0F172A) : Color(0xFFF1F5F9);
}

Color Theme::Surface() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF162032) : Color(0xFFFFFFFF);
}

Color Theme::SurfaceCard() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF1E293B) : Color(0xFFFFFFFF);
}

Color Theme::SurfaceElevated() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF243247) : Color(0xFFFFFFFF);
}

Color Theme::SurfaceSubtle() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF1E293B) : Color(0xFFF1F5F9);
}

Color Theme::Divider() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF334155) : Color(0xFFE2E8F0);
}

Color Theme::Border() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF2E3D52) : Color(0xFFCBD5E1);
}

Color Theme::BorderHighlight() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF475569) : Color(0xFF94A3B8);
}

Color Theme::TextPrimary() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFFF8FAFC) : Color(0xFF0F172A);
}

Color Theme::TextSecondary() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF94A3B8) : Color(0xFF475569);
}

Color Theme::TextMuted() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF64748B) : Color(0xFF94A3B8);
}

Color Theme::Accent() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF2563EB) : Color(0xFF0058EE);
}

Color Theme::AccentHover() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF3B82F6) : Color(0xFF2563EB);
}

Color Theme::AccentActive() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF1D4ED8) : Color(0xFF1D4ED8);
}

Color Theme::Success() {
    return Color(0xFF10B981);
}

Color Theme::Warning() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFFF59E0B) : Color(0xFFD97706);
}

Color Theme::Danger() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFFEF4444) : Color(0xFFDC2626);
}

Color Theme::FocusRing() {
    return (s_current_theme_mode == ThemeMode::Dark) ? Color(0xFF38BDF8) : Color(0xFF0058EE);
}

ControlStyle Theme::make_button_style() {
    return make_secondary_button_style();
}

ControlStyle Theme::make_primary_button_style() {
    ControlStyle style;
    style.set_style(VisualState::Normal,   StateStyle(Accent(), AccentHover(), Color::White(), RadiusControl(), 1));
    style.set_style(VisualState::Hover,    StateStyle(AccentHover(), Color::White(), Color::White(), RadiusControl(), 1));
    style.set_style(VisualState::Pressed,  StateStyle(AccentActive(), AccentActive(), Color::White(), RadiusControl(), 1));
    style.set_style(VisualState::Focused,  StateStyle(Accent(), FocusRing(), Color::White(), RadiusControl(), 2));
    style.set_style(VisualState::Disabled, StateStyle(SurfaceSubtle(), Border(), TextMuted(), RadiusControl(), 1));
    style.set_style(VisualState::Selected, StateStyle(AccentActive(), Color::White(), Color::White(), RadiusControl(), 1));
    return style;
}

ControlStyle Theme::make_secondary_button_style() {
    ControlStyle style;
    if (s_current_theme_mode == ThemeMode::Dark) {
        style.set_style(VisualState::Normal,   StateStyle(SurfaceSubtle(), Border(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Hover,    StateStyle(Color(0xFF2A374D), AccentHover(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Pressed,  StateStyle(AccentActive(), Accent(), Color::White(), RadiusControl(), 1));
        style.set_style(VisualState::Focused,  StateStyle(SurfaceSubtle(), FocusRing(), TextPrimary(), RadiusControl(), 2));
        style.set_style(VisualState::Disabled, StateStyle(Color(0xFF111827), Color(0xFF1F2937), TextMuted(), RadiusControl(), 1));
        style.set_style(VisualState::Selected, StateStyle(Accent(), AccentHover(), Color::White(), RadiusControl(), 1));
    } else {
        style.set_style(VisualState::Normal,   StateStyle(Color(0xFFFFFFFF), Border(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Hover,    StateStyle(SurfaceSubtle(), AccentHover(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Pressed,  StateStyle(Color(0xFFCBD5E1), Accent(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Focused,  StateStyle(Color(0xFFFFFFFF), FocusRing(), TextPrimary(), RadiusControl(), 2));
        style.set_style(VisualState::Disabled, StateStyle(Color(0xFFF8FAFC), Color(0xFFE2E8F0), TextMuted(), RadiusControl(), 1));
        style.set_style(VisualState::Selected, StateStyle(Accent(), AccentHover(), Color::White(), RadiusControl(), 1));
    }
    return style;
}

ControlStyle Theme::make_ghost_button_style() {
    ControlStyle style;
    style.set_style(VisualState::Normal,   StateStyle(Color::Transparent(), Color::Transparent(), TextPrimary(), RadiusControl(), 0));
    style.set_style(VisualState::Hover,    StateStyle(SurfaceSubtle(), Border(), TextPrimary(), RadiusControl(), 1));
    style.set_style(VisualState::Pressed,  StateStyle(SurfaceCard(), Accent(), TextPrimary(), RadiusControl(), 1));
    style.set_style(VisualState::Focused,  StateStyle(Color::Transparent(), FocusRing(), TextPrimary(), RadiusControl(), 2));
    style.set_style(VisualState::Disabled, StateStyle(Color::Transparent(), Color::Transparent(), TextMuted(), RadiusControl(), 0));
    return style;
}

ControlStyle Theme::make_accent_button_style() {
    return make_primary_button_style();
}

ControlStyle Theme::make_card_style() {
    ControlStyle style;
    style.set_style(VisualState::Normal,   StateStyle(SurfaceCard(), Border(), TextPrimary(), RadiusCard(), 1));
    style.set_style(VisualState::Hover,    StateStyle(SurfaceCard(), BorderHighlight(), TextPrimary(), RadiusCard(), 1));
    style.set_style(VisualState::Focused,  StateStyle(SurfaceCard(), FocusRing(), TextPrimary(), RadiusCard(), 2));
    return style;
}

ControlStyle Theme::make_textbox_style() {
    ControlStyle style;
    if (s_current_theme_mode == ThemeMode::Dark) {
        style.set_style(VisualState::Normal,   StateStyle(SurfaceSubtle(), Border(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Hover,    StateStyle(SurfaceSubtle(), BorderHighlight(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Focused,  StateStyle(SurfaceSubtle(), FocusRing(), TextPrimary(), RadiusControl(), 2));
        style.set_style(VisualState::Disabled, StateStyle(Color(0xFF111827), Color(0xFF1F2937), TextMuted(), RadiusControl(), 1));
    } else {
        style.set_style(VisualState::Normal,   StateStyle(Color(0xFFFFFFFF), Border(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Hover,    StateStyle(Color(0xFFFFFFFF), BorderHighlight(), TextPrimary(), RadiusControl(), 1));
        style.set_style(VisualState::Focused,  StateStyle(Color(0xFFFFFFFF), FocusRing(), TextPrimary(), RadiusControl(), 2));
        style.set_style(VisualState::Disabled, StateStyle(Color(0xFFF8FAFC), Color(0xFFE2E8F0), TextMuted(), RadiusControl(), 1));
    }
    return style;
}

} // namespace bos
