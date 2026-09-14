#ifndef BOS_UI_EVENTS_HPP
#define BOS_UI_EVENTS_HPP

#include <stdint.h>
#include "geometry.hpp"

namespace bos {

// ============================================================================
// Event Classifications
// ============================================================================
enum class EventType : uint32_t {
    None = 0,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseDown,
    MouseUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    CharInput,
    WindowClose,
    WindowResize,
    WindowMove,
    FocusGained,
    FocusLost
};

enum class EventPhase : uint32_t {
    Capture = 0,
    Target  = 1,
    Bubble  = 2
};

enum class MouseButton : uint32_t {
    None   = 0,
    Left   = 1,
    Right  = 2,
    Middle = 4
};

enum class Modifiers : uint32_t {
    None  = 0,
    Shift = 1,
    Ctrl  = 2,
    Alt   = 4
};

// ============================================================================
// Event Structure (Directly mapped to ATOMS BOS_GUIEvent ABI with Phase 4 Routing)
// ============================================================================
struct Event {
    EventType   type{EventType::None};
    EventPhase  phase{EventPhase::Target};
    uint32_t    window_id{0};
    Point       mouse_pos{0, 0};     // Window-local coordinates
    MouseButton mouse_button{MouseButton::None};
    int32_t     wheel_dx{0};
    int32_t     wheel_dy{0};
    uint32_t    key_code{0};        // Hardware keycode
    uint32_t    ascii_char{0};      // Printable ASCII character
    uint32_t    modifiers{0};       // Shift, Ctrl, Alt bitmask
    bool        handled{false};

    constexpr bool is_mouse_event() const {
        return type == EventType::MouseMove ||
               type == EventType::MouseEnter ||
               type == EventType::MouseLeave ||
               type == EventType::MouseDown ||
               type == EventType::MouseUp ||
               type == EventType::MouseWheel;
    }

    constexpr bool is_key_event() const {
        return type == EventType::KeyDown ||
               type == EventType::KeyUp ||
               type == EventType::CharInput;
    }

    constexpr bool is_window_event() const {
        return type == EventType::WindowClose ||
               type == EventType::WindowResize ||
               type == EventType::WindowMove ||
               type == EventType::FocusGained ||
               type == EventType::FocusLost;
    }

    constexpr bool has_shift() const { return (modifiers & (uint32_t)Modifiers::Shift) != 0; }
    constexpr bool has_ctrl()  const { return (modifiers & (uint32_t)Modifiers::Ctrl)  != 0; }
    constexpr bool has_alt()   const { return (modifiers & (uint32_t)Modifiers::Alt)   != 0; }
};

} // namespace bos

#endif // BOS_UI_EVENTS_HPP
