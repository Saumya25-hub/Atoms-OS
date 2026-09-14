#ifndef BOS_UI_CONTROLS_RADIOBUTTON_HPP
#define BOS_UI_CONTROLS_RADIOBUTTON_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class RadioButton : public Widget {
public:
    using SelectCallback = void (*)(RadioButton* sender, uint32_t group_id, void* user_data);

    RadioButton();
    RadioButton(const char* text, uint32_t group_id = 0, bool selected = false);
    ~RadioButton() override = default;

    const char* text() const { return m_text; }
    void set_text(const char* text);

    uint32_t group_id() const { return m_group_id; }
    void set_group_id(uint32_t group_id) { m_group_id = group_id; }

    bool is_selected() const { return m_selected; }
    void set_selected(bool selected);

    void set_on_select(SelectCallback cb, void* user_data = nullptr) {
        m_on_select = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char           m_text[64]{0};
    uint32_t       m_group_id{0};
    bool           m_selected{false};
    SelectCallback m_on_select{nullptr};
    void*          m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_RADIOBUTTON_HPP
