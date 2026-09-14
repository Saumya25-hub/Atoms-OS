#ifndef BOS_UI_SURFACE_HPP
#define BOS_UI_SURFACE_HPP

#include <stdint.h>
#include <stddef.h>
#include "font.hpp"
#include "types.hpp"
#include "geometry.hpp"
#include "resource.hpp"

namespace bos {

// ============================================================================
// Surface Abstraction (Mapped Native 32-bpp ARGB Framebuffer)
// ============================================================================
class Surface {
public:
    Surface();
    Surface(uint32_t window_id, uint32_t width, uint32_t height, uint32_t stride_bytes, uint32_t* pixels);
    ~Surface();

    // Move-only semantics to prevent multiple deallocations
    Surface(Surface&& other) noexcept;
    Surface& operator=(Surface&& other) noexcept;
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    // Attributes
    bool is_valid() const { return m_pixels != nullptr && m_width > 0 && m_height > 0; }
    uint32_t window_id() const { return m_window_id; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    uint32_t stride_bytes() const { return m_stride_bytes; }
    uint32_t* pixels() { return m_pixels; }
    const uint32_t* pixels() const { return m_pixels; }
    Rect bounds() const { return Rect(0, 0, m_width, m_height); }

    // Clipping
    void set_clip(const Rect& clip_rect);
    void reset_clip();
    Rect clip() const { return m_clip; }

    // Software 2D Drawing Primitives
    void clear(Color color);
    void set_pixel(int32_t x, int32_t y, Color color);
    void blend_pixel(int32_t x, int32_t y, Color color);
    Color get_pixel(int32_t x, int32_t y) const;
    void fill_rect(const Rect& rect, Color color);
    void draw_rect(const Rect& rect, Color color, uint32_t thickness = 1);
    void fill_rounded_rect(const Rect& rect, uint32_t radius, Color color);
    void draw_rounded_rect(const Rect& rect, uint32_t radius, Color color, uint32_t thickness = 1);
    void draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color);
    void draw_string(int32_t x, int32_t y, const char* text, Color fg, Color bg = Color::Transparent());
    void draw_string(int32_t x, int32_t y, const char* text, const Font& font, Color fg,
                    TextAlignment align = TextAlignment::Left, const Rect* clip_bounds = nullptr);

    // Modern Image & 9-Slice Rendering (Phase 2 Additions)
    void draw_image(const Image& image, const Rect& dest, const Rect* src = nullptr);
    void draw_image_9slice(const Image& image, const Rect& dest, const Insets& borders);

    // Typography Measurement Helpers
    static constexpr uint32_t font_glyph_width()  { return 8; }
    static constexpr uint32_t font_glyph_height() { return 16; }
    Size measure_string(const char* text) const;
    TextMetrics measure_string(const char* text, const Font& font) const;

    // Invalidation (commits dirty regions to the kernel compositor)
    void invalidate(const Rect& dirty_rect);
    void invalidate_all();

private:
    uint32_t  m_window_id{0};
    uint32_t  m_width{0};
    uint32_t  m_height{0};
    uint32_t  m_stride_bytes{0};
    uint32_t* m_pixels{nullptr};
    Rect      m_clip{0, 0, 0, 0};
};

} // namespace bos

#endif // BOS_UI_SURFACE_HPP
