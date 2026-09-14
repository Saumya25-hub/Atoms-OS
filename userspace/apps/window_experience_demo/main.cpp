/*
 * ============================================================================
 * ATOMS OS — BOS C++ UI FRAMEWORK PHASE 3 SHOWCASE APPLICATION
 * ============================================================================
 * Production Showcase & Forensic Certification for Phase 3:
 *  - Genuine Native Desktop Window Experience
 *  - Native BWE Chrome & Titlebar Integration (30px titlebar + 5px outer borders)
 *  - Client Area vs Non-Client Area Isolation (zero widget overlap on chrome)
 *  - Active / Inactive Window Appearance & Focus Routing
 *  - Smooth Native Window Dragging & 8-Way Edge/Corner Resizing
 *  - Native Window State Model: Normal, Minimized, Maximized, Active, Inactive, Closed
 *  - Multi-Window Architecture (Main Controller + Secondary Diagnostics Inspector)
 *  - Phase 2 Modern Controls Integration: Card, Button, Label, TextBox, CheckBox, Toggle, ProgressBar, ListView
 *  - 100% Freestanding C++20 Ring 3 userspace on ATOMS OS
 * ============================================================================
 */

#include <bos/ui.hpp>
#include <string.h>

// Forward declarations
static bos::Window* g_main_window = nullptr;
static bos::Window* g_sec_window  = nullptr;

