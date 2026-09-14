/*
 * ============================================================================
 * ATOMS OS — BOS C++ UI FRAMEWORK PHASE 1 VERIFICATION DEMO
 * ============================================================================
 * Pure Ring 3 C++ application validating the production foundation:
 *  - bos::Application event loop and lifecycle
 *  - bos::Window RAII native window controller & surface mapping
 *  - bos::Surface 2D software rendering (shapes, lines, 8x16 bitmap font)
 *  - bos::Widget tree, relative coordinates, invalidation, and hit testing
 *  - Real interactive mouse (hover, click) and keyboard event dispatch
 *  - Clean resource shutdown
 * ============================================================================
 */

#include <bos/ui.hpp>
#include <stdio.h>
#include <string.h>

class DemoCardWidget : public bos::Widget {
public:
    DemoCardWidget() {
        set_focusable(true);
    }

    void paint(bos::Surface& surface) override {
        bos::Rect r = absolute_bounds();

        // 1. Background Slate Fill
        surface.fill_rect(r, bos::Color::Slate900());

        // 2. Top Header Bar
        bos::Rect header_rect(r.x + 10, r.y + 10, r.width - 20, 45);
        surface.fill_rect(header_rect, bos::Color::Slate800());
        surface.draw_rect(header_rect, bos::Color::Slate700(), 1);
        surface.draw_string(header_rect.x + 15, header_rect.y + 15,
                            "ATOMS OS :: BOS C++ UI FOUNDATION (PHASE 1)",
                            bos::Color::Blue500());

        // 3. Central Interactive Card
        bos::Rect card_rect(r.x + 10, r.y + 65, r.width - 20, 260);
        surface.fill_rect(card_rect, bos::Color::Slate800());
        surface.draw_rect(card_rect, is_focused() ? bos::Color::Blue500() : bos::Color::Slate700(), 2);

        surface.draw_string(card_rect.x + 20, card_rect.y + 20,
                            "Status: Framework Native Surface Active (32-bpp ARGB)",
                            bos::Color::Slate100());

        char stats_buf[128];
        snprintf(stats_buf, sizeof(stats_buf),
                 "Interactions: %u Clicks  |  Last Key: '%c' (Code: 0x%02X)",
                 m_clicks, (m_last_key >= 32 && m_last_key <= 126) ? (char)m_last_key : ' ', m_last_key);
        surface.draw_string(card_rect.x + 20, card_rect.y + 50, stats_buf, bos::Color::Emerald500());

        // 4. Interactive Click Button Area inside Card
        m_btn_rect = bos::Rect(card_rect.x + 20, card_rect.y + 90, 200, 40);
        bos::Color btn_bg = m_btn_pressed ? bos::Color::Blue600()
                          : (m_btn_hovered ? bos::Color::Blue500() : bos::Color::Slate700());
        surface.fill_rect(m_btn_rect, btn_bg);
        surface.draw_rect(m_btn_rect, bos::Color::Blue500(), 1);
        surface.draw_string(m_btn_rect.x + 35, m_btn_rect.y + 12,
                            "Click To Test Interaction",
                            bos::Color::White());

        // 5. Architecture Notes
        surface.draw_string(card_rect.x + 20, card_rect.y + 155,
                            "Architecture Guarantees:",
                            bos::Color::Slate400());
        surface.draw_string(card_rect.x + 20, card_rect.y + 180,
                            "- One Authoritative Window Server (Kernel BOSurface / BWE)",
                            bos::Color::Slate400());
        surface.draw_string(card_rect.x + 20, card_rect.y + 205,
                            "- Frozen Syscalls 16-23 C ABI Boundary (Zero Compositor Duplication)",
                            bos::Color::Slate400());
        surface.draw_string(card_rect.x + 20, card_rect.y + 230,
                            "- RAII Lifetime, Freestanding C++20, Zero-Leak Teardown",
                            bos::Color::Slate400());

        // 6. Bottom Telemetry Bar
        bos::Rect footer_rect(r.x + 10, r.bottom() - 40, r.width - 20, 30);
        surface.fill_rect(footer_rect, bos::Color::Slate800());
        surface.draw_rect(footer_rect, bos::Color::Slate700(), 1);
        surface.draw_string(footer_rect.x + 15, footer_rect.y + 8,
                            "Ready for Phase 2: Controls + PNG Visual System",
                            bos::Color::Amber500());

        // Call base paint to render any child widgets
        Widget::paint(surface);
    }

    bool on_event(const bos::Event& event) override {
        bos::Point window_pt = local_to_window(event.mouse_pos);

        if (event.type == bos::EventType::MouseMove) {
            bool now_hovered = m_btn_rect.contains(window_pt);
            if (now_hovered != m_btn_hovered) {
                m_btn_hovered = now_hovered;
                invalidate();
            }
            return true;
        }

        if (event.type == bos::EventType::MouseDown) {
            if (m_btn_rect.contains(window_pt)) {
                m_btn_pressed = true;
                m_clicks++;
                invalidate();
                return true;
            }
        }

        if (event.type == bos::EventType::MouseUp) {
            if (m_btn_pressed) {
                m_btn_pressed = false;
                invalidate();
                return true;
            }
        }

        if (event.type == bos::EventType::KeyDown) {
            m_last_key = event.ascii_char ? event.ascii_char : event.key_code;
            invalidate();
            return true;
        }

        return false;
    }

private:
    bos::Rect m_btn_rect{0, 0, 0, 0};
    bool      m_btn_hovered{false};
    bool      m_btn_pressed{false};
    uint32_t  m_clicks{0};
    uint32_t  m_last_key{' '};
};

extern "C" int main(int argc, char** argv) {
    bos::Application app(argc, argv);

    bos::Window window("ATOMS OS — C++ UI Framework Phase 1 Demo", 100, 80, 720, 420);
    if (!window.is_valid()) {
        return -1;
    }

    DemoCardWidget demo_card;
    window.set_root_widget(&demo_card);

    window.set_on_close([](bos::Window* win) {
        (void)win;
        if (bos::Application::instance()) {
            bos::Application::instance()->quit(0);
        }
    });

    window.show();

    return app.run();
}
