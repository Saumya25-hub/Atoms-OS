#ifndef BOS_UI_APPLICATION_HPP
#define BOS_UI_APPLICATION_HPP

#include <stdint.h>
#include <stddef.h>
#include "types.hpp"
#include "window.hpp"

namespace bos {

// ============================================================================
// Application Class (Top-Level ATOMS OS Application Lifetime & Event Loop)
// ============================================================================
class Application {
public:
    static constexpr size_t MAX_WINDOWS = 16;

    Application(int argc = 0, char** argv = nullptr);
    ~Application();

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static Application* instance() { return s_instance; }

    // Event Loop
    int run();
    void quit(int exit_code = 0);
    bool is_running() const { return m_running; }

    // Window Registry
    bool register_window(Window* win);
    void unregister_window(Window* win);
    Window* find_window(uint32_t window_id);
    size_t active_window_count() const { return m_window_count; }
    Window* active_window() const { return m_active_window; }
    Window* window_at(size_t index) const { return (index < m_window_count) ? m_windows[index] : nullptr; }
    void set_active_window(Window* win);

private:
    void process_native_events();

    static Application* s_instance;
    bool                m_running{false};
    int                 m_exit_code{0};

    Window*             m_windows[MAX_WINDOWS]{nullptr};
    size_t              m_window_count{0};
    Window*             m_active_window{nullptr};
};

} // namespace bos

#endif // BOS_UI_APPLICATION_HPP
