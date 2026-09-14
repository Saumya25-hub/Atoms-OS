#ifndef BOS_UI_CONTROLS_TEXTBOX_HPP
#define BOS_UI_CONTROLS_TEXTBOX_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class TextBox : public Widget {
public:
    using TextChangedCallback = void (*)(TextBox* sender, void* user_data);

    TextBox();
    explicit TextBox(const char* placeholder);
    ~TextBox() override = default;

    const char* text() const { return m_text; }
    void set_text(const char* text);

    const char* placeholder() const { return m_placeholder; }
    void set_placeholder(const char* placeholder);

    bool is_read_only() const { return m_read_only; }
    void set_read_only(bool ro) { m_read_only = ro; }

    void set_on_text_changed(TextChangedCallback cb, void* user_data = nullptr) {
        m_on_text_changed = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char                m_text[128]{0};
    char                m_placeholder[64]{0};
    size_t              m_cursor_pos{0};
    bool                m_read_only{false};
    TextChangedCallback m_on_text_changed{nullptr};
    void*               m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_TEXTBOX_HPP
