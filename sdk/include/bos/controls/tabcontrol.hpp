#ifndef BOS_UI_CONTROLS_TABCONTROL_HPP
#define BOS_UI_CONTROLS_TABCONTROL_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

struct TabPage {
    char    title[32]{0};
    Widget* content{nullptr};
};

class TabControl : public Widget {
public:
    static constexpr size_t MAX_TABS = 8;
    using TabChangedCallback = void (*)(TabControl* sender, int32_t index, void* user_data);

    TabControl();
    ~TabControl() override = default;

    bool add_tab(const char* title, Widget* content = nullptr);
    void clear_tabs();

    size_t tab_count() const { return m_tab_count; }
    int32_t active_tab() const { return m_active_tab; }
    void set_active_tab(int32_t index);

    void set_on_tab_changed(TabChangedCallback cb, void* user_data = nullptr) {
        m_on_tab_changed = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    TabPage            m_tabs[MAX_TABS]{};
    size_t             m_tab_count{0};
    int32_t            m_active_tab{0};
    int32_t            m_hover_tab{-1};
    uint32_t           m_header_height{34};
    TabChangedCallback m_on_tab_changed{nullptr};
    void*              m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_TABCONTROL_HPP
