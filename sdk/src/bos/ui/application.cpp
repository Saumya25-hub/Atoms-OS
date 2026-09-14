#include "bos/application.hpp"
#include "bos/animation.hpp"
#include <string.h>

#define SYS_YIELD          3ULL
#define SYS_GUI_POLL_EVENT 22ULL

typedef struct {
    uint32_t abi_version;
    uint32_t type;
    uint32_t window_id;
    int32_t  mouse_x;
    int32_t  mouse_y;
    uint32_t mouse_btn;
    uint32_t key_code;
    uint32_t ascii_char;
    uint32_t modifiers;
    uint32_t reserved;
} RawBOSGUIEvent;

static inline int sys_call_poll_event(uint32_t win_id, RawBOSGUIEvent* out_event) {
    if (!out_event) return 0;
    out_event->abi_version = 1;
    int res = 0;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_POLL_EVENT), "D"(win_id), "S"(out_event), "d"((uint32_t)sizeof(RawBOSGUIEvent))
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline void sys_call_yield(void) {
    __asm__ volatile(
        "syscall"
        :
        : "a"(SYS_YIELD)
        : "rcx", "r11", "memory"
    );
}

namespace bos {

Application* Application::s_instance = nullptr;

Application::Application(int argc, char** argv) {
    (void)argc;
    (void)argv;
    s_instance = this;
}

Application::~Application() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool Application::register_window(Window* win) {
    if (!win || m_window_count >= MAX_WINDOWS) return false;

    for (size_t i = 0; i < m_window_count; ++i) {
        if (m_windows[i] == win) return true;
    }

    m_windows[m_window_count++] = win;
    return true;
}

void Application::unregister_window(Window* win) {
    if (!win) return;

    if (m_active_window == win) {
        m_active_window = nullptr;
    }

    for (size_t i = 0; i < m_window_count; ++i) {
        if (m_windows[i] == win) {
            for (size_t j = i; j < m_window_count - 1; ++j) {
                m_windows[j] = m_windows[j + 1];
            }
            m_windows[--m_window_count] = nullptr;
            return;
        }
    }
}

Window* Application::find_window(uint32_t window_id) {
    for (size_t i = 0; i < m_window_count; ++i) {
        if (m_windows[i] && m_windows[i]->native_id() == window_id) {
            return m_windows[i];
        }
    }
    return nullptr;
}

void Application::set_active_window(Window* win) {
    if (m_active_window == win) return;

    if (m_active_window && m_active_window != win) {
        m_active_window->set_active(false);
    }
    m_active_window = win;
    if (m_active_window) {
        m_active_window->set_active(true);
    }
}

int Application::run() {
    m_running = true;

    // Set initial active window if none set
    if (!m_active_window && m_window_count > 0) {
        set_active_window(m_windows[0]);
    }

    while (m_running && m_window_count > 0) {
        size_t events_processed = 0;

        // Poll events for each active window
        for (size_t i = 0; i < m_window_count; ++i) {
            Window* win = m_windows[i];
            if (!win || win->is_closed()) continue;

            RawBOSGUIEvent raw;
            memset(&raw, 0, sizeof(raw));
            raw.abi_version = 1;

            if (sys_call_poll_event(win->native_id(), &raw) && raw.type != 0) {
                events_processed++;

                Event ev;
                ev.window_id = raw.window_id;
                ev.mouse_pos = Point(raw.mouse_x, raw.mouse_y);
                ev.mouse_button = (MouseButton)raw.mouse_btn;
                ev.key_code = raw.key_code;
                ev.ascii_char = raw.ascii_char;
                ev.modifiers = raw.modifiers;

                switch (raw.type) {
                    case 1: // BOS_GUI_EVENT_CLICK
                    case 6: // BOS_GUI_EVENT_MOUSE_DOWN
                        ev.type = EventType::MouseDown;
                        set_active_window(win);
                        break;
                    case 7: // BOS_GUI_EVENT_MOUSE_UP
                        ev.type = EventType::MouseUp;
                        break;
                    case 5: // BOS_GUI_EVENT_MOUSE_MOVE
                        ev.type = EventType::MouseMove;
                        break;
                    case 3: // BOS_GUI_EVENT_KEY_DOWN
                        ev.type = EventType::KeyDown;
                        break;
                    case 4: // BOS_GUI_EVENT_KEY_UP
                        ev.type = EventType::KeyUp;
                        break;
                    case 2: // BOS_GUI_EVENT_CLOSE
                        ev.type = EventType::WindowClose;
                        break;
                    case 8: // BOS_GUI_EVENT_FOCUS_GAIN
                        ev.type = EventType::FocusGained;
                        set_active_window(win);
                        break;
                    case 9: // BOS_GUI_EVENT_FOCUS_LOST
                        ev.type = EventType::FocusLost;
                        if (m_active_window == win) {
                            m_active_window = nullptr;
                        }
                        break;
                    default:
                        ev.type = EventType::None;
                        break;
                }

                if (ev.type != EventType::None) {
                    win->dispatch_event(ev);
                }
            }
        }

        // Advance non-blocking UI animations
        if (Animator::instance().has_active_animations()) {
            Animator::instance().update(16);
        }

        // If no events were available this cycle and no animations running, yield CPU to prevent spinning
        if (events_processed == 0 && !Animator::instance().has_active_animations()) {
            sys_call_yield();
        }
    }

    return m_exit_code;
}

void Application::quit(int exit_code) {
    m_running = false;
    m_exit_code = exit_code;
}

} // namespace bos
