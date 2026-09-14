#include "bos/window.hpp"
#include "bos/application.hpp"
#include "bos/theme.hpp"
#include <string.h>

#define SYS_GUI_CREATE_WINDOW   16ULL
#define SYS_GUI_DESTROY_WINDOW  17ULL
#define SYS_GUI_SHOW_WINDOW     18ULL
#define SYS_GUI_SET_BOUNDS      19ULL
#define SYS_GUI_MAP_SURFACE     20ULL
#define SYS_GUI_INVALIDATE      21ULL
#define SYS_GUI_GET_SCREEN_INFO 23ULL

static inline uint32_t sys_call_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title) {
    uint32_t id = 0;
    register uint64_t r10 __asm__("r10") = (uint64_t)h;
    register uint64_t r8  __asm__("r8")  = (uint64_t)flags;
    register uint64_t r9  __asm__("r9")  = (uint64_t)title;
    __asm__ volatile(
        "syscall"
        : "=a"(id)
        : "a"(SYS_GUI_CREATE_WINDOW), "D"(x), "S"(y), "d"(w), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return id;
}

static inline int sys_call_destroy_window(uint32_t win_id) {
    int res = 0;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_DESTROY_WINDOW), "D"(win_id)
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline int sys_call_show_window(uint32_t win_id, bool visible) {
    int res = 0;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_SHOW_WINDOW), "D"(win_id), "S"((uint32_t)visible)
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline int sys_call_set_bounds(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    int res = 0;
    register uint64_t r10 __asm__("r10") = (uint64_t)w;
    register uint64_t r8  __asm__("r8")  = (uint64_t)h;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_SET_BOUNDS), "D"(win_id), "S"(x), "d"(y), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline int sys_call_map_surface(uint32_t win_id, uint32_t** out_surface_pixels, uint32_t* out_stride_bytes) {
    int res = 0;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_MAP_SURFACE), "D"(win_id), "S"(out_surface_pixels), "d"(out_stride_bytes)
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline int sys_call_get_screen_info(uint32_t* out_w, uint32_t* out_h, uint32_t* out_bpp) {
    int res = 0;
    __asm__ volatile(
        "syscall"
        : "=a"(res)
        : "a"(SYS_GUI_GET_SCREEN_INFO), "D"(out_w), "S"(out_h), "d"(out_bpp)
        : "rcx", "r11", "memory"
    );
    return res;
}

namespace bos {

Window::Window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height)
    : m_bounds(x, y, width, height),
      m_normal_bounds(x, y, width, height),
      m_visible(false),
      m_closed(false),
      m_active(false),
      m_state(WindowState::Normal),
      m_has_icon(false) {
    if (title) {
        strncpy(m_title, title, sizeof(m_title) - 1);
        m_title[sizeof(m_title) - 1] = '\0';
    } else {
        strcpy(m_title, "BOS Application");
    }

    m_window_id = sys_call_create_window(x, y, (int32_t)width, (int32_t)height, 0, m_title);
    if (m_window_id > 0) {
        map_native_surface();
    }

    if (Application::instance()) {
        Application::instance()->register_window(this);
    }
}

Window::~Window() {
    if (Application::instance()) {
        Application::instance()->unregister_window(this);
    }
    close();
}

Window::Window(Window&& other) noexcept
    : m_window_id(other.m_window_id),
      m_bounds(other.m_bounds),
      m_normal_bounds(other.m_normal_bounds),
      m_visible(other.m_visible),
      m_closed(other.m_closed),
      m_active(other.m_active),
      m_state(other.m_state),
      m_icon(static_cast<Image&&>(other.m_icon)),
      m_has_icon(other.m_has_icon),
      m_surface(static_cast<Surface&&>(other.m_surface)),
      m_root_widget(other.m_root_widget),
      m_hovered_widget(other.m_hovered_widget),
      m_focused_widget(other.m_focused_widget),
      m_on_close(other.m_on_close),
      m_on_resize(other.m_on_resize),
      m_on_activate(other.m_on_activate),
      m_on_deactivate(other.m_on_deactivate) {
    memcpy(m_title, other.m_title, sizeof(m_title));

    if (m_root_widget) {
        m_root_widget->set_parent(reinterpret_cast<Widget*>(this));
    }

    other.m_window_id = 0;
    other.m_visible = false;
    other.m_closed = true;
    other.m_active = false;
    other.m_state = WindowState::Closed;
    other.m_has_icon = false;
    other.m_root_widget = nullptr;
    other.m_hovered_widget = nullptr;
    other.m_focused_widget = nullptr;
    other.m_on_close = nullptr;
    other.m_on_resize = nullptr;
    other.m_on_activate = nullptr;
    other.m_on_deactivate = nullptr;

    if (Application::instance()) {
        Application::instance()->unregister_window(&other);
        Application::instance()->register_window(this);
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        close();

        m_window_id = other.m_window_id;
        m_bounds = other.m_bounds;
        m_normal_bounds = other.m_normal_bounds;
        m_visible = other.m_visible;
        m_closed = other.m_closed;
        m_active = other.m_active;
        m_state = other.m_state;
        m_icon = static_cast<Image&&>(other.m_icon);
        m_has_icon = other.m_has_icon;
        m_surface = static_cast<Surface&&>(other.m_surface);
        m_root_widget = other.m_root_widget;
        m_hovered_widget = other.m_hovered_widget;
        m_focused_widget = other.m_focused_widget;
        m_on_close = other.m_on_close;
        m_on_resize = other.m_on_resize;
        m_on_activate = other.m_on_activate;
        m_on_deactivate = other.m_on_deactivate;
        memcpy(m_title, other.m_title, sizeof(m_title));

        if (m_root_widget) {
            m_root_widget->set_parent(reinterpret_cast<Widget*>(this));
        }

        other.m_window_id = 0;
        other.m_visible = false;
        other.m_closed = true;
        other.m_active = false;
        other.m_state = WindowState::Closed;
        other.m_has_icon = false;
        other.m_root_widget = nullptr;
        other.m_hovered_widget = nullptr;
        other.m_focused_widget = nullptr;
        other.m_on_close = nullptr;
        other.m_on_resize = nullptr;
        other.m_on_activate = nullptr;
        other.m_on_deactivate = nullptr;

        if (Application::instance()) {
            Application::instance()->unregister_window(&other);
            Application::instance()->register_window(this);
        }
    }
    return *this;
}

void Window::map_native_surface() {
    if (m_window_id == 0) return;

    uint32_t* pixels = nullptr;
    uint32_t stride = 0;
    int res = sys_call_map_surface(m_window_id, &pixels, &stride);
    if (res == 0 && pixels) {
        m_surface = Surface(m_window_id, m_bounds.width, m_bounds.height, stride, pixels);
    }
}

void Window::destroy_native_window() {
    if (m_window_id != 0) {
        sys_call_destroy_window(m_window_id);
        m_window_id = 0;
    }
}

Rect Window::client_bounds() const {
    int32_t l = border_thickness();
    int32_t t = title_bar_height();
    int32_t w = (int32_t)m_bounds.width - (l + l);
    int32_t h = (int32_t)m_bounds.height - (t + l);
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    return Rect(l, t, (uint32_t)w, (uint32_t)h);
}

Size Window::client_size() const {
    Rect cb = client_bounds();
    return Size(cb.width, cb.height);
}

Point Window::window_to_client(const Point& pt) const {
    return Point(pt.x - border_thickness(), pt.y - title_bar_height());
}

Point Window::client_to_window(const Point& pt) const {
    return Point(pt.x + border_thickness(), pt.y + title_bar_height());
}

void Window::show() {
    if (m_window_id == 0 || m_closed) return;
    m_visible = true;
    render();
    sys_call_show_window(m_window_id, true);
}

void Window::hide() {
    if (m_window_id == 0 || m_closed) return;
    m_visible = false;
    sys_call_show_window(m_window_id, false);
}

void Window::close() {
    if (m_closed) return;
    m_closed = true;
    m_visible = false;
    m_active = false;
    m_state = WindowState::Closed;

    if (m_on_close) {
        m_on_close(this);
    }

    if (m_root_widget) {
        m_root_widget->set_parent(nullptr);
        m_root_widget = nullptr;
    }
    m_hovered_widget = nullptr;
    m_focused_widget = nullptr;

    destroy_native_window();
}

void Window::minimize() {
    if (m_window_id == 0 || m_closed) return;
    m_state = WindowState::Minimized;
    hide();
}

void Window::maximize() {
    if (m_window_id == 0 || m_closed || m_state == WindowState::Maximized) return;
    m_normal_bounds = m_bounds;
    uint32_t sw = 1920, sh = 1080, bpp = 32;
    sys_call_get_screen_info(&sw, &sh, &bpp);
    if (sw == 0) sw = 1920;
    if (sh == 0) sh = 1080;
    uint32_t work_h = (sh > 32) ? (sh - 32) : sh;
    m_state = WindowState::Maximized;
    set_bounds(0, 0, sw, work_h);
}

void Window::restore() {
    if (m_window_id == 0 || m_closed) return;
    if (m_state == WindowState::Minimized) {
        m_state = WindowState::Normal;
        show();
        activate();
    } else if (m_state == WindowState::Maximized) {
        m_state = WindowState::Normal;
        set_bounds(m_normal_bounds.x, m_normal_bounds.y, m_normal_bounds.width, m_normal_bounds.height);
    }
}

void Window::activate() {
    if (m_window_id == 0 || m_closed) return;
    set_active(true);
    sys_call_show_window(m_window_id, true);
}

void Window::set_active(bool active) {
    if (m_active == active) return;
    m_active = active;
    if (m_active) {
        if (m_state == WindowState::Inactive) m_state = WindowState::Normal;
        if (m_on_activate) m_on_activate(this);
    } else {
        if (m_state == WindowState::Normal) m_state = WindowState::Inactive;
        if (m_on_deactivate) m_on_deactivate(this);
    }
    render();
}

void Window::set_icon(const Image& icon) {
    m_icon = icon;
    m_has_icon = icon.is_valid();
}

void Window::set_bounds(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (m_window_id == 0 || m_closed) return;
    m_bounds = Rect(x, y, width, height);
    sys_call_set_bounds(m_window_id, x, y, (int32_t)width, (int32_t)height);

    map_native_surface();
    if (m_root_widget) {
        Size cs = client_size();
        m_root_widget->set_bounds(Rect(0, 0, cs.width, cs.height));
    }
    render();
}

void Window::set_size(uint32_t width, uint32_t height) {
    set_bounds(m_bounds.x, m_bounds.y, width, height);
}

void Window::set_position(int32_t x, int32_t y) {
    set_bounds(x, y, m_bounds.width, m_bounds.height);
}

void Window::set_title(const char* title) {
    if (!title) return;
    strncpy(m_title, title, sizeof(m_title) - 1);
    m_title[sizeof(m_title) - 1] = '\0';
}

void Window::set_root_widget(Widget* root) {
    if (m_root_widget == root) return;

    if (m_root_widget) {
        m_root_widget->set_parent(nullptr);
    }

    m_root_widget = root;
    if (m_root_widget) {
        m_root_widget->set_parent(reinterpret_cast<Widget*>(this));
        Size cs = client_size();
        m_root_widget->set_bounds(Rect(0, 0, cs.width, cs.height));
        m_root_widget->perform_layout();
    }
    render();
}

void Window::render() {
    if (!m_surface.is_valid() || !m_visible || m_closed) return;

    m_surface.reset_clip();
    m_surface.clear(Theme::Background());

    if (m_root_widget && m_root_widget->visible()) {
        m_root_widget->paint(m_surface);
    }

    m_surface.invalidate_all();
}

void Window::invalidate() {
    render();
}

void Window::invalidate(const Rect& dirty_rect) {
    if (!m_surface.is_valid() || !m_visible || m_closed || dirty_rect.is_empty()) return;

    m_surface.set_clip(dirty_rect);
    m_surface.fill_rect(dirty_rect, Theme::Background());

    if (m_root_widget && m_root_widget->visible()) {
        m_root_widget->paint(m_surface);
    }

    m_surface.invalidate(dirty_rect);
    m_surface.reset_clip();
}

static void collect_focusable_widgets(Widget* root, Widget** out_list, size_t& count, size_t max_count) {
    if (!root || !root->visible() || count >= max_count) return;

    if (root->focusable() && root->enabled()) {
        out_list[count++] = root;
    }

    for (size_t i = 0; i < root->child_count(); ++i) {
        collect_focusable_widgets(root->child_at(i), out_list, count, max_count);
    }
}

void Window::set_focused_widget(Widget* w) {
    if (m_focused_widget == w) return;
    if (m_focused_widget) {
        m_focused_widget->set_focused(false);
    }
    m_focused_widget = (w && w->focusable() && w->enabled()) ? w : nullptr;
    if (m_focused_widget) {
        m_focused_widget->set_focused(true);
    }
}

void Window::focus_next_widget() {
    if (!m_root_widget) return;
    Widget* focusables[64];
    size_t count = 0;
    collect_focusable_widgets(m_root_widget, focusables, count, 64);
    if (count == 0) return;

    for (size_t i = 0; i < count; ++i) {
        for (size_t j = 0; j + 1 < count; ++j) {
            if (focusables[j]->tab_index() > focusables[j + 1]->tab_index()) {
                Widget* tmp = focusables[j];
                focusables[j] = focusables[j + 1];
                focusables[j + 1] = tmp;
            }
        }
    }

    size_t cur_idx = count;
    for (size_t i = 0; i < count; ++i) {
        if (focusables[i] == m_focused_widget) {
            cur_idx = i;
            break;
        }
    }

    size_t next_idx = (cur_idx == count || cur_idx + 1 >= count) ? 0 : cur_idx + 1;
    set_focused_widget(focusables[next_idx]);
}

void Window::focus_prev_widget() {
    if (!m_root_widget) return;
    Widget* focusables[64];
    size_t count = 0;
    collect_focusable_widgets(m_root_widget, focusables, count, 64);
    if (count == 0) return;

    for (size_t i = 0; i < count; ++i) {
        for (size_t j = 0; j + 1 < count; ++j) {
            if (focusables[j]->tab_index() > focusables[j + 1]->tab_index()) {
                Widget* tmp = focusables[j];
                focusables[j] = focusables[j + 1];
                focusables[j + 1] = tmp;
            }
        }
    }

    size_t cur_idx = count;
    for (size_t i = 0; i < count; ++i) {
        if (focusables[i] == m_focused_widget) {
            cur_idx = i;
            break;
        }
    }

    size_t prev_idx = (cur_idx == count || cur_idx == 0) ? count - 1 : cur_idx - 1;
    set_focused_widget(focusables[prev_idx]);
}

void Window::dispatch_event(const Event& event) {
    if (m_closed) return;

    if (event.type == EventType::WindowClose) {
        close();
        return;
    }

    if (event.type == EventType::FocusGained) {
        set_active(true);
        return;
    }

    if (event.type == EventType::FocusLost) {
        set_active(false);
        return;
    }

    if (event.type == EventType::WindowResize) {
        uint32_t nw = (uint32_t)event.mouse_pos.x;
        uint32_t nh = (uint32_t)event.mouse_pos.y;
        if (nw > 0 && nh > 0) {
            m_bounds.width = nw;
            m_bounds.height = nh;
            map_native_surface();
            Size cs = client_size();
            if (m_root_widget) {
                m_root_widget->set_bounds(Rect(0, 0, cs.width, cs.height));
                m_root_widget->perform_layout();
            }
            if (m_on_resize) {
                m_on_resize(this, Size(nw, nh));
            }
            render();
        }
        return;
    }

    if (!m_root_widget || !m_root_widget->visible()) return;

    if (event.is_mouse_event()) {
        // Reject mouse interactions in the outside rounded corner dead-zone
        int32_t r = (int32_t)Theme::RadiusWindow();
        int32_t r2 = r * r;
        int32_t mx = event.mouse_pos.x;
        int32_t my = event.mouse_pos.y;
        int32_t w = (int32_t)m_bounds.width;
        int32_t h = (int32_t)m_bounds.height;

        bool outside_corner = false;
        if (mx < r && my < r && (r - mx) * (r - mx) + (r - my) * (r - my) > r2) outside_corner = true;
        else if (mx >= w - r && my < r && (mx - (w - r - 1)) * (mx - (w - r - 1)) + (r - my) * (r - my) > r2) outside_corner = true;
        else if (mx < r && my >= h - r && (r - mx) * (r - mx) + (my - (h - r - 1)) * (my - (h - r - 1)) > r2) outside_corner = true;
        else if (mx >= w - r && my >= h - r && (mx - (w - r - 1)) * (mx - (w - r - 1)) + (my - (h - r - 1)) * (my - (h - r - 1)) > r2) outside_corner = true;

        if (outside_corner) {
            if (m_hovered_widget) {
                m_hovered_widget->set_hovered(false);
                m_hovered_widget = nullptr;
            }
            return;
        }

        Point client_pos = window_to_client(event.mouse_pos);

        // Active mouse capture takes highest precedence
        if (m_captured_widget) {
            Event local_ev = event;
            local_ev.mouse_pos = m_captured_widget->window_to_local(client_pos);
            m_captured_widget->on_event(local_ev);

            if (event.type == EventType::MouseUp) {
                m_captured_widget = nullptr;
            }
            return;
        }

        int32_t border = border_thickness();
        int32_t title_h = title_bar_height();

        bool in_non_client = (event.mouse_pos.y < title_h) ||
                             (event.mouse_pos.x < border) ||
                             (event.mouse_pos.x >= (int32_t)m_bounds.width - border) ||
                             (event.mouse_pos.y >= (int32_t)m_bounds.height - border);

        if (in_non_client) {
            if (m_hovered_widget) {
                m_hovered_widget->set_hovered(false);
                m_hovered_widget = nullptr;
            }
            return;
        }

        Widget* hit = m_root_widget->hit_test(client_pos);

        if (hit != m_hovered_widget) {
            if (m_hovered_widget) {
                m_hovered_widget->set_hovered(false);
            }
            m_hovered_widget = hit;
            if (m_hovered_widget) {
                m_hovered_widget->set_hovered(true);
            }
        }

        if (event.type == EventType::MouseDown) {
            set_focused_widget(hit);
        }

        if (hit) {
            Event local_ev = event;
            local_ev.mouse_pos = hit->window_to_local(client_pos);
            hit->on_event(local_ev);
        }
    } else if (event.is_key_event()) {
        // Tab / Shift+Tab keyboard focus navigation
        if (event.type == EventType::KeyDown && (event.key_code == 9 || event.ascii_char == '\t')) {
            if (event.has_shift()) {
                focus_prev_widget();
            } else {
                focus_next_widget();
            }
            return;
        }

        Widget* target = m_focused_widget ? m_focused_widget : m_root_widget;
        if (target) {
            target->on_event(event);
        }
    }
}

} // namespace bos
