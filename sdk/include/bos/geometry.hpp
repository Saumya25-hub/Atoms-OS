#ifndef BOS_UI_GEOMETRY_HPP
#define BOS_UI_GEOMETRY_HPP

#include <stdint.h>

namespace bos {

// ============================================================================
// Point (2D Integer Coordinates)
// ============================================================================
struct Point {
    int32_t x{0};
    int32_t y{0};

    constexpr Point() = default;
    constexpr Point(int32_t in_x, int32_t in_y) : x(in_x), y(in_y) {}

    constexpr bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    constexpr bool operator!=(const Point& other) const { return x != other.x || y != other.y; }

    constexpr Point operator+(const Point& other) const { return Point(x + other.x, y + other.y); }
    constexpr Point operator-(const Point& other) const { return Point(x - other.x, y - other.y); }

    Point& operator+=(const Point& other) { x += other.x; y += other.y; return *this; }
    Point& operator-=(const Point& other) { x -= other.x; y -= other.y; return *this; }
};

// ============================================================================
// Size (2D Dimensions)
// ============================================================================
struct Size {
    uint32_t width{0};
    uint32_t height{0};

    constexpr Size() = default;
    constexpr Size(uint32_t in_w, uint32_t in_h) : width(in_w), height(in_h) {}

    constexpr bool operator==(const Size& other) const { return width == other.width && height == other.height; }
    constexpr bool operator!=(const Size& other) const { return width != other.width || height != other.height; }

    constexpr bool is_empty() const { return width == 0 || height == 0; }
};

// ============================================================================
// Insets / Margins / Padding
// ============================================================================
struct Insets {
    int32_t left{0};
    int32_t top{0};
    int32_t right{0};
    int32_t bottom{0};

    constexpr Insets() = default;
    constexpr Insets(int32_t all) : left(all), top(all), right(all), bottom(all) {}
    constexpr Insets(int32_t horizontal, int32_t vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Insets(int32_t l, int32_t t, int32_t r, int32_t b)
        : left(l), top(t), right(r), bottom(b) {}

    constexpr int32_t horizontal() const { return left + right; }
    constexpr int32_t vertical()   const { return top + bottom; }

    constexpr bool operator==(const Insets& other) const {
        return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
    }
    constexpr bool operator!=(const Insets& other) const { return !(*this == other); }
};

// ============================================================================
// Rect (2D Rectangle)
// ============================================================================
struct Rect {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};

    constexpr Rect() = default;
    constexpr Rect(int32_t in_x, int32_t in_y, uint32_t in_w, uint32_t in_h)
        : x(in_x), y(in_y), width(in_w), height(in_h) {}
    constexpr Rect(Point origin, Size size)
        : x(origin.x), y(origin.y), width(size.width), height(size.height) {}

    constexpr int32_t left()   const { return x; }
    constexpr int32_t top()    const { return y; }
    constexpr int32_t right()  const { return x + (int32_t)width; }
    constexpr int32_t bottom() const { return y + (int32_t)height; }

    constexpr Point origin() const { return Point(x, y); }
    constexpr Size size() const { return Size(width, height); }

    constexpr bool is_empty() const { return width == 0 || height == 0; }

    constexpr bool contains(int32_t px, int32_t py) const {
        return px >= x && px < (x + (int32_t)width) &&
               py >= y && py < (y + (int32_t)height);
    }

    constexpr bool contains(const Point& p) const {
        return contains(p.x, p.y);
    }

    constexpr bool contains(const Rect& other) const {
        return other.left() >= left() && other.right() <= right() &&
               other.top() >= top() && other.bottom() <= bottom();
    }

    constexpr bool intersects(const Rect& other) const {
        return !(left() >= other.right() || right() <= other.left() ||
                 top() >= other.bottom() || bottom() <= other.top());
    }

    constexpr Rect intersect(const Rect& other) const {
        int32_t nx = (left() > other.left()) ? left() : other.left();
        int32_t ny = (top() > other.top()) ? top() : other.top();
        int32_t nr = (right() < other.right()) ? right() : other.right();
        int32_t nb = (bottom() < other.bottom()) ? bottom() : other.bottom();

        if (nr <= nx || nb <= ny) return Rect(0, 0, 0, 0);
        return Rect(nx, ny, (uint32_t)(nr - nx), (uint32_t)(nb - ny));
    }

    constexpr Rect offset(int32_t dx, int32_t dy) const {
        return Rect(x + dx, y + dy, width, height);
    }

    constexpr Rect inset(const Insets& insets) const {
        int32_t nw = (int32_t)width - insets.horizontal();
        int32_t nh = (int32_t)height - insets.vertical();
        return Rect(x + insets.left, y + insets.top,
                    (nw > 0) ? (uint32_t)nw : 0,
                    (nh > 0) ? (uint32_t)nh : 0);
    }

    constexpr bool operator==(const Rect& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    constexpr bool operator!=(const Rect& other) const { return !(*this == other); }
};

} // namespace bos

#endif // BOS_UI_GEOMETRY_HPP
