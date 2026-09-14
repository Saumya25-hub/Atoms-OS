#ifndef BOS_UI_ICON_HPP
#define BOS_UI_ICON_HPP

#include <stdint.h>
#include "types.hpp"
#include "geometry.hpp"
#include "resource.hpp"
#include "surface.hpp"

namespace bos {

enum class IconPosition {
    Left,
    Right,
    Top,
    Center
};

// ============================================================================
// Modern Icon Resource with Sizing & Alignment
// ============================================================================
class Icon {
public:
    Icon() = default;

    explicit Icon(const Image& img, Size size = Size(20, 20))
        : m_image(img), m_size(size) {}

    explicit Icon(const char* png_path, Size size = Size(20, 20))
        : m_image(Image::from_file(png_path)), m_size(size) {}

    bool is_valid() const { return m_image.is_valid(); }
    const Image& image() const { return m_image; }
    Size size() const { return m_size; }
    void set_size(Size s) { m_size = s; }

    void draw(Surface& surface, Point pos) const {
        if (!is_valid() || m_size.is_empty()) return;
        Rect dest(pos.x, pos.y, m_size.width, m_size.height);
        surface.draw_image(m_image, dest);
    }

    void draw(Surface& surface, const Rect& dest_bounds) const {
        if (!is_valid() || dest_bounds.is_empty()) return;
        surface.draw_image(m_image, dest_bounds);
    }

private:
    Image m_image{};
    Size  m_size{20, 20};
};

} // namespace bos

#endif // BOS_UI_ICON_HPP
