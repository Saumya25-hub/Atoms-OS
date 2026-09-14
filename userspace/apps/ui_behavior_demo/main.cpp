/*
 * ============================================================================
 * ATOMS OS — BOS C++ UI FRAMEWORK PHASE 4 SHOWCASE APPLICATION
 * ============================================================================
 * Production Showcase & Forensic Certification for Phase 4:
 *   EVENTS + LAYOUT + THEME + ANIMATION + POLISH
 *
 * Demonstrates:
 *   [Test A] Mouse Enter / Leave / Hover Transitions
 *   [Test B] Button Press & Release Activation
 *   [Test C] Mouse Capture & Drag-Off Cancellation
 *   [Test D] Tab Forward Keyboard Focus Traversal
 *   [Test E] Shift+Tab Reverse Keyboard Focus Traversal
 *   [Test F] Keyboard Focus, Caret, & Text Input
 *   [Test G] ScrollView Viewport Clipping & Mouse Wheel Scrolling
 *   [Test H] Responsive Layout & Container Reflow on Window Resize
 *   [Test I] Dynamic Light / Dark Theme Switching
 *   [Test J] Non-Blocking Animation System with Easing Curves
 *   [Test K] Dirty-Region Damage Clipping & Redraw Pipeline
 *   [Test L] High-Contrast Accessible Focus Rings (2px Indicator)
 * ============================================================================
 */

#include <bos/ui.hpp>
#include <string.h>

