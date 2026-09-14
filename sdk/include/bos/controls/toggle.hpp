#ifndef BOS_UI_CONTROLS_TOGGLE_HPP
#define BOS_UI_CONTROLS_TOGGLE_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class Toggle : public Widget {
public:
    using ToggleCallback = void (*)(Toggle* sender, bool is_on, void* user_data);

    Toggle();
    explicit Toggle(bool is_on);
    ~Toggle() override = default;

    bool is_on() const { return m_is_on; }
    void set_on(bool is_on);

    void set_on_toggle(ToggleCallback cb, void* user_data = nullptr) {
        m_on_toggle = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    bool           m_is_on{false};
    ToggleCallback m_on_toggle{nullptr};
    void*          m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_TOGGLE_HPP
