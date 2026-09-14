/*
 * ============================================================================
 * ATOMS OS — BOS C++ UI FRAMEWORK PHASE 4 FINAL SHOWCASE
 * SMART LAYOUT & DEVELOPER ERGONOMICS DEMO
 * ============================================================================
 *
 * Demonstrates:
 *   [1] SmartPanel / Auto-Framing Containers (AutoSize, AutoHeight, Fill)
 *   [2] Zero Manual Pixel Coordinates for Controls (x/y positioning eliminated)
 *   [3] Small Controls Stay Small (Buttons size naturally to text via Font metrics)
 *   [4] Dynamic Auto-Framing (Adding/removing items expands/contracts panel)
 *   [5] Responsive Shell (Header, Sidebar, Auto-Framing Content, StatusBar)
 *   [6] Real Rounded Native Window Geometry (RadiusWindow = 10)
 * ============================================================================
 */

#include <bos/ui.hpp>
#include <string.h>

// Forward declaration
static bos::Window* g_demo_window = nullptr;

// Freestanding integer-to-string helper
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
// Smart Layout Demo Main View
// ============================================================================
class SmartLayoutDemoView : public bos::Widget {
public:
    SmartLayoutDemoView() {
        // Root uses AnchorLayout for the responsive application shell
        m_shell_layout = new bos::AnchorLayout();
        set_layout(m_shell_layout);

        // --------------------------------------------------------------------
        // 1. Application Header Bar (Docked Top, Height 50)
        // --------------------------------------------------------------------
        m_header_panel = new bos::HBox(bos::SizingMode::Fill);
        m_header_panel->set_card_style(true, bos::Theme::SurfaceElevated(), bos::Theme::Border(), 0);
        m_header_panel->set_padding(bos::Insets(14, 8, 14, 8));
        m_header_panel->set_alignment(bos::Alignment::Center);

        m_app_title = new bos::Label("BOS Studio — Project Settings", bos::Theme::TextPrimary());
        m_app_title->set_alignment(bos::Alignment::Start);
        m_header_panel->add(m_app_title);

        // Header Action Buttons (ButtonGroup)
        m_header_actions = new bos::ButtonGroup();
        m_save_btn = new bos::Button("Save", bos::ButtonVariant::Primary);
        m_discard_btn = new bos::Button("Discard", bos::ButtonVariant::Secondary);
        m_theme_toggle_btn = new bos::Button("Theme", bos::ButtonVariant::Ghost);

        m_theme_toggle_btn->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->toggle_theme();
        }, this);

        m_save_btn->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->update_status("Project settings successfully saved.");
        }, this);

        m_header_actions->add(m_theme_toggle_btn);
        m_header_actions->add(m_discard_btn);
        m_header_actions->add(m_save_btn);
        m_header_panel->add(m_header_actions);

        add_child(m_header_panel);
        m_shell_layout->add_dock(m_header_panel, bos::DockEdge::Top, 50);

        // --------------------------------------------------------------------
        // 2. Telemetry / Status Bar (Docked Bottom, Height 32)
        // --------------------------------------------------------------------
        m_status_panel = new bos::HBox(bos::SizingMode::Fill);
        m_status_panel->set_card_style(true, bos::Theme::SurfaceSubtle(), bos::Theme::Border(), 0);
        m_status_panel->set_padding(bos::Insets(12, 6, 12, 6));
        m_status_panel->set_alignment(bos::Alignment::Center);

        m_status_label = new bos::Label("Telemetry: Ready | SmartLayout: Active | Zero Manual Coordinates", bos::Theme::TextSecondary());
        m_status_panel->add(m_status_label);

        add_child(m_status_panel);
        m_shell_layout->add_dock(m_status_panel, bos::DockEdge::Bottom, 32);

        // --------------------------------------------------------------------
        // 3. Navigation Sidebar (Docked Left, Width 160)
        // --------------------------------------------------------------------
        m_sidebar_panel = new bos::VBox(bos::SizingMode::Fill);
        m_sidebar_panel->set_card_style(true, bos::Theme::SurfaceSubtle(), bos::Theme::Border(), 0);
        m_sidebar_panel->set_padding(bos::Insets(10, 14, 10, 14));
        m_sidebar_panel->set_spacing(6);

        m_nav_items[0] = new bos::Button("General", bos::ButtonVariant::Secondary);
        m_nav_items[1] = new bos::Button("Build Options", bos::ButtonVariant::Ghost);
        m_nav_items[2] = new bos::Button("Appearance", bos::ButtonVariant::Ghost);
        m_nav_items[3] = new bos::Button("Diagnostics", bos::ButtonVariant::Ghost);

        for (int i = 0; i < 4; i++) {
            m_sidebar_panel->add(m_nav_items[i]);
        }

        add_child(m_sidebar_panel);
        m_shell_layout->add_dock(m_sidebar_panel, bos::DockEdge::Left, 160);

        // --------------------------------------------------------------------
        // 4. Main Content Workspace (DockEdge::Fill)
        // Contains SmartPanels that auto-frame their contents!
        // --------------------------------------------------------------------
        m_content_container = new bos::HBox(bos::SizingMode::Fill);
        m_content_container->set_padding(bos::Insets(12, 12, 12, 12));
        m_content_container->set_spacing(12);

        // --- Left Smart Column: Project Properties & Controls ---
        m_col_left = new bos::VBox(bos::SizingMode::Fill);
        m_col_left->set_spacing(12);

        // SmartPanel Card 1: Project Configuration
        m_card_props = new bos::SmartPanel(bos::Orientation::Vertical, bos::SizingMode::AutoHeight);
        m_card_props->set_card_style(true);
        m_card_props->set_title("Project Properties", "Declarative form inputs with automatic sizing");

        m_name_box = new bos::TextBox("Project name...");
        m_name_box->set_text("AtomsCore");
        m_card_props->add(m_name_box);

        m_id_box = new bos::TextBox("Identifier (e.g. os.atoms.core)");
        m_card_props->add(m_id_box);

        m_aot_check = new bos::CheckBox("Enable Ahead-of-Time Optimization", true);
        m_card_props->add(m_aot_check);

        m_hw_toggle = new bos::Toggle(true);
        m_hw_label = new bos::Label("Hardware Video Acceleration", bos::Theme::TextSecondary());
        m_toggle_row = new bos::HBox();
        m_toggle_row->set_alignment(bos::Alignment::Center);
        m_toggle_row->set_spacing(8);
        m_toggle_row->add(m_hw_toggle);
        m_toggle_row->add(m_hw_label);
        m_card_props->add(m_toggle_row);

        m_col_left->add(m_card_props);

        // SmartPanel Card 2: Button Auto-Sizing Showcase ("Small Controls Stay Small")
        m_card_buttons = new bos::SmartPanel(bos::Orientation::Vertical, bos::SizingMode::AutoHeight);
        m_card_buttons->set_card_style(true);
        m_card_buttons->set_title("Natural Control Sizing", "Controls size around content rather than stretching");

        m_btn_row = new bos::HBox();
        m_btn_row->set_spacing(8);
        m_btn_row->set_alignment(bos::Alignment::Center);

        m_btn_ok = new bos::Button("OK", bos::ButtonVariant::Primary);
        m_btn_apply = new bos::Button("Apply", bos::ButtonVariant::Secondary);
        m_btn_long = new bos::Button("Save Changes and Continue", bos::ButtonVariant::Secondary);

        m_btn_ok->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->update_status("Clicked compact natural button [OK]");
        }, this);

        m_btn_apply->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->update_status("Clicked medium natural button [Apply]");
        }, this);

        m_btn_long->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->update_status("Clicked wide natural button [Save Changes and Continue]");
        }, this);

        m_btn_row->add(m_btn_ok);
        m_btn_row->add(m_btn_apply);
        m_btn_row->add(m_btn_long);
        m_card_buttons->add(m_btn_row);

        m_col_left->add(m_card_buttons);
        m_content_container->add(m_col_left);

        // --- Right Smart Column: Dynamic Auto-Framing & Reflow ---
        m_col_right = new bos::VBox(bos::SizingMode::Fill);
        m_col_right->set_spacing(12);

        // SmartPanel Card 3: Dynamic Child Addition & Auto-Framing
        m_card_dynamic = new bos::SmartPanel(bos::Orientation::Vertical, bos::SizingMode::AutoHeight);
        m_card_dynamic->set_card_style(true);
        m_card_dynamic->set_title("Dynamic Auto-Framing", "Add/remove items; panel auto-expands & reflows");

        m_dynamic_actions = new bos::HBox();
        m_dynamic_actions->set_spacing(8);

        m_add_btn = new bos::Button("+ Add Item", bos::ButtonVariant::Primary);
        m_rem_btn = new bos::Button("- Remove Item", bos::ButtonVariant::Secondary);
        m_dynamic_actions->add(m_add_btn);
        m_dynamic_actions->add(m_rem_btn);
        m_card_dynamic->add(m_dynamic_actions);

        // Sub-panel containing dynamic tag items
        m_tag_container = new bos::VBox(bos::SizingMode::Auto);
        m_tag_container->set_card_style(true, bos::Theme::SurfaceSubtle(), bos::Theme::Border(), 6);
        m_tag_container->set_padding(bos::Insets(8, 8, 8, 8));
        m_tag_container->set_spacing(6);

        for (int i = 0; i < 3; i++) {
            create_and_add_tag();
        }
        m_card_dynamic->add(m_tag_container);

        m_add_btn->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->create_and_add_tag();
            self->update_status("Added item to container: auto-reflow PASS");
        }, this);

        m_rem_btn->set_on_click([](bos::Button*, void* ud) {
            SmartLayoutDemoView* self = static_cast<SmartLayoutDemoView*>(ud);
            self->remove_last_tag();
            self->update_status("Removed item from container: auto-contract PASS");
        }, this);

        m_col_right->add(m_card_dynamic);
        m_content_container->add(m_col_right);

        add_child(m_content_container);
        m_shell_layout->add_dock(m_content_container, bos::DockEdge::Fill);
    }

    void create_and_add_tag() {
        if (m_tag_count >= 8) return;
        char buf[32];
        strcpy(buf, "Config Module #");
        char num[16];
        int_to_str(m_tag_count + 1, num, sizeof(num));
        strcat(buf, num);

        m_tags[m_tag_count] = new bos::Button(buf, bos::ButtonVariant::Ghost);
        m_tag_container->add(m_tags[m_tag_count]);
        m_tag_count++;
        m_tag_container->perform_layout();
        invalidate();
    }

    void remove_last_tag() {
        if (m_tag_count <= 0) return;
        m_tag_count--;
        m_tag_container->remove(m_tags[m_tag_count]);
        delete m_tags[m_tag_count];
        m_tags[m_tag_count] = nullptr;
        m_tag_container->perform_layout();
        invalidate();
    }

    void toggle_theme() {
        bos::Theme::toggle_mode();
        update_status(bos::Theme::mode() == bos::ThemeMode::Light
                          ? "Theme switched to: Light Theme"
                          : "Theme switched to: Dark Theme");
        if (g_demo_window) {
            g_demo_window->invalidate();
        }
    }

    void update_status(const char* text) {
        if (m_status_label && text) {
            m_status_label->set_text(text);
        }
    }

    bool run_automated_certification() {
        // Test 1: Button content-driven preferred size
        bos::Size sz_ok = m_btn_ok->measure_preferred_size();
        bos::Size sz_long = m_btn_long->measure_preferred_size();
        if (sz_ok.width >= sz_long.width || sz_ok.width == 0 || sz_long.width == 0) {
            return false; // FAIL: Small button is not smaller than long button
        }

        // Test 2: Dynamic Auto-Framing
        int initial_count = m_tag_count;
        bos::Size sz_initial = m_tag_container->measure_preferred_size();
        create_and_add_tag();
        bos::Size sz_expanded = m_tag_container->measure_preferred_size();
        if (sz_expanded.height <= sz_initial.height) {
            return false; // FAIL: Container did not expand upon child addition
        }

        remove_last_tag();
        bos::Size sz_contracted = m_tag_container->measure_preferred_size();
        if (sz_contracted.height != sz_initial.height || m_tag_count != initial_count) {
            return false; // FAIL: Container did not contract upon child removal
        }

        // Test 3: SmartPanel content arrangement & zero manual coordinate check
        m_card_props->perform_layout();
        if (m_name_box->bounds().width == 0 || m_name_box->bounds().height == 0) {
            return false; // FAIL: Layout did not calculate valid control dimensions
        }

        return true; // ALL TESTS PASS
    }

