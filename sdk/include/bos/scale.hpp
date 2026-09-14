#ifndef BOS_UI_SCALE_HPP
#define BOS_UI_SCALE_HPP

#include <stdint.h>
#include "geometry.hpp"

namespace bos {

// ============================================================================
// DPI & Display Scale Management
// ============================================================================
class Scale {
public:
    constexpr explicit Scale(uint32_t percent = 100) : m_percent(percent > 0 ? percent : 100) {}

    static constexpr Scale Scale100() { return Scale(100); }
    static constexpr Scale Scale125() { return Scale(125); }
    static constexpr Scale Scale150() { return Scale(150); }
    static constexpr Scale Scale175() { return Scale(175); }
    static constexpr Scale Scale200() { return Scale(200); }

    uint32_t percent() const { return m_percent; }
    void set_percent(uint32_t p) { m_percent = p > 0 ? p : 100; }

    int32_t to_physical(int32_t logical_val) const {
        if (m_percent == 100) return logical_val;
        return (logical_val * (int32_t)m_percent + 50) / 100;
    }

    uint32_t to_physical_u(uint32_t logical_val) const {
        if (m_percent == 100) return logical_val;
        return (logical_val * m_percent + 50) / 100;
    }

    int32_t to_logical(int32_t physical_val) const {
        if (m_percent == 100) return physical_val;
        return (physical_val * 100 + (int32_t)m_percent / 2) / (int32_t)m_percent;
    }

    Point to_physical(Point logical_pt) const {
        return Point(to_physical(logical_pt.x), to_physical(logical_pt.y));
    }

    Size to_physical(Size logical_size) const {
        return Size(to_physical_u(logical_size.width), to_physical_u(logical_size.height));
    }

    Rect to_physical(Rect logical_rect) const {
        return Rect(to_physical(logical_rect.x),
                    to_physical(logical_rect.y),
                    to_physical_u(logical_rect.width),
                    to_physical_u(logical_rect.height));
    }

    Insets to_physical(Insets logical_insets) const {
        return Insets(to_physical(logical_insets.left),
                      to_physical(logical_insets.top),
                      to_physical(logical_insets.right),
                      to_physical(logical_insets.bottom));
    }

private:
    uint32_t m_percent{100};
};

} // namespace bos

#endif // BOS_UI_SCALE_HPP
