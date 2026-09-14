#ifndef BOS_UI_CONTROLS_LABEL_HPP
#define BOS_UI_CONTROLS_LABEL_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class Label : public Widget {
public:
    using LinkCallback = void (*)(Label* sender, void* user_data);

    Label();
    explicit Label(const char* text, Color color = Theme::TextPrimary());
    ~Label() override = default;

    const char* text() const { return m_text; }
    void set_text(const char* text);

    Color color() const { return m_color; }
    void set_color(Color color) { m_color = color; invalidate(); }

    Alignment alignment() const { return m_alignment; }
    void set_alignment(Alignment align) { m_alignment = align; invalidate(); }

    bool is_link() const { return m_is_link; }
    void set_link(bool is_link, LinkCallback callback = nullptr, void* user_data = nullptr) {
        m_is_link = is_link;
        m_on_click = callback;
        m_user_data = user_data;
        invalidate();
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char         m_text[128]{0};
    Color        m_color{Theme::TextPrimary()};
    Alignment    m_alignment{Alignment::Start};
    bool         m_is_link{false};
    LinkCallback m_on_click{nullptr};
    void*        m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_LABEL_HPP
