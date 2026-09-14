#ifndef BOS_UI_WINDOW_HPP
#define BOS_UI_WINDOW_HPP

#include <stdint.h>
#include <stddef.h>
#include "types.hpp"
#include "geometry.hpp"
#include "events.hpp"
#include "surface.hpp"
#include "widget.hpp"
#include "resource.hpp"

namespace bos {

class Application;

// ============================================================================
// Window State Model
// ============================================================================
enum class WindowState : uint32_t {
    Normal = 0,
    Minimized,
    Maximized,
    Active,
    Inactive,
    Closing,
    Closed
};

// ============================================================================
// Window Class (Native ATOMS OS Window RAII Controller)
// ============================================================================
class Window {
public:
    using CloseCallback      = void (*)(Window*);
    using ResizeCallback     = void (*)(Window*, Size);
    using ActivateCallback   = void (*)(Window*);
    using DeactivateCallback = void (*)(Window*);

    Window(const char* title, int32_t x = 100, int32_t y = 100, uint32_t width = 800, uint32_t height = 600);
    ~Window();

    // Move-only semantics (RAII ownership of native window handle)
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Attributes
    uint32_t native_id() const { return m_window_id; }
    bool is_valid() const { return m_window_id != 0; }
    bool is_visible() const { return m_visible; }
    bool is_closed() const { return m_closed; }
    const char* title() const { return m_title; }
    Rect bounds() const { return m_bounds; }
    Size size() const { return m_bounds.size(); }

    // State Inspection
    WindowState window_state() const { return m_state; }
    bool is_minimized() const { return m_state == WindowState::Minimized; }
    bool is_maximized() const { return m_state == WindowState::Maximized; }
    bool is_active() const { return m_active; }

    Surface& surface() { return m_surface; }
    const Surface& surface() const { return m_surface; }

    // Client Area vs Non-Client Area Metrics
    Rect client_bounds() const;
    Size client_size() const;
    static int32_t title_bar_height() { return 35; }
    static int32_t border_thickness() { return 5; }
    Point window_to_client(const Point& pt) const;
    Point client_to_window(const Point& pt) const;

    // Lifecycle Actions & Window State Machine
    void show();
    void hide();
    void close();
    void minimize();
    void maximize();
    void restore();
    void activate();
    void set_active(bool active);

    void set_bounds(int32_t x, int32_t y, uint32_t width, uint32_t height);
    void set_size(uint32_t width, uint32_t height);
    void set_position(int32_t x, int32_t y);
    void set_title(const char* title);

    // Window Icon Integration
    void set_icon(const Image& icon);
    const Image* icon() const { return m_has_icon ? &m_icon : nullptr; }

    // Widget Tree & Rendering
    void set_root_widget(Widget* root);
    Widget* root_widget() const { return m_root_widget; }
    void render();
    void invalidate();
    void invalidate(const Rect& dirty_rect);

    // Mouse Capture (Phase 4)
    void set_mouse_capture(Widget* w) { m_captured_widget = w; }
    void release_mouse_capture() { m_captured_widget = nullptr; }
    Widget* mouse_capture() const { return m_captured_widget; }

    // Focus Management & Traversal (Phase 4)
    void set_focused_widget(Widget* w);
    Widget* focused_widget() const { return m_focused_widget; }
    void focus_next_widget();
    void focus_prev_widget();

    // Event Handling
    void dispatch_event(const Event& event);

    // Callback Registration
    void set_on_close(CloseCallback cb) { m_on_close = cb; }
    void set_on_resize(ResizeCallback cb) { m_on_resize = cb; }
    void set_on_activate(ActivateCallback cb) { m_on_activate = cb; }
    void set_on_deactivate(DeactivateCallback cb) { m_on_deactivate = cb; }

private:
    friend class Application;
    void map_native_surface();
    void unmap_native_surface();
    void destroy_native_window();

    uint32_t           m_window_id{0};
    char               m_title[128]{0};
    Rect               m_bounds{100, 100, 800, 600};
    Rect               m_normal_bounds{100, 100, 800, 600};
    bool               m_visible{false};
    bool               m_closed{false};
    bool               m_active{false};
    WindowState        m_state{WindowState::Normal};

    Image              m_icon;
    bool               m_has_icon{false};

    Surface            m_surface;
    Widget*            m_root_widget{nullptr};
    Widget*            m_hovered_widget{nullptr};
    Widget*            m_focused_widget{nullptr};
    Widget*            m_captured_widget{nullptr};

    CloseCallback      m_on_close{nullptr};
    ResizeCallback     m_on_resize{nullptr};
    ActivateCallback   m_on_activate{nullptr};
    DeactivateCallback m_on_deactivate{nullptr};
};

} // namespace bos

#endif // BOS_UI_WINDOW_HPP
