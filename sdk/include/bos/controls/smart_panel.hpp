#ifndef BOS_UI_CONTROLS_SMART_PANEL_HPP
#define BOS_UI_CONTROLS_SMART_PANEL_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"
#include "bos/font.hpp"

namespace bos {

// ============================================================================
// Smart Panel Sizing Modes
// ============================================================================
enum class SizingMode : uint32_t {
    Auto       = 0, // Natural fit: Width and Height fit content + padding
    AutoSize   = 0, // Synonym for Auto
    AutoWidth  = 1, // Width fits content; height governed by parent or fixed
    AutoHeight = 2, // Height fits content; width fills parent or fixed
    Fill       = 3, // Consumes available parent space
    Stretch    = 3, // Synonym for Fill
    Fixed      = 4  // Adheres to explicit bounds set by developer
};

// ============================================================================
// SmartPanel — High-Level Auto-Framing Container
// ============================================================================
class SmartPanel : public Widget {
public:
    SmartPanel(Orientation orientation = Orientation::Vertical,
               SizingMode sizing = SizingMode::Auto);
    ~SmartPanel() override = default;

    // Fluent Configuration
    SmartPanel& add(Widget* child);
    SmartPanel& add(Widget& child) { return add(&child); }
    bool remove(Widget* child) { return remove_child(child); }

    void set_orientation(Orientation o);
    Orientation orientation() const { return m_orientation; }

    void set_sizing_mode(SizingMode mode);
    SizingMode sizing_mode() const { return m_sizing_mode; }

    void set_alignment(Alignment align);
    Alignment alignment() const { return m_alignment; }

    void set_spacing(int32_t spacing);
    int32_t spacing() const { return m_spacing; }

    // Optional Card Styling & Header
    void set_card_style(bool enable,
                        Color bg = Theme::SurfaceCard(),
                        Color border = Theme::Border(),
                        uint32_t radius = Theme::RadiusCard());
    bool is_card_style() const { return m_card_style; }

    void set_title(const char* title, const char* subtitle = nullptr);
    const char* title() const { return m_title; }
    const char* subtitle() const { return m_subtitle; }

    // Widget Overrides
    void paint(Surface& surface) override;
    Size measure_preferred_size() const override;
    void perform_layout();

private:
    Orientation m_orientation{Orientation::Vertical};
    SizingMode  m_sizing_mode{SizingMode::Auto};
    Alignment   m_alignment{Alignment::Start}; // Small controls stay small by default
    int32_t     m_spacing{8};                  // Default Theme::PanelSpacing()

    bool        m_card_style{false};
    Color       m_bg_color{Theme::SurfaceCard()};
    Color       m_border_color{Theme::Border()};
    uint32_t    m_corner_radius{Theme::RadiusCard()};

    char        m_title[64]{0};
    char        m_subtitle[64]{0};
    bool        m_in_layout{false};
};

// ============================================================================
// Convenience Primitives
// ============================================================================

// VBox: Vertical Smart Container
class VBox : public SmartPanel {
public:
    explicit VBox(SizingMode sizing = SizingMode::Auto)
        : SmartPanel(Orientation::Vertical, sizing) {}
};

// HBox: Horizontal Smart Container
class HBox : public SmartPanel {
public:
    explicit HBox(SizingMode sizing = SizingMode::Auto)
        : SmartPanel(Orientation::Horizontal, sizing) {}
};

// ButtonGroup: Pre-configured horizontal button cluster
class ButtonGroup : public SmartPanel {
public:
    ButtonGroup() : SmartPanel(Orientation::Horizontal, SizingMode::Auto) {
        set_spacing(6);
        set_padding(Insets(0, 0, 0, 0));
        set_alignment(Alignment::Center);
    }
};

} // namespace bos

#endif // BOS_UI_CONTROLS_SMART_PANEL_HPP
