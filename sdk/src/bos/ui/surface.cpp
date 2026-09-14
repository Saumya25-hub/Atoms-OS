#include "bos/surface.hpp"
#include "bos/resource.hpp"
#include "bovisual/Text/font8x16.h"
#include <string.h>

#define SYS_GUI_INVALIDATE 21ULL

static inline int atoms_sys_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register uint64_t r10 __asm__("r10") = (uint64_t)w;
    register uint64_t r8  __asm__("r8")  = (uint64_t)h;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_INVALIDATE), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return res;
}

namespace bos {

Surface::Surface() = default;

Surface::Surface(uint32_t window_id, uint32_t width, uint32_t height, uint32_t stride_bytes, uint32_t* pixels)
    : m_window_id(window_id),
      m_width(width),
      m_height(height),
      m_stride_bytes(stride_bytes > 0 ? stride_bytes : width * 4),
      m_pixels(pixels),
      m_clip(0, 0, width, height) {}

Surface::~Surface() {
    m_pixels = nullptr;
    m_width = 0;
    m_height = 0;
    m_stride_bytes = 0;
    m_window_id = 0;
}

Surface::Surface(Surface&& other) noexcept
    : m_window_id(other.m_window_id),
      m_width(other.m_width),
      m_height(other.m_height),
      m_stride_bytes(other.m_stride_bytes),
      m_pixels(other.m_pixels),
      m_clip(other.m_clip) {
    other.m_window_id = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_stride_bytes = 0;
    other.m_pixels = nullptr;
    other.m_clip = Rect(0, 0, 0, 0);
}

Surface& Surface::operator=(Surface&& other) noexcept {
    if (this != &other) {
        m_window_id = other.m_window_id;
        m_width = other.m_width;
        m_height = other.m_height;
        m_stride_bytes = other.m_stride_bytes;
        m_pixels = other.m_pixels;
        m_clip = other.m_clip;

        other.m_window_id = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_stride_bytes = 0;
        other.m_pixels = nullptr;
        other.m_clip = Rect(0, 0, 0, 0);
    }
    return *this;
}

void Surface::set_clip(const Rect& clip_rect) {
    m_clip = clip_rect.intersect(Rect(0, 0, m_width, m_height));
}

void Surface::reset_clip() {
    m_clip = Rect(0, 0, m_width, m_height);
}

void Surface::clear(Color color) {
    if (!m_pixels || m_width == 0 || m_height == 0) return;
    uint32_t val = color.argb();
    uint32_t total_pixels = (m_stride_bytes / 4) * m_height;
    for (uint32_t i = 0; i < total_pixels; ++i) {
        m_pixels[i] = val;
    }
}

void Surface::set_pixel(int32_t x, int32_t y, Color color) {
    if (!m_pixels) return;
    if (x < m_clip.left() || x >= m_clip.right() ||
        y < m_clip.top()  || y >= m_clip.bottom()) {
        return;
    }
    uint32_t stride_elements = m_stride_bytes / 4;
    m_pixels[y * stride_elements + x] = color.argb();
}

void Surface::blend_pixel(int32_t x, int32_t y, Color color) {
    if (!m_pixels) return;
    if (x < m_clip.left() || x >= m_clip.right() ||
        y < m_clip.top()  || y >= m_clip.bottom()) {
        return;
    }
    uint8_t a = color.alpha();
    if (a == 0) return;

    uint32_t stride_elements = m_stride_bytes / 4;
    uint32_t idx = y * stride_elements + x;

    if (a == 255) {
        m_pixels[idx] = color.argb();
        return;
    }

    uint32_t dst_val = m_pixels[idx];
    uint32_t sr = color.red();
    uint32_t sg = color.green();
    uint32_t sb = color.blue();

    uint32_t dr = (dst_val >> 16) & 0xFF;
    uint32_t dg = (dst_val >> 8)  & 0xFF;
    uint32_t db = dst_val & 0xFF;

    uint32_t inv_a = 255 - a;
    uint32_t out_r = (sr * a + dr * inv_a) / 255;
    uint32_t out_g = (sg * a + dg * inv_a) / 255;
    uint32_t out_b = (sb * a + db * inv_a) / 255;

    m_pixels[idx] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
}

Color Surface::get_pixel(int32_t x, int32_t y) const {
    if (!m_pixels || x < 0 || x >= (int32_t)m_width || y < 0 || y >= (int32_t)m_height) {
        return Color::Transparent();
    }
    uint32_t stride_elements = m_stride_bytes / 4;
    return Color(m_pixels[y * stride_elements + x]);
}

void Surface::fill_rect(const Rect& rect, Color color) {
    if (!m_pixels || rect.is_empty()) return;
    Rect target = rect.intersect(m_clip);
    if (target.is_empty()) return;

    if (color.alpha() < 255) {
        for (int32_t y = target.top(); y < target.bottom(); ++y) {
            for (int32_t x = target.left(); x < target.right(); ++x) {
                blend_pixel(x, y, color);
            }
        }
        return;
    }

    uint32_t val = color.argb();
    uint32_t stride_elements = m_stride_bytes / 4;

    for (int32_t y = target.top(); y < target.bottom(); ++y) {
        uint32_t* row = &m_pixels[y * stride_elements + target.left()];
        for (int32_t x = 0; x < (int32_t)target.width; ++x) {
            row[x] = val;
        }
    }
}

void Surface::draw_rect(const Rect& rect, Color color, uint32_t thickness) {
    if (!m_pixels || rect.is_empty() || thickness == 0) return;
    fill_rect(Rect(rect.x, rect.y, rect.width, thickness), color);
    fill_rect(Rect(rect.x, rect.bottom() - (int32_t)thickness, rect.width, thickness), color);
    fill_rect(Rect(rect.x, rect.y, thickness, rect.height), color);
    fill_rect(Rect(rect.right() - (int32_t)thickness, rect.y, thickness, rect.height), color);
}

void Surface::fill_rounded_rect(const Rect& rect, uint32_t radius, Color color) {
    if (!m_pixels || rect.is_empty()) return;
    Rect target = rect.intersect(m_clip);
    if (target.is_empty()) return;

    int32_t r = (int32_t)radius;
    if (r * 2 > (int32_t)rect.width)  r = (int32_t)rect.width / 2;
    if (r * 2 > (int32_t)rect.height) r = (int32_t)rect.height / 2;

    if (r <= 0) {
        fill_rect(rect, color);
        return;
    }

    int32_t r_inner = (r > 1) ? (r - 1) : 0;
    int32_t r2_inner = r_inner * r_inner;
    int32_t r2_outer = r * r;
    int32_t r2_denom = (r2_outer > r2_inner) ? (r2_outer - r2_inner) : 1;

    int32_t left_corner   = rect.x + r;
    int32_t right_corner  = rect.right() - r;
    int32_t top_corner    = rect.y + r;
    int32_t bottom_corner = rect.bottom() - r;

    for (int32_t y = target.top(); y < target.bottom(); ++y) {
        for (int32_t x = target.left(); x < target.right(); ++x) {
            int32_t dx = 0, dy = 0;
            bool in_corner = false;

            if (x < left_corner && y < top_corner) {
                dx = left_corner - x;
                dy = top_corner - y;
                in_corner = true;
            } else if (x >= right_corner && y < top_corner) {
                dx = x - right_corner + 1;
                dy = top_corner - y;
                in_corner = true;
            } else if (x < left_corner && y >= bottom_corner) {
                dx = left_corner - x;
                dy = y - bottom_corner + 1;
                in_corner = true;
            } else if (x >= right_corner && y >= bottom_corner) {
                dx = x - right_corner + 1;
                dy = y - bottom_corner + 1;
                in_corner = true;
            }

            if (!in_corner) {
                blend_pixel(x, y, color);
                continue;
            }

            int32_t d2 = dx * dx + dy * dy;
            if (d2 <= r2_inner) {
                blend_pixel(x, y, color);
            } else if (d2 <= r2_outer) {
                uint32_t cov = ((uint32_t)(r2_outer - d2) * 255) / (uint32_t)r2_denom;
                if (cov > 255) cov = 255;
                uint32_t blended_a = ((uint32_t)color.alpha() * cov) / 255;
                blend_pixel(x, y, color.with_alpha((uint8_t)blended_a));
            }
        }
    }
}

void Surface::draw_rounded_rect(const Rect& rect, uint32_t radius, Color color, uint32_t thickness) {
    if (!m_pixels || rect.is_empty() || thickness == 0) return;
    int32_t r = (int32_t)radius;
    if (r * 2 > (int32_t)rect.width)  r = (int32_t)rect.width / 2;
    if (r * 2 > (int32_t)rect.height) r = (int32_t)rect.height / 2;

    if (r <= 0) {
        draw_rect(rect, color, thickness);
        return;
    }

    int32_t r_outer2 = r * r;
    int32_t r_inner = r - (int32_t)thickness;
    int32_t r_inner2 = (r_inner > 0) ? (r_inner * r_inner) : 0;

    Rect target = rect.intersect(m_clip);
    int32_t left_corner   = rect.x + r;
    int32_t right_corner  = rect.right() - r;
    int32_t top_corner    = rect.y + r;
    int32_t bottom_corner = rect.bottom() - r;

    for (int32_t y = target.top(); y < target.bottom(); ++y) {
        for (int32_t x = target.left(); x < target.right(); ++x) {
            bool in_outer = true;
            bool in_inner = false;

            if (x < left_corner && y < top_corner) {
                int32_t dx = left_corner - x;
                int32_t dy = top_corner - y;
                int32_t d2 = dx * dx + dy * dy;
                in_outer = (d2 <= r_outer2);
                in_inner = (r_inner > 0 && d2 <= r_inner2);
            } else if (x >= right_corner && y < top_corner) {
                int32_t dx = x - right_corner + 1;
                int32_t dy = top_corner - y;
                int32_t d2 = dx * dx + dy * dy;
                in_outer = (d2 <= r_outer2);
                in_inner = (r_inner > 0 && d2 <= r_inner2);
            } else if (x < left_corner && y >= bottom_corner) {
                int32_t dx = left_corner - x;
                int32_t dy = y - bottom_corner + 1;
                int32_t d2 = dx * dx + dy * dy;
                in_outer = (d2 <= r_outer2);
                in_inner = (r_inner > 0 && d2 <= r_inner2);
            } else if (x >= right_corner && y >= bottom_corner) {
                int32_t dx = x - right_corner + 1;
                int32_t dy = y - bottom_corner + 1;
                int32_t d2 = dx * dx + dy * dy;
                in_outer = (d2 <= r_outer2);
                in_inner = (r_inner > 0 && d2 <= r_inner2);
            } else {
                // In straight edges
                bool on_border = (x < rect.x + (int32_t)thickness ||
                                  x >= rect.right() - (int32_t)thickness ||
                                  y < rect.y + (int32_t)thickness ||
                                  y >= rect.bottom() - (int32_t)thickness);
                if (on_border) {
                    blend_pixel(x, y, color);
                }
                continue;
            }

            if (in_outer && !in_inner) {
                blend_pixel(x, y, color);
            }
        }
    }
}

void Surface::draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color) {
    if (!m_pixels) return;

    int32_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int32_t dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;

    while (true) {
        blend_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Surface::draw_string(int32_t x, int32_t y, const char* text, Color fg, Color bg) {
    if (!m_pixels || !text) return;

    int32_t cur_x = x;
    int32_t cur_y = y;

    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c == '\n') {
            cur_x = x;
            cur_y += 16;
            continue;
        }
        if (c == '\r') continue;

        uint8_t uc = (uint8_t)c;
        const uint8_t* glyph = g_font8x16_stub[uc];

        for (int row = 0; row < 16; ++row) {
            int32_t py = cur_y + row;
            uint8_t bits = glyph[row];

            for (int col = 0; col < 8; ++col) {
                int32_t px = cur_x + col;
                if ((bits >> col) & 1) {
                    blend_pixel(px, py, fg);
                } else if (!bg.is_transparent()) {
                    blend_pixel(px, py, bg);
                }
            }
        }
        cur_x += 8;
    }
}