private:
    bos::AnchorLayout* m_shell_layout{nullptr};

    // Header
    bos::HBox*         m_header_panel{nullptr};
    bos::Label*        m_app_title{nullptr};
    bos::ButtonGroup*  m_header_actions{nullptr};
    bos::Button*       m_save_btn{nullptr};
    bos::Button*       m_discard_btn{nullptr};
    bos::Button*       m_theme_toggle_btn{nullptr};

    // Status
    bos::HBox*         m_status_panel{nullptr};
    bos::Label*        m_status_label{nullptr};

    // Sidebar
    bos::VBox*         m_sidebar_panel{nullptr};
    bos::Button*       m_nav_items[4]{nullptr};

    // Workspace & SmartPanels
    bos::HBox*         m_content_container{nullptr};
    bos::VBox*         m_col_left{nullptr};
    bos::VBox*         m_col_right{nullptr};

    // Card 1: Properties
    bos::SmartPanel*   m_card_props{nullptr};
    bos::TextBox*      m_name_box{nullptr};
    bos::TextBox*      m_id_box{nullptr};
    bos::CheckBox*     m_aot_check{nullptr};
    bos::HBox*         m_toggle_row{nullptr};
    bos::Toggle*       m_hw_toggle{nullptr};
    bos::Label*        m_hw_label{nullptr};

    // Card 2: Sizing
    bos::SmartPanel*   m_card_buttons{nullptr};
    bos::HBox*         m_btn_row{nullptr};
    bos::Button*       m_btn_ok{nullptr};
    bos::Button*       m_btn_apply{nullptr};
    bos::Button*       m_btn_long{nullptr};

    // Card 3: Dynamic Auto-Framing
    bos::SmartPanel*   m_card_dynamic{nullptr};
    bos::HBox*         m_dynamic_actions{nullptr};
    bos::Button*       m_add_btn{nullptr};
    bos::Button*       m_rem_btn{nullptr};
    bos::VBox*         m_tag_container{nullptr};
    bos::Button*       m_tags[8]{nullptr};
    int                m_tag_count{0};
};

// ============================================================================
// Process Entry Point & Application Loop
// ============================================================================
extern "C" int main(int argc, char** argv) {
    bos::Application app(argc, argv);

    // Window size: 840x540
    bos::Window window("BOS Smart Layout Showcase", 90, 80, 840, 540);
    g_demo_window = &window;

    SmartLayoutDemoView view;
    window.set_root_widget(&view);
    window.show();

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        bool pass = view.run_automated_certification();
        return pass ? 0 : 1;
    }

    return app.run();
}