// Helper: Integer to string for freestanding runtime
static void int_to_str(int32_t val, char* buf, size_t max_len) {
    if (max_len == 0) return;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[32];
    size_t i = 0;
    bool neg = false;
    if (val < 0) {
        neg = true;
        val = -val;
    }
    while (val > 0 && i < sizeof(tmp) - 1) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    if (neg && i < sizeof(tmp) - 1) {
        tmp[i++] = '-';
    }
    size_t j = 0;
    while (i > 0 && j < max_len - 1) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

// ============================================================================
// Main Window Content View
// ============================================================================
class MainWindowView : public bos::Widget {
public:
    MainWindowView() {
        // 1. Header Card with Title & Description
        m_header_card = new bos::Card("ATOMS Desktop Controller (Primary Window)");
        m_header_card->set_subtitle("Native BOS chrome, dragging, 8-way resize, focus & state");
        m_header_card->set_position(16, 16);
        m_header_card->set_size(676, 72);
        add_child(m_header_card);

        // 2. Action Buttons Row
        m_btn_minimize = new bos::Button("Minimize");
        m_btn_minimize->set_position(16, 100);
        m_btn_minimize->set_size(100, 30);
        m_btn_minimize->set_on_click([](bos::Button*, void*) {
            if (g_main_window) g_main_window->minimize();
        });
        add_child(m_btn_minimize);

        m_btn_maximize = new bos::Button("Maximize");
        m_btn_maximize->set_position(122, 100);
        m_btn_maximize->set_size(100, 30);
        m_btn_maximize->set_on_click([](bos::Button*, void*) {
            if (g_main_window) g_main_window->maximize();
        });
        add_child(m_btn_maximize);

        m_btn_restore = new bos::Button("Restore");
        m_btn_restore->set_position(228, 100);
        m_btn_restore->set_size(100, 30);
        m_btn_restore->set_on_click([](bos::Button*, void*) {
            if (g_main_window) g_main_window->restore();
        });
        add_child(m_btn_restore);

        m_btn_focus_sec = new bos::Button("Focus Secondary");
        m_btn_focus_sec->set_position(16, 136);
        m_btn_focus_sec->set_size(150, 30);
        m_btn_focus_sec->set_on_click([](bos::Button*, void*) {
            if (g_sec_window && !g_sec_window->is_closed()) {
                g_sec_window->show();
                g_sec_window->activate();
            }
        });
        add_child(m_btn_focus_sec);

        m_btn_toggle_sec = new bos::Button("Toggle Secondary");
        m_btn_toggle_sec->set_position(172, 136);
        m_btn_toggle_sec->set_size(156, 30);
        m_btn_toggle_sec->set_on_click([](bos::Button*, void*) {
            if (g_sec_window && !g_sec_window->is_closed()) {
                if (g_sec_window->is_visible()) {
                    g_sec_window->hide();
                } else {
                    g_sec_window->show();
                    g_sec_window->activate();
                }
            }
        });
        add_child(m_btn_toggle_sec);

        // 3. Status Labels
        m_focus_label = new bos::Label("Focus: ACTIVE", bos::Color(0x38, 0xBD, 0xF8));
        m_focus_label->set_position(340, 100);
        m_focus_label->set_size(180, 22);
        add_child(m_focus_label);

        m_bounds_label = new bos::Label("Bounds: 720 x 480", bos::Color(0x94, 0xA3, 0xB8));
        m_bounds_label->set_position(340, 124);
        m_bounds_label->set_size(180, 22);
        add_child(m_bounds_label);

        m_client_label = new bos::Label("Client: 710 x 440", bos::Color(0x94, 0xA3, 0xB8));
        m_client_label->set_position(340, 148);
        m_client_label->set_size(180, 22);
        add_child(m_client_label);

        // 4. Form Controls Column
        m_checkbox = new bos::CheckBox("Hardware Composition Sync", true);
        m_checkbox->set_position(530, 100);
        m_checkbox->set_size(160, 24);
        add_child(m_checkbox);

        m_toggle = new bos::Toggle(true);
        m_toggle->set_position(530, 134);
        m_toggle->set_size(44, 22);
        add_child(m_toggle);

        m_toggle_label = new bos::Label("BWE Shadows", bos::Color::White());
        m_toggle_label->set_position(584, 134);
        m_toggle_label->set_size(100, 22);
        add_child(m_toggle_label);

        // 5. Progress Bar
        m_progress = new bos::ProgressBar(0, 100, 78);
        m_progress->set_position(16, 178);
        m_progress->set_size(676, 16);
        add_child(m_progress);

        // 6. ListView Event Log
        m_listview = new bos::ListView();
        m_listview->set_position(16, 206);
        m_listview->set_size(676, 210);
        m_listview->add_item("[INFO] Window initialized: Handle valid, Surface mapped to 0x50000000");
        m_listview->add_item("[BWE] Native chrome active: 30px titlebar + 5px outer border");
        m_listview->add_item("[SYS] Multi-window manager active. Secondary Window ready.");
        m_listview->add_item("[INPUT] 8-way resize zones (TL, TR, BL, BR, T, B, L, R) engaged.");
        m_listview->add_item("[OK] Ready for user interaction and hardware verification.");
        add_child(m_listview);
    }

    void paint(bos::Surface& surface) override {
        surface.fill_rect(bounds(), bos::Color(0x0F, 0x17, 0x2A)); // Slate 900
        Widget::paint(surface);
    }

    void update_status(bool active, const bos::Rect& bounds, const bos::Size& client) {
        if (active) {
            m_focus_label->set_text("Focus: ACTIVE");
            m_focus_label->set_color(bos::Color(0x38, 0xBD, 0xF8));
        } else {
            m_focus_label->set_text("Focus: INACTIVE");
            m_focus_label->set_color(bos::Color(0x64, 0x74, 0x8B));
        }

        char b_buf[64] = "Bounds: ";
        char num[16];
        int_to_str((int32_t)bounds.width, num, sizeof(num));
        strcat(b_buf, num);
        strcat(b_buf, " x ");
        int_to_str((int32_t)bounds.height, num, sizeof(num));
        strcat(b_buf, num);
        m_bounds_label->set_text(b_buf);

        char c_buf[64] = "Client: ";
        int_to_str((int32_t)client.width, num, sizeof(num));
        strcat(c_buf, num);
        strcat(c_buf, " x ");
        int_to_str((int32_t)client.height, num, sizeof(num));
        strcat(c_buf, num);
        m_client_label->set_text(c_buf);

        invalidate();
    }

    void add_log(const char* msg) {
        if (m_listview) {
            m_listview->add_item(msg);
            invalidate();
        }
    }

private:
    bos::Card*        m_header_card{nullptr};
    bos::Button*      m_btn_minimize{nullptr};
    bos::Button*      m_btn_maximize{nullptr};
    bos::Button*      m_btn_restore{nullptr};
    bos::Button*      m_btn_focus_sec{nullptr};
    bos::Button*      m_btn_toggle_sec{nullptr};

    bos::Label*       m_focus_label{nullptr};
    bos::Label*       m_bounds_label{nullptr};
    bos::Label*       m_client_label{nullptr};

    bos::CheckBox*    m_checkbox{nullptr};
    bos::Toggle*      m_toggle{nullptr};
    bos::Label*       m_toggle_label{nullptr};
    bos::ProgressBar* m_progress{nullptr};
    bos::ListView*    m_listview{nullptr};
};

// ============================================================================
// Secondary Window Content View (Diagnostics Inspector)
// ============================================================================
class SecWindowView : public bos::Widget {
public:
    SecWindowView() {
        // 1. Header Card
        m_card = new bos::Card("ATOMS Inspector (Secondary Window)");
        m_card->set_subtitle("Real-time dual-window focus & coordinate tracking");
        m_card->set_position(14, 14);
        m_card->set_size(430, 70);
        add_child(m_card);

        // 2. Action buttons
        m_btn_focus_main = new bos::Button("Activate Main");
        m_btn_focus_main->set_position(14, 94);
        m_btn_focus_main->set_size(135, 30);
        m_btn_focus_main->set_on_click([](bos::Button*, void*) {
            if (g_main_window) g_main_window->activate();
        });
        add_child(m_btn_focus_main);

        m_btn_min_sec = new bos::Button("Minimize Me");
        m_btn_min_sec->set_position(155, 94);
        m_btn_min_sec->set_size(135, 30);
        m_btn_min_sec->set_on_click([](bos::Button*, void*) {
            if (g_sec_window) g_sec_window->minimize();
        });
        add_child(m_btn_min_sec);

        m_btn_close_sec = new bos::Button("Close Me");
        m_btn_close_sec->set_position(296, 94);
        m_btn_close_sec->set_size(148, 30);
        m_btn_close_sec->set_on_click([](bos::Button*, void*) {
            if (g_sec_window) g_sec_window->close();
        });
        add_child(m_btn_close_sec);

        // 3. Status
        m_status_label = new bos::Label("Status: Running | Focus: INACTIVE", bos::Color(0x94, 0xA3, 0xB8));
        m_status_label->set_position(14, 132);
        m_status_label->set_size(430, 22);
        add_child(m_status_label);

        // 4. TextBox test
        m_textbox = new bos::TextBox("Live diagnostics notes...");
        m_textbox->set_position(14, 160);
        m_textbox->set_size(430, 32);
        add_child(m_textbox);

        // 5. Diagnostics Log
        m_log = new bos::ListView();
        m_log->set_position(14, 202);
        m_log->set_size(430, 110);
        m_log->add_item("[INSPECTOR] Secondary Window active.");
        m_log->add_item("[INSPECTOR] Z-order management verified.");
        m_log->add_item("[INSPECTOR] Ready for event tracing.");
        add_child(m_log);
    }

    void paint(bos::Surface& surface) override {
        surface.fill_rect(bounds(), bos::Color(0x0F, 0x17, 0x2A));
        Widget::paint(surface);
    }

    void update_focus(bool active) {
        if (active) {
            m_status_label->set_text("Status: Running | Focus: ACTIVE");
            m_status_label->set_color(bos::Color(0x10, 0xB9, 0x81));
        } else {
            m_status_label->set_text("Status: Running | Focus: INACTIVE");
            m_status_label->set_color(bos::Color(0x94, 0xA3, 0xB8));
        }
        invalidate();
    }

private:
    bos::Card*     m_card{nullptr};
    bos::Button*   m_btn_focus_main{nullptr};
    bos::Button*   m_btn_min_sec{nullptr};
    bos::Button*   m_btn_close_sec{nullptr};
    bos::Label*    m_status_label{nullptr};
    bos::TextBox*  m_textbox{nullptr};
    bos::ListView* m_log{nullptr};
};

// ============================================================================
// Application Entry Point
// ============================================================================
extern "C" int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    bos::Application app;

    // 1. Create Main Window (720 x 480)
    bos::Window main_win("ATOMS Desktop - Primary Controller", 60, 60, 720, 480);
    if (!main_win.is_valid()) {
        return 1;
    }
    g_main_window = &main_win;

    MainWindowView main_view;
    main_win.set_root_widget(&main_view);

    // 2. Create Secondary Window (460 x 340)
    bos::Window sec_win("ATOMS Inspector - System Diagnostics", 360, 120, 460, 340);
    if (!sec_win.is_valid()) {
        return 2;
    }
    g_sec_window = &sec_win;

    SecWindowView sec_view;
    sec_win.set_root_widget(&sec_view);

    // Register Callbacks on Main Window
    main_win.set_on_activate([](bos::Window* w) {
        MainWindowView* v = static_cast<MainWindowView*>(w->root_widget());
        if (v) {
            v->update_status(true, w->bounds(), w->client_size());
            v->add_log("[EVENT] Main Window: FOCUS_GAINED (Active Styling)");
        }
    });

    main_win.set_on_deactivate([](bos::Window* w) {
        MainWindowView* v = static_cast<MainWindowView*>(w->root_widget());
        if (v) {
            v->update_status(false, w->bounds(), w->client_size());
            v->add_log("[EVENT] Main Window: FOCUS_LOST (Inactive Styling)");
        }
    });

    main_win.set_on_resize([](bos::Window* w, bos::Size sz) {
        (void)sz;
        MainWindowView* v = static_cast<MainWindowView*>(w->root_widget());
        if (v) {
            v->update_status(w->is_active(), w->bounds(), w->client_size());
            v->add_log("[EVENT] Main Window: RESIZED by BWE");
        }
    });

    main_win.set_on_close([](bos::Window*) {
        if (g_sec_window && !g_sec_window->is_closed()) {
            g_sec_window->close();
        }
    });

    // Register Callbacks on Secondary Window
    sec_win.set_on_activate([](bos::Window* w) {
        SecWindowView* v = static_cast<SecWindowView*>(w->root_widget());
        if (v) v->update_focus(true);
    });

    sec_win.set_on_deactivate([](bos::Window* w) {
        SecWindowView* v = static_cast<SecWindowView*>(w->root_widget());
        if (v) v->update_focus(false);
    });

    // Show both windows
    main_win.show();
    sec_win.show();

    // Primary window starts active
    main_win.activate();

    return app.run();
}
