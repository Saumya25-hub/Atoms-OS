#ifndef BOS_UI_RESOURCE_HPP
#define BOS_UI_RESOURCE_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "types.hpp"
#include "geometry.hpp"

namespace bos {

// Forward declaration
class Image;

// ============================================================================
// Bitmap Memory Layout (32-bpp ARGB Uncompressed Buffer)
// ============================================================================
struct Bitmap {
    uint32_t  width{0};
    uint32_t  height{0};
    uint32_t  stride_bytes{0};
    uint32_t* pixels{nullptr};
    bool      owns_memory{false};

    constexpr Bitmap() = default;

    Bitmap(uint32_t w, uint32_t h, uint32_t* px, bool owns = false)
        : width(w), height(h), stride_bytes(w * 4), pixels(px), owns_memory(owns) {}

    ~Bitmap() {
        if (owns_memory && pixels) {
            free(pixels);
            pixels = nullptr;
        }
    }

    // Move constructor
    Bitmap(Bitmap&& other) noexcept
        : width(other.width),
          height(other.height),
          stride_bytes(other.stride_bytes),
          pixels(other.pixels),
          owns_memory(other.owns_memory) {
        other.width = 0;
        other.height = 0;
        other.stride_bytes = 0;
        other.pixels = nullptr;
        other.owns_memory = false;
    }

    // Move assignment
    Bitmap& operator=(Bitmap&& other) noexcept {
        if (this != &other) {
            if (owns_memory && pixels) {
                free(pixels);
            }
            width = other.width;
            height = other.height;
            stride_bytes = other.stride_bytes;
            pixels = other.pixels;
            owns_memory = other.owns_memory;

            other.width = 0;
            other.height = 0;
            other.stride_bytes = 0;
            other.pixels = nullptr;
            other.owns_memory = false;
        }
        return *this;
    }

    // Copy constructor
    Bitmap(const Bitmap& other)
        : width(other.width), height(other.height), stride_bytes(other.stride_bytes), owns_memory(other.owns_memory) {
        if (other.pixels && width > 0 && height > 0) {
            if (other.owns_memory) {
                size_t num_bytes = (stride_bytes > 0) ? (stride_bytes * height) : (width * height * 4);
                pixels = (uint32_t*)malloc(num_bytes);
                if (pixels) {
                    memcpy(pixels, other.pixels, num_bytes);
                } else {
                    owns_memory = false;
                }
            } else {
                pixels = other.pixels;
            }
        } else {
            pixels = nullptr;
        }
    }

    // Copy assignment
    Bitmap& operator=(const Bitmap& other) {
        if (this != &other) {
            if (owns_memory && pixels) {
                free(pixels);
                pixels = nullptr;
            }
            width = other.width;
            height = other.height;
            stride_bytes = other.stride_bytes;
            owns_memory = other.owns_memory;

            if (other.pixels && width > 0 && height > 0) {
                if (other.owns_memory) {
                    size_t num_bytes = (stride_bytes > 0) ? (stride_bytes * height) : (width * height * 4);
                    pixels = (uint32_t*)malloc(num_bytes);
                    if (pixels) {
                        memcpy(pixels, other.pixels, num_bytes);
                    } else {
                        owns_memory = false;
                    }
                } else {
                    pixels = other.pixels;
                }
            } else {
                pixels = nullptr;
            }
        }
        return *this;
    }

    bool is_valid() const { return pixels != nullptr && width > 0 && height > 0; }
    Size size() const { return Size(width, height); }

    Color get_pixel(uint32_t x, uint32_t y) const {
        if (!pixels || x >= width || y >= height) return Color::Transparent();
        uint32_t stride_elements = (stride_bytes > 0) ? (stride_bytes / 4) : width;
        return Color(pixels[y * stride_elements + x]);
    }

    void set_pixel(uint32_t x, uint32_t y, Color color) {
        if (!pixels || x >= width || y >= height) return;
        uint32_t stride_elements = (stride_bytes > 0) ? (stride_bytes / 4) : width;
        pixels[y * stride_elements + x] = color.argb();
    }
};

// ============================================================================
// Image Resource
// ============================================================================
class Image {
public:
    Image() = default;
    explicit Image(const Bitmap& bmp) : m_bitmap(bmp) {}
    explicit Image(Bitmap&& bmp) : m_bitmap(static_cast<Bitmap&&>(bmp)) {}

    bool is_valid() const { return m_bitmap.is_valid(); }
    uint32_t width() const { return m_bitmap.width; }
    uint32_t height() const { return m_bitmap.height; }
    Size size() const { return m_bitmap.size(); }
    const Bitmap& bitmap() const { return m_bitmap; }
    Bitmap& bitmap() { return m_bitmap; }

    // Static Asset Factory Methods (Declared here, implemented in png.cpp)
    static Image from_memory(const uint8_t* data, size_t size);
    static Image from_file(const char* path);
    static Image create_solid(uint32_t w, uint32_t h, Color color);

private:
    Bitmap m_bitmap;
};

} // namespace bos

#endif // BOS_UI_RESOURCE_HPP
