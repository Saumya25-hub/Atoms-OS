#ifndef BOS_UI_CONTROLS_COMBOBOX_HPP
#define BOS_UI_CONTROLS_COMBOBOX_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class ComboBox : public Widget {
public:
    static constexpr size_t MAX_ITEMS = 16;
    using SelectionCallback = void (*)(ComboBox* sender, int32_t index, const char* item, void* user_data);

    ComboBox();
    ~ComboBox() override = default;

    bool add_item(const char* item);
    void clear_items();

    size_t item_count() const { return m_item_count; }
    const char* item_at(size_t index) const { return (index < m_item_count) ? m_items[index] : ""; }

    int32_t selected_index() const { return m_selected_index; }
    void set_selected_index(int32_t index);
    const char* selected_item() const { return (m_selected_index >= 0 && (size_t)m_selected_index < m_item_count) ? m_items[m_selected_index] : ""; }

    bool is_dropped_down() const { return m_dropped_down; }
    void set_dropped_down(bool dropped);

    void set_on_selection_changed(SelectionCallback cb, void* user_data = nullptr) {
        m_on_selection = cb;
        m_user_data = user_data;
    }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    char              m_items[MAX_ITEMS][48]{};
    size_t            m_item_count{0};
    int32_t           m_selected_index{-1};
    int32_t           m_hover_index{-1};
    bool              m_dropped_down{false};
    SelectionCallback m_on_selection{nullptr};
    void*             m_user_data{nullptr};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_COMBOBOX_HPP
