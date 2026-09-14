#ifndef BOS_UI_CONTROLS_LISTVIEW_HPP
#define BOS_UI_CONTROLS_LISTVIEW_HPP

#include "bos/widget.hpp"
#include "bos/icon.hpp"
#include "bos/theme.hpp"

namespace bos {

struct ListViewItem {
    char text[64]{0};
    char detail[64]{0};
    Icon icon{};
};

class ListView : public Widget {
public:
    static constexpr size_t MAX_ITEMS = 32;
    using ItemClickCallback = void (*)(ListView* sender, int32_t index, void* user_data);

    ListView();
    ~ListView() override = default;

    bool add_item(const char* text, const Icon& icon = Icon(), const char* detail = nullptr);
    void clear_items();

    size_t item_count() const { return m_item_count; }
    const ListViewItem& item_at(size_t index) const { return m_items[index]; }

    int32_t selected_index() const { return m_selected_index; }
    void set_selected_index(int32_t index);

    uint32_t item_height() const { return m_item_height; }
    void set_item_height(uint32_t h) { m_item_height = h; invalidate(); }

    void set_on_item_click(ItemClickCallback cb, void* user_data = nullptr) {
        m_on_item_click = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    ListViewItem      m_items[MAX_ITEMS]{};
    size_t            m_item_count{0};
    int32_t           m_selected_index{-1};
    int32_t           m_hover_index{-1};
    uint32_t          m_item_height{32};
    ItemClickCallback m_on_item_click{nullptr};
    void*             m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_LISTVIEW_HPP