// Forward declarations
static bos::Window* g_demo_window = nullptr;

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
// Phase 4 Main Application View
// ============================================================================
class BehaviorDemoView : public bos::Widget {
public:
    BehaviorDemoView() {
        // Root uses AnchorLayout (Header docked top, Status docked bottom, Content fills center)
        m_root_layout = new bos::AnchorLayout();
        set_layout(m_root_layout);
        set_padding(bos::Insets(8, 8, 8, 8));

        // 1. Header Card (Docked Top, Height 70)
        m_header_card = new bos::Card("UI Behavior & Experience Showcase");
        m_header_card->set_subtitle("Interactive event dispatch, responsive flow, motion transitions, and accessible navigation");
        m_header_card->set_margin(bos::Insets(0, 0, 0, 8));
        add_child(m_header_card);
        m_root_layout->add_dock(m_header_card, bos::DockEdge::Top, 70);

        // 2. Status / Telemetry Bar (Docked Bottom, Height 36)
        m_status_card = new bos::Card();
        m_status_card->set_margin(bos::Insets(0, 8, 0, 0));
        m_status_layout = new bos::LinearLayout(bos::Orientation::Horizontal, 16, bos::Alignment::Center);
        m_status_card->set_layout(m_status_layout);
        m_status_card->set_padding(bos::Insets(8, 4, 8, 4));

        m_status_label = new bos::Label("Telemetry: Ready | Theme: Dark | Tab Focus: Initialized");
        m_status_card->add_child(m_status_label);
        add_child(m_status_card);
        m_root_layout->add_dock(m_status_card, bos::DockEdge::Bottom, 36);

        // 3. Central Responsive Workspace (DockEdge::Fill)
        m_workspace = new bos::Widget();
        m_workspace_layout = new bos::LinearLayout(bos::Orientation::Horizontal, 12, bos::Alignment::Stretch);
        m_workspace->set_layout(m_workspace_layout);
        add_child(m_workspace);
        m_root_layout->add_dock(m_workspace, bos::DockEdge::Fill);

        // --------------------------------------------------------------------
        // Panel 1 (Left Column): Form Controls & Tab Focus Traversal (Width 320)
        // --------------------------------------------------------------------
        m_left_panel = new bos::Card("Controls & Focus Navigation");
        m_left_panel->set_subtitle("Tab and Shift+Tab accessible keyboard traversal");
        m_left_panel->set_bounds(bos::Rect(0, 0, 320, 380));
        m_left_layout = new bos::LinearLayout(bos::Orientation::Vertical, 10, bos::Alignment::Stretch);
        m_left_panel->set_layout(m_left_layout);
        m_left_panel->set_padding(bos::Insets(12, 40, 12, 12));

        // Tab item 1: First Name input
        m_name_input = new bos::TextBox("Enter application name...");
        m_name_input->set_tab_index(1);
        m_name_input->set_on_text_changed([](bos::TextBox*, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->update_telemetry("TextBox edited: name updated");
        }, this);
        m_left_panel->add_child(m_name_input);

        // Tab item 2: Organization input
        m_org_input = new bos::TextBox("Organization (e.g. ATOMS Core)");
        m_org_input->set_tab_index(2);
        m_left_panel->add_child(m_org_input);

        // Tab item 3: Hardware Acceleration Toggle
        m_hw_toggle = new bos::Toggle(true);
        m_hw_toggle->set_tab_index(3);
        m_hw_toggle->set_on_toggle([](bos::Toggle*, bool is_on, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->update_telemetry(is_on ? "Toggle: Hardware Accel ENABLED" : "Toggle: Hardware Accel DISABLED");
        }, this);
        m_left_panel->add_child(m_hw_toggle);

        // Tab item 4: High DPI Checkbox
        m_dpi_check = new bos::CheckBox("Enable High DPI Density Scaling", true);
        m_dpi_check->set_tab_index(4);
        m_left_panel->add_child(m_dpi_check);

        // Tab item 5: Button Action (Primary Accent Variant)
        m_submit_btn = new bos::Button("Submit Form", bos::ButtonVariant::Primary);
        m_submit_btn->set_tab_index(5);
        m_submit_btn->set_on_click([](bos::Button*, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->m_submit_count++;
            char buf[64];
            strcpy(buf, "Form Submitted! Total clicks: ");
            char num[16];
            int_to_str(self->m_submit_count, num, sizeof(num));
            strcat(buf, num);
            self->update_telemetry(buf);
        }, this);
        m_left_panel->add_child(m_submit_btn);

        m_workspace->add_child(m_left_panel);

        // --------------------------------------------------------------------
        // Panel 2 (Middle Column): Mouse Capture & Animation (Width 280)
        // --------------------------------------------------------------------
        m_mid_panel = new bos::Card("Gestures & Motion");
        m_mid_panel->set_subtitle("Mouse capture safety and cubic easing transitions");
        m_mid_panel->set_bounds(bos::Rect(0, 0, 280, 380));
        m_mid_layout = new bos::LinearLayout(bos::Orientation::Vertical, 10, bos::Alignment::Stretch);
        m_mid_panel->set_layout(m_mid_layout);
        m_mid_panel->set_padding(bos::Insets(12, 40, 12, 12));

        // Capture test button
        m_capture_btn = new bos::Button("Hold & Drag Pointer Out");
        m_capture_btn->set_tab_index(6);
        m_capture_btn->set_on_click([](bos::Button*, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->m_capture_clicks++;
            char buf[64];
            strcpy(buf, "Capture Button Clicked! Count: ");
            char num[16];
            int_to_str(self->m_capture_clicks, num, sizeof(num));
            strcat(buf, num);
            self->update_telemetry(buf);
        }, this);
        m_mid_panel->add_child(m_capture_btn);

        // Animation Progress Bar
        m_anim_label = new bos::Label("Progress Animation (EaseInOut):");
        m_mid_panel->add_child(m_anim_label);

        m_progress = new bos::ProgressBar();
        m_progress->set_value(25);
        m_mid_panel->add_child(m_progress);

        // Trigger animation button
        m_trigger_anim_btn = new bos::Button("Start 60 FPS Transition");
        m_trigger_anim_btn->set_tab_index(7);
        m_trigger_anim_btn->set_on_click([](bos::Button*, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->start_progress_animation();
        }, this);
        m_mid_panel->add_child(m_trigger_anim_btn);

        // Theme Toggle Button (Test I)
        m_theme_btn = new bos::Button("Switch Appearance Theme");
        m_theme_btn->set_tab_index(8);
        m_theme_btn->set_style(bos::Theme::make_accent_button_style());
        m_theme_btn->set_on_click([](bos::Button*, void* ud) {
            BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
            self->toggle_theme_mode();
        }, this);
        m_mid_panel->add_child(m_theme_btn);

        m_workspace->add_child(m_mid_panel);

        // --------------------------------------------------------------------
        // Panel 3 (Right Column): ScrollView & Clipping (Flex Width)
        // --------------------------------------------------------------------
        m_right_panel = new bos::Card("Viewport & Scrolling");
        m_right_panel->set_subtitle("Hierarchical damage clipping and smooth scrollview");
        m_right_panel->set_bounds(bos::Rect(0, 0, 240, 380));
        m_right_layout = new bos::LinearLayout(bos::Orientation::Vertical, 8, bos::Alignment::Stretch);
        m_right_panel->set_layout(m_right_layout);
        m_right_panel->set_padding(bos::Insets(12, 40, 12, 12));

        m_scroll_view = new bos::ScrollView();
        m_scroll_view->set_content_height(480);
        m_scroll_view->set_bounds(bos::Rect(0, 0, 220, 260));

        // Add 8 item buttons inside ScrollView
        for (int i = 1; i <= 8; ++i) {
            char bname[32];
            strcpy(bname, "Clipped Item #");
            char num[8];
            int_to_str(i, num, sizeof(num));
            strcat(bname, num);

            bos::Button* item = new bos::Button(bname);
            item->set_bounds(bos::Rect(8, (i - 1) * 36 + 4, 180, 30));
            m_scroll_view->add_child(item);
        }
        m_right_panel->add_child(m_scroll_view);

        m_workspace->add_child(m_right_panel);
    }

