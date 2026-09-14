#ifndef BOS_UI_TYPES_HPP
#define BOS_UI_TYPES_HPP

#include <stdint.h>
#include <stddef.h>

namespace bos {

// ============================================================================
// Color Representation (32-bit ARGB / 0xAARRGGBB)
// ============================================================================
class Color {
public:
    constexpr Color() : m_value(0x00000000) {}
    constexpr explicit Color(uint32_t argb) : m_value(argb) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : m_value(((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b) {}

    static constexpr Color from_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
        return Color(r, g, b, a);
    }

    static constexpr Color from_rgb(uint8_t r, uint8_t g, uint8_t b) {
        return Color(r, g, b, 255);
    }

    constexpr uint32_t argb() const { return m_value; }
    constexpr uint8_t alpha() const { return (uint8_t)((m_value >> 24) & 0xFF); }
    constexpr uint8_t red()   const { return (uint8_t)((m_value >> 16) & 0xFF); }
    constexpr uint8_t green() const { return (uint8_t)((m_value >> 8)  & 0xFF); }
    constexpr uint8_t blue()  const { return (uint8_t)(m_value & 0xFF); }

    constexpr bool is_opaque() const { return alpha() == 255; }
    constexpr bool is_transparent() const { return alpha() == 0; }
    constexpr Color with_alpha(uint8_t a) const {
        return Color((m_value & 0x00FFFFFF) | ((uint32_t)a << 24));
    }

    constexpr bool operator==(const Color& other) const { return m_value == other.m_value; }
    constexpr bool operator!=(const Color& other) const { return m_value != other.m_value; }

    // Predefined Core Palette Constants
    static constexpr Color Transparent() { return Color(0x00000000); }
    static constexpr Color Black()       { return Color(0xFF000000); }
    static constexpr Color White()       { return Color(0xFFFFFFFF); }
    static constexpr Color Gray()        { return Color(0xFF808080); }
    static constexpr Color LightGray()   { return Color(0xFFD3D3D3); }
    static constexpr Color DarkGray()    { return Color(0xFFA9A9A9); }
    static constexpr Color Red()         { return Color(0xFFFF0000); }
    static constexpr Color Green()       { return Color(0xFF00FF00); }
    static constexpr Color Blue()        { return Color(0xFF0000FF); }
    static constexpr Color Cyan()        { return Color(0xFF00FFFF); }
    static constexpr Color Magenta()     { return Color(0xFFFF00FF); }
    static constexpr Color Yellow()      { return Color(0xFFFFFF00); }

    // ATOMS OS Modern Design System Palette
    static constexpr Color Slate900()    { return Color(0xFF0F172A); } // Background
    static constexpr Color Slate800()    { return Color(0xFF1E293B); } // Card/Surface
    static constexpr Color Slate700()    { return Color(0xFF334155); } // Border/Divider
    static constexpr Color Slate600()    { return Color(0xFF475569); } // Inactive Text
    static constexpr Color Slate400()    { return Color(0xFF94A3B8); } // Secondary Text
    static constexpr Color Slate100()    { return Color(0xFFF1F5F9); } // Primary Text
    static constexpr Color Blue500()     { return Color(0xFF3B82F6); } // Primary Accent
    static constexpr Color Blue600()     { return Color(0xFF2563EB); } // Active State
    static constexpr Color Emerald500()  { return Color(0xFF10B981); } // Success/Valid
    static constexpr Color Amber500()    { return Color(0xFFF59E0B); } // Warning
    static constexpr Color Rose500()     { return Color(0xFFF43F5E); } // Error/Destructive

private:
    uint32_t m_value;
};

// ============================================================================
// Error & Result Types (Freestanding / -fno-exceptions Safe)
// ============================================================================
enum class ErrorCode : int32_t {
    Success = 0,
    InvalidParameter = -1,
    OutOfMemory = -2,
    WindowCreationFailed = -3,
    SurfaceMappingFailed = -4,
    InvalidNativeHandle = -5,
    EventSubsystemError = -6,
    ResourceNotFound = -7,
    InvalidState = -8
};

struct Result {
    ErrorCode code;

    constexpr Result() : code(ErrorCode::Success) {}
    constexpr Result(ErrorCode c) : code(c) {}

    constexpr bool is_ok() const { return code == ErrorCode::Success; }
    constexpr bool is_error() const { return code != ErrorCode::Success; }

    static constexpr Result Ok() { return Result(ErrorCode::Success); }
    static constexpr Result Fail(ErrorCode c) { return Result(c); }
};

// ============================================================================
// UI Alignment & Layout Enums
// ============================================================================
enum class Alignment {
    Start,
    Center,
    End,
    Stretch
};

enum class Orientation {
    Horizontal,
    Vertical
};

} // namespace bos

#endif // BOS_UI_TYPES_HPP
