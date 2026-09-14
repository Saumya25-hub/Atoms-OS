#ifndef BOS_UI_CONTROLS_SCROLLVIEW_HPP
#define BOS_UI_CONTROLS_SCROLLVIEW_HPP

#include "bos/widget.hpp"
#include "bos/theme.hpp"

namespace bos {

class ScrollView : public Widget {
public:
    ScrollView();
    ~ScrollView() override = default;

    int32_t scroll_y() const { return m_scroll_y; }
    void set_scroll_y(int32_t y);

    int32_t content_height() const { return m_content_height; }
    void set_content_height(int32_t h) { m_content_height = h; invalidate(); }

    void paint(Surface& surface) override;
    bool on_event(const Event& event) override;
    Size measure_preferred_size() const override;

private:
    int32_t m_scroll_y{0};
    int32_t m_content_height{400};
    bool    m_dragging_thumb{false};
    int32_t m_drag_start_y{0};
    int32_t m_drag_start_scroll{0};
};

} // namespace bos

#endif // BOS_UI_CONTROLS_SCROLLVIEW_HPP