Size Surface::measure_string(const char* text) const {
    if (!text) return Size(0, 0);

    uint32_t max_line_len = 0;
    uint32_t cur_line_len = 0;
    uint32_t line_count = 1;

    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            line_count++;
            if (cur_line_len > max_line_len) max_line_len = cur_line_len;
            cur_line_len = 0;
        } else if (text[i] != '\r') {
            cur_line_len++;
        }
    }
    if (cur_line_len > max_line_len) max_line_len = cur_line_len;

    return Size(max_line_len * 8, line_count * 16);
}

void Surface::draw_string(int32_t x, int32_t y, const char* text, const Font& font, Color fg,
                          TextAlignment align, const Rect* clip_bounds) {
    if (!m_pixels || !text || text[0] == '\0' || !font.is_valid()) return;

    int32_t cur_y = y;
    const char* p = text;

    while (*p != '\0') {
        const char* line_start = p;
        size_t line_len = 0;
        while (*p != '\0' && *p != '\n' && *p != '\r') {
            line_len++;
            p++;
        }

        // Measure line width for alignment
        int32_t line_w = 0;
        for (size_t i = 0; i < line_len; ++i) {
            const GlyphData* g = font.get_glyph(line_start[i]);
            if (g) line_w += g->advance_x;
        }

        // Alignment offset
        int32_t cur_x = x;
        if (align == TextAlignment::Center) {
            if (clip_bounds) {
                cur_x = clip_bounds->x + ((int32_t)clip_bounds->width - line_w) / 2;
            } else {
                cur_x = x - line_w / 2;
            }
        } else if (align == TextAlignment::Right) {
            if (clip_bounds) {
                cur_x = clip_bounds->right() - line_w;
            } else {
                cur_x = x - line_w;
            }
        }

        int32_t baseline_y = cur_y + font.ascent();

        for (size_t i = 0; i < line_len; ++i) {
            char c = line_start[i];
            const GlyphData* g = font.get_glyph(c);
            if (!g) continue;

            if (g->width > 0 && g->height > 0) {
                const uint8_t* bmp = font.get_glyph_bitmap(*g);
                if (bmp) {
                    int32_t gx = cur_x + g->bearing_x;
                    int32_t gy = baseline_y + g->bearing_y;

                    for (uint32_t row = 0; row < g->height; ++row) {
                        int32_t py = gy + row;
                        for (uint32_t col = 0; col < g->width; ++col) {
                            int32_t px = gx + col;
                            uint8_t alpha = bmp[row * g->width + col];
                            if (alpha > 0) {
                                uint32_t final_a = ((uint32_t)fg.alpha() * alpha) / 255;
                                blend_pixel(px, py, fg.with_alpha((uint8_t)final_a));
                            }
                        }
                    }
                }
            }
            cur_x += g->advance_x;
        }

        cur_y += font.line_height();

        if (*p == '\r') p++;
        if (*p == '\n') p++;
    }
}

