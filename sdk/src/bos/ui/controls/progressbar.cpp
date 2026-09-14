#include "bos/controls/progressbar.hpp"
#include <stdio.h>

namespace bos {

ProgressBar::ProgressBar() : ProgressBar(0, 100, 0) {}

ProgressBar::ProgressBar(int32_t min, int32_t max, int32_t val)
    : m_min(min), m_max(max > min ? max : min + 1), m_val(val) {
    set_size(180, 20);
}

void ProgressBar::set_value(int32_t val) {
    if (val < m_min) val = m_min;
    if (val > m_max) val = m_max;
    if (m_val != val) {
        m_val = val;
        invalidate();
    }
}

void ProgressBar::set_range(int32_t min, int32_t max) {
    m_min = min;
    m_max = (max > min) ? max : min + 1;
    if (m_val < m_min) m_val = m_min;
    if (m_val > m_max) m_val = m_max;
    invalidate();
}

void ProgressBar::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Background Track (Pill radius)
    uint32_t r = abs_r.height / 2;
    surface.fill_rounded_rect(abs_r, r, Theme::SurfaceSubtle());
    surface.draw_rounded_rect(abs_r, r, Theme::Border(), 1);

    // 2. Filled Progress Bar
    int64_t span = m_max - m_min;
    int64_t cur = m_val - m_min;
    uint32_t fill_w = (uint32_t)((cur * abs_r.width) / span);

    if (fill_w > 0) {
        Rect fill_rect(abs_r.x, abs_r.y, fill_w, abs_r.height);
        surface.fill_rounded_rect(fill_rect, r, Theme::Accent());
    }

    // 3. Percentage Text (Inter Caption)
    if (m_show_percentage) {
        int percent = (int)((cur * 100) / span);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", percent);

        const Font& cap_font = Font::Caption();
        TextMetrics tm = surface.measure_string(buf, cap_font);
        int32_t text_x = abs_r.x + ((int32_t)abs_r.width - tm.width) / 2;
        int32_t text_y = abs_r.y + ((int32_t)abs_r.height - tm.height) / 2;
        surface.draw_string(text_x, text_y, buf, cap_font, Color::White());
    }

    Widget::paint(surface);
}

Size ProgressBar::measure_preferred_size() const {
    return Size(180, 20);
}

} // namespace bos
