#ifndef BOS_UI_CONTROLS_CHECKBOX_HPP
#define BOS_UI_CONTROLS_CHECKBOX_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class CheckBox : public Widget {
public:
    using ToggleCallback = void (*)(CheckBox* sender, bool checked, void* user_data);

    CheckBox();
    explicit CheckBox(const char* text, bool checked = false);
    ~CheckBox() override = default;

    const char* text() const { return m_text; }
    void set_text(const char* text);

    bool is_checked() const { return m_checked; }
    void set_checked(bool checked);

    void set_on_toggle(ToggleCallback cb, void* user_data = nullptr) {
        m_on_toggle = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char           m_text[64]{0};
    bool           m_checked{false};
    ToggleCallback m_on_toggle{nullptr};
    void*          m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_CHECKBOX_HPP