TextMetrics Surface::measure_string(const char* text, const Font& font) const {
    return font.measure(text);
}

void Surface::draw_image(const Image& image, const Rect& dest, const Rect* src) {
    if (!m_pixels || !image.is_valid() || dest.is_empty()) return;

    const Bitmap& bmp = image.bitmap();
    Rect src_r = src ? *src : Rect(0, 0, bmp.width, bmp.height);
    if (src_r.is_empty()) return;

    Rect clipped_dest = dest.intersect(m_clip);
    if (clipped_dest.is_empty()) return;

    for (int32_t dy = clipped_dest.top(); dy < clipped_dest.bottom(); ++dy) {
        int32_t sy = src_r.y + (int32_t)(((int64_t)(dy - dest.y) * src_r.height) / dest.height);
        if (sy < 0 || sy >= (int32_t)bmp.height) continue;

        for (int32_t dx = clipped_dest.left(); dx < clipped_dest.right(); ++dx) {
            int32_t sx = src_r.x + (int32_t)(((int64_t)(dx - dest.x) * src_r.width) / dest.width);
            if (sx < 0 || sx >= (int32_t)bmp.width) continue;

            Color c = bmp.get_pixel((uint32_t)sx, (uint32_t)sy);
            blend_pixel(dx, dy, c);
        }
    }
}

