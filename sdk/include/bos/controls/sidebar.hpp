#ifndef BOS_UI_CONTROLS_SIDEBAR_HPP
#define BOS_UI_CONTROLS_SIDEBAR_HPP

#include "bos/widget.hpp"
#include "bos/icon.hpp"
#include "bos/theme.hpp"

namespace bos {

struct SidebarItem {
    char label[48]{0};
    Icon icon{};
};

class Sidebar : public Widget {
public:
    static constexpr size_t MAX_ITEMS = 16;
    using SelectionCallback = void (*)(Sidebar* sender, int32_t index, const char* label, void* user_data);

    Sidebar();
    ~Sidebar() override = default;

    bool add_item(const char* label, const Icon& icon = Icon());
    void clear_items();

    size_t item_count() const { return m_item_count; }
    const SidebarItem& item_at(size_t index) const { return m_items[index]; }

    int32_t selected_index() const { return m_selected_index; }
    void set_selected_index(int32_t index);

    void set_on_item_selected(SelectionCallback cb, void* user_data = nullptr) {
        m_on_selected = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    SidebarItem       m_items[MAX_ITEMS]{};
    size_t            m_item_count{0};
    int32_t           m_selected_index{0};
    int32_t           m_hover_index{-1};
    uint32_t          m_item_height{38};
    SelectionCallback m_on_selected{nullptr};
    void*             m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_SIDEBAR_HPP
