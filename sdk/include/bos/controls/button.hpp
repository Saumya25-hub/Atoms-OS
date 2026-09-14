#ifndef BOS_UI_CONTROLS_BUTTON_HPP
#define BOS_UI_CONTROLS_BUTTON_HPP

#include "bos/widget.hpp"
#include "bos/state.hpp"
#include "bos/icon.hpp"
#include "bos/theme.hpp"

namespace bos {

enum class ButtonVariant : uint8_t {
    Primary   = 0,
    Secondary = 1,
    Ghost     = 2
};

class Button : public Widget {
public:
    using ClickCallback = void (*)(Button* sender, void* user_data);

    Button();
    explicit Button(const char* text);
    Button(const char* text, const Icon& icon);
    Button(const char* text, ButtonVariant variant);
    ~Button() override = default;

    ButtonVariant variant() const { return m_variant; }
    void set_variant(ButtonVariant v);

    const char* text() const { return m_text; }
    void set_text(const char* text);

    const Icon& icon() const { return m_icon; }
    void set_icon(const Icon& icon);
    void set_icon_position(IconPosition pos) { m_icon_pos = pos; invalidate(); }

    bool is_selected() const { return m_selected; }
    void set_selected(bool selected);

    void set_style(const ControlStyle& style) { m_style = style; invalidate(); }
    ControlStyle& style() { return m_style; }

    void set_on_click(ClickCallback callback, void* user_data = nullptr) {
        m_on_click = callback;
        m_user_data = user_data;
    }

    VisualState current_visual_state() const;

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char          m_text[64]{0};
    Icon          m_icon{};
    IconPosition  m_icon_pos{IconPosition::Left};
    ControlStyle  m_style{Theme::make_button_style()};
    ButtonVariant m_variant{ButtonVariant::Secondary};
    bool          m_pressed{false};
    bool          m_selected{false};
    ClickCallback m_on_click{nullptr};
    void*         m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_BUTTON_HPP
