#include "bos/controls/image_widget.hpp"

namespace bos {

ImageWidget::ImageWidget() {
    set_size(120, 80);
}

ImageWidget::ImageWidget(const Image& image, ImageFit fit) : ImageWidget() {
    set_image(image);
    m_fit = fit;
}

void ImageWidget::set_image(const Image& image) {
    m_image = image;
    invalidate();
}

void ImageWidget::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    if (m_image.is_valid()) {
        const Bitmap& bmp = m_image.bitmap();
        Rect dest_r = abs_r;

        if (m_fit == ImageFit::Center) {
            int32_t cx = abs_r.x + ((int32_t)abs_r.width - (int32_t)bmp.width) / 2;
            int32_t cy = abs_r.y + ((int32_t)abs_r.height - (int32_t)bmp.height) / 2;
            dest_r = Rect(cx, cy, bmp.width, bmp.height);
        } else if (m_fit == ImageFit::Contain) {
            // Maintain aspect ratio fitting inside abs_r
            uint32_t w = abs_r.width;
            uint32_t h = (uint32_t)((int64_t)w * bmp.height / bmp.width);
            if (h > abs_r.height) {
                h = abs_r.height;
                w = (uint32_t)((int64_t)h * bmp.width / bmp.height);
            }
            int32_t cx = abs_r.x + ((int32_t)abs_r.width - (int32_t)w) / 2;
            int32_t cy = abs_r.y + ((int32_t)abs_r.height - (int32_t)h) / 2;
            dest_r = Rect(cx, cy, w, h);
        }

        surface.draw_image(m_image, dest_r);
    }

    if (m_border_thick > 0 && !m_border_color.is_transparent()) {
        if (m_radius > 0) {
            surface.draw_rounded_rect(abs_r, m_radius, m_border_color, m_border_thick);
        } else {
            surface.draw_rect(abs_r, m_border_color, m_border_thick);
        }
    }

    Widget::paint(surface);
}

Size ImageWidget::measure_preferred_size() const {
    if (m_image.is_valid()) {
        return m_image.size();
    }
    return Size(120, 80);
}

} // namespace bos
