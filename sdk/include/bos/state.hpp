#ifndef BOS_UI_STATE_HPP
#define BOS_UI_STATE_HPP

#include <stdint.h>
#include "types.hpp"
#include "geometry.hpp"
#include "resource.hpp"

namespace bos {

// ============================================================================
// Universal Visual State Enumeration
// ============================================================================
enum class VisualState : uint32_t {
    Normal = 0,
    Hover,
    Pressed,
    Focused,
    Disabled,
    Selected
};

// ============================================================================
// Single State Visual Style Definition
// ============================================================================
struct StateStyle {
    Color    background{Color::Slate800()};
    Color    border{Color::Slate700()};
    Color    foreground{Color::Slate100()};
    uint32_t corner_radius{6};
    uint32_t border_thickness{1};
    Image    image_asset{};
    Insets   slice_borders{0};

    constexpr StateStyle() = default;

    StateStyle(Color bg, Color bdr, Color fg, uint32_t radius = 6, uint32_t thick = 1)
        : background(bg), border(bdr), foreground(fg), corner_radius(radius), border_thickness(thick) {}

    StateStyle(const Image& img, const Insets& borders, Color fg = Color::White())
        : foreground(fg), image_asset(img), slice_borders(borders) {}

    bool has_image() const { return image_asset.is_valid(); }
};

// ============================================================================
// Multi-State Control Style Resolver with Fallback
// ============================================================================
class ControlStyle {
public:
    ControlStyle() {
        // Sensible default styles for standard modern controls
        m_styles[(size_t)VisualState::Normal] = StateStyle(Color::Slate800(), Color::Slate700(), Color::Slate100(), 6, 1);
        m_styles[(size_t)VisualState::Hover] = StateStyle(Color::Slate700(), Color::Blue500(), Color::White(), 6, 1);
        m_styles[(size_t)VisualState::Pressed] = StateStyle(Color::Blue600(), Color::Blue500(), Color::White(), 6, 1);
        m_styles[(size_t)VisualState::Focused] = StateStyle(Color::Slate800(), Color::Blue500(), Color::White(), 6, 2);
        m_styles[(size_t)VisualState::Disabled] = StateStyle(Color(0xFF1E293B), Color(0xFF334155), Color::Slate600(), 6, 1);
        m_styles[(size_t)VisualState::Selected] = StateStyle(Color::Blue600(), Color::Blue500(), Color::White(), 6, 1);
        m_is_set[(size_t)VisualState::Normal] = true;
    }

    void set_style(VisualState state, const StateStyle& style) {
        m_styles[(size_t)state] = style;
        m_is_set[(size_t)state] = true;
    }

    const StateStyle& resolve(VisualState state) const {
        size_t idx = (size_t)state;
        if (m_is_set[idx]) {
            return m_styles[idx];
        }

        // Deterministic Fallback Rules:
        // Selected -> Pressed -> Hover -> Normal
        // Pressed  -> Hover -> Normal
        // Hover    -> Normal
        // Focused  -> Normal
        // Disabled -> Normal
        if (state == VisualState::Selected && m_is_set[(size_t)VisualState::Pressed]) {
            return m_styles[(size_t)VisualState::Pressed];
        }
        if ((state == VisualState::Selected || state == VisualState::Pressed) && m_is_set[(size_t)VisualState::Hover]) {
            return m_styles[(size_t)VisualState::Hover];
        }

        return m_styles[(size_t)VisualState::Normal];
    }

private:
    StateStyle m_styles[6];
    bool       m_is_set[6]{false};
};

} // namespace bos

#endif // BOS_UI_STATE_HPP