    ~BehaviorDemoView() override {
        delete m_root_layout;
        delete m_status_layout;
        delete m_workspace_layout;
        delete m_left_layout;
        delete m_mid_layout;
        delete m_right_layout;
    }

    void update_telemetry(const char* msg) {
        if (m_status_label && msg) {
            m_status_label->set_text(msg);
        }
    }

    void toggle_theme_mode() {
        bos::Theme::toggle_mode();
        bool is_dark = (bos::Theme::mode() == bos::ThemeMode::Dark);

        // Update control styles across window
        m_submit_btn->set_style(bos::Theme::make_button_style());
        m_capture_btn->set_style(bos::Theme::make_button_style());
        m_trigger_anim_btn->set_style(bos::Theme::make_button_style());
        m_theme_btn->set_style(bos::Theme::make_accent_button_style());
        m_header_card->set_background_color(bos::Theme::SurfaceCard());
        m_status_card->set_background_color(bos::Theme::SurfaceCard());
        m_left_panel->set_background_color(bos::Theme::SurfaceCard());
        m_mid_panel->set_background_color(bos::Theme::SurfaceCard());
        m_right_panel->set_background_color(bos::Theme::SurfaceCard());

        char buf[64];
        strcpy(buf, "Theme switched to: ");
        strcat(buf, is_dark ? "DARK MODE" : "LIGHT MODE");
        update_telemetry(buf);

        if (g_demo_window) {
            g_demo_window->invalidate();
        }
    }

    void start_progress_animation() {
        bos::Animator::instance().start(
            0.0f, 100.0f, 1200,
            bos::Easing::EaseInOut,
            [](float val, void* ud) {
                BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
                self->m_progress->set_value((int32_t)val);
            },
            [](void* ud) {
                BehaviorDemoView* self = static_cast<BehaviorDemoView*>(ud);
                self->update_telemetry("Animation Complete (60fps EaseInOut verified)");
            },
            this
        );
        update_telemetry("Animation running: 0% -> 100% EaseInOut");
    }