void Surface::draw_image_9slice(const Image& image, const Rect& dest, const Insets& borders) {
    if (!m_pixels || !image.is_valid() || dest.is_empty()) return;

    const Bitmap& bmp = image.bitmap();
    uint32_t sw = bmp.width;
    uint32_t sh = bmp.height;

    int32_t bl = borders.left;
    int32_t bt = borders.top;
    int32_t br = borders.right;
    int32_t bb = borders.bottom;

    if (bl + br > (int32_t)dest.width || bt + bb > (int32_t)dest.height ||
        bl + br > (int32_t)sw || bt + bb > (int32_t)sh) {
        // Fallback to regular scaled draw if borders exceed dimensions
        draw_image(image, dest);
        return;
    }

    int32_t mid_sw = (int32_t)sw - bl - br;
    int32_t mid_sh = (int32_t)sh - bt - bb;
    int32_t mid_dw = (int32_t)dest.width - bl - br;
    int32_t mid_dh = (int32_t)dest.height - bt - bb;

    // 1. Top-Left Corner
    Rect s_tl(0, 0, bl, bt);
    Rect d_tl(dest.x, dest.y, bl, bt);
    draw_image(image, d_tl, &s_tl);

    // 2. Top Edge
    if (mid_dw > 0 && mid_sw > 0) {
        Rect s_top(bl, 0, mid_sw, bt);
        Rect d_top(dest.x + bl, dest.y, mid_dw, bt);
        draw_image(image, d_top, &s_top);
    }

    // 3. Top-Right Corner
    Rect s_tr((int32_t)sw - br, 0, br, bt);
    Rect d_tr(dest.right() - br, dest.y, br, bt);
    draw_image(image, d_tr, &s_tr);

    // 4. Left Edge
    if (mid_dh > 0 && mid_sh > 0) {
        Rect s_left(0, bt, bl, mid_sh);
        Rect d_left(dest.x, dest.y + bt, bl, mid_dh);
        draw_image(image, d_left, &s_left);
    }

    // 5. Center
    if (mid_dw > 0 && mid_dh > 0 && mid_sw > 0 && mid_sh > 0) {
        Rect s_center(bl, bt, mid_sw, mid_sh);
        Rect d_center(dest.x + bl, dest.y + bt, mid_dw, mid_dh);
        draw_image(image, d_center, &s_center);
    }

    // 6. Right Edge
    if (mid_dh > 0 && mid_sh > 0) {
        Rect s_right((int32_t)sw - br, bt, br, mid_sh);
        Rect d_right(dest.right() - br, dest.y + bt, br, mid_dh);
        draw_image(image, d_right, &s_right);
    }

    // 7. Bottom-Left Corner
    Rect s_bl(0, (int32_t)sh - bb, bl, bb);
    Rect d_bl(dest.x, dest.bottom() - bb, bl, bb);
    draw_image(image, d_bl, &s_bl);

    // 8. Bottom Edge
    if (mid_dw > 0 && mid_sw > 0) {
        Rect s_bottom(bl, (int32_t)sh - bb, mid_sw, bb);
        Rect d_bottom(dest.x + bl, dest.bottom() - bb, mid_dw, bb);
        draw_image(image, d_bottom, &s_bottom);
    }

    // 9. Bottom-Right Corner
    Rect s_br((int32_t)sw - br, (int32_t)sh - bb, br, bb);
    Rect d_br(dest.right() - br, dest.bottom() - bb, br, bb);
    draw_image(image, d_br, &s_br);
}

void Surface::invalidate(const Rect& dirty_rect) {
    if (m_window_id == 0) return;
    Rect target = dirty_rect.intersect(Rect(0, 0, m_width, m_height));
    if (!target.is_empty()) {
        atoms_sys_gui_invalidate(m_window_id, target.x, target.y, target.width, target.height);
    }
}

void Surface::invalidate_all() {
    invalidate(Rect(0, 0, m_width, m_height));
}

} // namespace bos