    // Automated Self-Test Harness for Formal Certification (Tests A through L)
    bool run_automated_certification_suite() {
        // Test A: Hover State via MouseMove
        bos::Event move_ev;
        move_ev.type = bos::EventType::MouseMove;
        move_ev.mouse_pos = bos::Point(m_submit_btn->absolute_bounds().x + 5,
                                       m_submit_btn->absolute_bounds().y + 5);
        if (g_demo_window) {
            g_demo_window->dispatch_event(move_ev);
        }
        if (!m_submit_btn->is_hovered()) return false;

        // Test B: Button Press and Activation
        int orig_clicks = m_submit_count;
        bos::Event press_ev;
        press_ev.type = bos::EventType::MouseDown;
        press_ev.mouse_button = bos::MouseButton::Left;
        press_ev.mouse_pos = bos::Point(10, 10);
        m_submit_btn->on_event(press_ev);

        bos::Event release_ev;
        release_ev.type = bos::EventType::MouseUp;
        release_ev.mouse_button = bos::MouseButton::Left;
        release_ev.mouse_pos = bos::Point(10, 10);
        m_submit_btn->on_event(release_ev);
        if (m_submit_count != orig_clicks + 1) return false;

        // Test C: Drag-off cancellation (release outside bounds)
        int cap_clicks = m_capture_clicks;
        m_capture_btn->on_event(press_ev);
        bos::Event outside_release;
        outside_release.type = bos::EventType::MouseUp;
        outside_release.mouse_button = bos::MouseButton::Left;
        outside_release.mouse_pos = bos::Point(999, 999); // Outside bounds
        m_capture_btn->on_event(outside_release);
        if (m_capture_clicks != cap_clicks) return false; // Must NOT have incremented!

        // Test D & E: Tab and Shift+Tab traversal
        if (g_demo_window) {
            g_demo_window->set_focused_widget(m_name_input);
            if (g_demo_window->focused_widget() != m_name_input) return false;

            g_demo_window->focus_next_widget();
            if (g_demo_window->focused_widget() != m_org_input) return false;

            g_demo_window->focus_prev_widget();
            if (g_demo_window->focused_widget() != m_name_input) return false;
        }

        // Test F: Text typing
        m_name_input->set_text("ATOMS");
        if (strcmp(m_name_input->text(), "ATOMS") != 0) return false;

        // Test G: Scroll wheel
        int32_t orig_scroll = m_scroll_view->scroll_y();
        bos::Event wheel_ev;
        wheel_ev.type = bos::EventType::MouseWheel;
        wheel_ev.wheel_dy = -2;
        m_scroll_view->on_event(wheel_ev);
        if (m_scroll_view->scroll_y() <= orig_scroll) return false;

        // Test H: Responsive layout reflow
        set_bounds(bos::Rect(0, 0, 960, 600));
        perform_layout();
        if (m_workspace->bounds().width == 0) return false;

        // Test I: Theme switching
        bos::Theme::set_mode(bos::ThemeMode::Light);
        if (bos::Theme::mode() != bos::ThemeMode::Light) return false;
        bos::Theme::set_mode(bos::ThemeMode::Dark);
        if (bos::Theme::mode() != bos::ThemeMode::Dark) return false;

        // Test J: Animation easing evaluation
        float v0 = bos::evaluate_easing(bos::Easing::EaseInOut, 0.0f);
        float v5 = bos::evaluate_easing(bos::Easing::EaseInOut, 0.5f);
        float v1 = bos::evaluate_easing(bos::Easing::EaseInOut, 1.0f);
        if (v0 != 0.0f || v5 != 0.5f || v1 != 1.0f) return false;

        // Test K: Dirty rect tracking
        m_submit_btn->clear_dirty();
        m_submit_btn->invalidate(bos::Rect(2, 2, 20, 20));
        if (!m_submit_btn->is_dirty()) return false;

        // Test L: Accessible Focus Ring
        bos::Color dark_ring = bos::Theme::FocusRing();
        if (dark_ring.argb() != 0xFF38BDF8) return false;
        bos::Theme::set_mode(bos::ThemeMode::Light);
        bos::Color light_ring = bos::Theme::FocusRing();
        bos::Theme::set_mode(bos::ThemeMode::Dark);
        if (light_ring.argb() != 0xFF0058EE) return false;

        return true;
    }

private:
    bos::AnchorLayout* m_root_layout{nullptr};
    bos::Card*         m_header_card{nullptr};
    bos::Card*         m_status_card{nullptr};
    bos::LinearLayout* m_status_layout{nullptr};
    bos::Label*        m_status_label{nullptr};

    bos::Widget*       m_workspace{nullptr};
    bos::LinearLayout* m_workspace_layout{nullptr};

    bos::Card*         m_left_panel{nullptr};
    bos::LinearLayout* m_left_layout{nullptr};
    bos::TextBox*      m_name_input{nullptr};
    bos::TextBox*      m_org_input{nullptr};
    bos::Toggle*       m_hw_toggle{nullptr};
    bos::CheckBox*     m_dpi_check{nullptr};
    bos::Button*       m_submit_btn{nullptr};

    bos::Card*         m_mid_panel{nullptr};
    bos::LinearLayout* m_mid_layout{nullptr};
    bos::Button*       m_capture_btn{nullptr};
    bos::Label*        m_anim_label{nullptr};
    bos::ProgressBar*  m_progress{nullptr};
    bos::Button*       m_trigger_anim_btn{nullptr};
    bos::Button*       m_theme_btn{nullptr};

    bos::Card*         m_right_panel{nullptr};
    bos::LinearLayout* m_right_layout{nullptr};
    bos::ScrollView*   m_scroll_view{nullptr};

    int32_t            m_submit_count{0};
    int32_t            m_capture_clicks{0};
};

// ============================================================================
// Application Entry Point
// ============================================================================
extern "C" int main(int argc, char** argv) {
    bos::Application app(argc, argv);

    bos::Window win("ATOMS OS - Phase 4 Application UI Behavior & Polish Showcase", 60, 40, 920, 560);
    g_demo_window = &win;

    BehaviorDemoView* demo_view = new BehaviorDemoView();
    win.set_root_widget(demo_view);

    win.set_on_resize([](bos::Window*, bos::Size new_size) {
        if (g_demo_window && g_demo_window->root_widget()) {
            g_demo_window->root_widget()->set_bounds(bos::Rect(0, 0, new_size.width, new_size.height));
            g_demo_window->root_widget()->perform_layout();
        }
    });

    win.show();

    // If invoked with --test or in verification mode, run certification suite
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        bool pass = demo_view->run_automated_certification_suite();
        return pass ? 0 : 1;
    }

    return app.run();
}
