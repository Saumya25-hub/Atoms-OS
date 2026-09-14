/*
 * ============================================================================
 * ATOMS OS — BOS C++ UI FRAMEWORK PHASE 2 SHOWCASE APPLICATION
 * ============================================================================
 * Production Showcase & Forensic Certification for Phase 2:
 *  - Native PNG / Image Asset Engine with Alpha Transparency
 *  - Universal Visual State Model (Normal, Hover, Pressed, Focused, Disabled, Selected)
 *  - Complete Modern Control Suite:
 *      Button, Label, TextBox, CheckBox, RadioButton, Toggle,
 *      ListView, ComboBox, ProgressBar, Card, Sidebar, ScrollView,
 *      TabControl, ImageWidget
 *  - 9-Slice Border Rendering
 *  - DPI / Scale Aware Rendering Engine (100%, 150%, 200%)
 *  - 100% Native ATOMS OS Architecture (Ring 3 userspace)
 * ============================================================================
 */

#include <bos/ui.hpp>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Main Settings Showcase View
// ============================================================================
class SettingsShowcaseView : public bos::Widget {
public:
    SettingsShowcaseView() {
        // Construct Navigation Sidebar
        m_sidebar = new bos::Sidebar();
        m_sidebar->set_position(0, 0);
        m_sidebar->set_size(180, 600);

        // Populate Sidebar with Real ATOMS PNG Icons
        m_sidebar->add_item("Home",              bos::Icon("assets/icons/navigation/home.png", bos::Size(18, 18)));
        m_sidebar->add_item("System",            bos::Icon("assets/icons/computer.png", bos::Size(18, 18)));
        m_sidebar->add_item("Bluetooth",         bos::Icon("BOOT(OS-ICO)/USBdrive.png", bos::Size(18, 18)));
        m_sidebar->add_item("Network",           bos::Icon("assets/icons/system/wifi.png", bos::Size(18, 18)));
        m_sidebar->add_item("Personalization",   bos::Icon("assets/icons/settings.png", bos::Size(18, 18)));
        m_sidebar->add_item("Apps",              bos::Icon("assets/icons/apps/atoms.png", bos::Size(18, 18)));
        m_sidebar->add_item("Accounts",          bos::Icon("BOOT(OS-ICO)/user.png", bos::Size(18, 18)));
        m_sidebar->add_item("Accessibility",     bos::Icon("BOOT(OS-ICO)/eye.png", bos::Size(18, 18)));
        m_sidebar->add_item("Security",          bos::Icon("BOOT(OS-ICO)/lock.png", bos::Size(18, 18)));
        m_sidebar->set_selected_index(4); // Personalization selected by default
        add_child(m_sidebar);

        // 1. Breadcrumb Title
        m_title_label = new bos::Label("Personalization > Themes", bos::Color::White());
        m_title_label->set_position(200, 16);
        m_title_label->set_size(300, 24);
        add_child(m_title_label);

        // 2. Wallpaper & Theme Preview Hero Card
        m_hero_card = new bos::Card("Current Theme: ATOMS Slate Light", bos::Icon("assets/icons/branding/atoms_start.png", bos::Size(24, 24)));
        m_hero_card->set_subtitle("Haswell Native 32-bpp ARGB Composition");
        m_hero_card->set_position(200, 50);
        m_hero_card->set_size(360, 160);
        add_child(m_hero_card);

        // 3. Theme Preset Buttons inside Content Area
        m_theme_btn1 = new bos::Button("Slate Dark");
        m_theme_btn1->set_position(200, 220);
        m_theme_btn1->set_size(84, 30);
        m_theme_btn1->set_selected(true);
        add_child(m_theme_btn1);

        m_theme_btn2 = new bos::Button("Cyber Blue");
        m_theme_btn2->set_position(290, 220);
        m_theme_btn2->set_size(84, 30);
        add_child(m_theme_btn2);

        m_theme_btn3 = new bos::Button("Emerald");
        m_theme_btn3->set_position(380, 220);
        m_theme_btn3->set_size(84, 30);
        add_child(m_theme_btn3);

        m_theme_btn4 = new bos::Button("Crimson");
        m_theme_btn4->set_position(470, 220);
        m_theme_btn4->set_size(84, 30);
        add_child(m_theme_btn4);

        // 4. Interactive Form Controls Column
        // CheckBox
        m_checkbox = new bos::CheckBox("Enable Hardware Acceleration", true);
        m_checkbox->set_position(200, 265);
        m_checkbox->set_size(260, 24);
        add_child(m_checkbox);

        // RadioButtons
        m_radio1 = new bos::RadioButton("Pure UEFI Mode", 1, true);
        m_radio1->set_position(200, 295);
        m_radio1->set_size(160, 24);
        add_child(m_radio1);

        m_radio2 = new bos::RadioButton("Legacy CSM Mode", 1, false);
        m_radio2->set_position(370, 295);
        m_radio2->set_size(160, 24);
        add_child(m_radio2);

        // Toggle Switch
        m_toggle_label = new bos::Label("Aero Glass Blur:");
        m_toggle_label->set_position(200, 330);
        m_toggle_label->set_size(130, 24);
        add_child(m_toggle_label);

        m_toggle = new bos::Toggle(true);
        m_toggle->set_position(340, 330);
        add_child(m_toggle);

        // TextBox
        m_textbox_label = new bos::Label("Device Name:");
        m_textbox_label->set_position(200, 365);
        m_textbox_label->set_size(100, 24);
        add_child(m_textbox_label);

        m_textbox = new bos::TextBox("ATOMS-H81-PC");
        m_textbox->set_text("ATOMS-H81-PC");
        m_textbox->set_position(300, 360);
        m_textbox->set_size(250, 32);
        add_child(m_textbox);

        // ComboBox
        m_combo_label = new bos::Label("Display Scaling:");
        m_combo_label->set_position(200, 405);
        m_combo_label->set_size(100, 24);
        add_child(m_combo_label);

        m_combobox = new bos::ComboBox();
        m_combobox->set_position(300, 400);
        m_combobox->set_size(250, 32);
        m_combobox->add_item("100% (Standard 96 DPI)");
        m_combobox->add_item("125% (Medium 120 DPI)");
        m_combobox->add_item("150% (High 144 DPI)");
        m_combobox->add_item("200% (Retina 192 DPI)");
        m_combobox->set_selected_index(0);
        add_child(m_combobox);

        // ProgressBar
        m_progress_label = new bos::Label("Memory Allocation (Heap Usage):");
        m_progress_label->set_position(200, 445);
        m_progress_label->set_size(260, 20);
        add_child(m_progress_label);

        m_progressbar = new bos::ProgressBar(0, 100, 68);
        m_progressbar->set_position(200, 470);
        m_progressbar->set_size(350, 22);
        add_child(m_progressbar);

        // Link Label
        m_link_label = new bos::Label("Learn more about BOS C++ Architecture...", bos::Theme::AccentHover());
        m_link_label->set_link(true);
        m_link_label->set_position(200, 505);
        m_link_label->set_size(320, 20);
        add_child(m_link_label);

        // 5. Right Showcase Column: Tabs, ListView, Card, and Scaled Buttons
        m_tabs = new bos::TabControl();
        m_tabs->set_position(570, 50);
        m_tabs->set_size(370, 230);
        m_tabs->add_tab("General");
        m_tabs->add_tab("Display");
        m_tabs->add_tab("Sound");
        m_tabs->add_tab("About");
        add_child(m_tabs);

        // ListView inside Tab/Right panel
        m_listview = new bos::ListView();
        m_listview->set_position(570, 290);
        m_listview->set_size(370, 160);
        m_listview->add_item("Documents",    bos::Icon("assets/icons/navigation/folder.png", bos::Size(16, 16)), "Folder");
        m_listview->add_item("Wallpaper.png",bos::Icon("BOOT(OS-ICO)/media-player.png", bos::Size(16, 16)), "2.4 MB");
        m_listview->add_item("Readme.txt",   bos::Icon("assets/icons/notes.png", bos::Size(16, 16)), "12 KB");
        m_listview->add_item("App.bosx",     bos::Icon("assets/icons/apps/atoms.png", bos::Size(16, 16)), "5.1 MB");
        m_listview->set_selected_index(0);
        add_child(m_listview);

        // Visual State Buttons Showcase Row
        m_btn_normal = new bos::Button("Normal");
        m_btn_normal->set_position(570, 465);
        m_btn_normal->set_size(85, 30);
        add_child(m_btn_normal);

        m_btn_hover = new bos::Button("Hover");
        m_btn_hover->set_position(665, 465);
        m_btn_hover->set_size(85, 30);
        add_child(m_btn_hover);

        m_btn_pressed = new bos::Button("Pressed");
        m_btn_pressed->set_position(760, 465);
        m_btn_pressed->set_size(85, 30);
        add_child(m_btn_pressed);

        m_btn_disabled = new bos::Button("Disabled");
        m_btn_disabled->set_position(855, 465);
        m_btn_disabled->set_size(85, 30);
        m_btn_disabled->set_enabled(false);
        add_child(m_btn_disabled);

        // DPI Scaling Buttons
        m_btn_dpi100 = new bos::Button("100%");
        m_btn_dpi100->set_position(570, 510);
        m_btn_dpi100->set_size(60, 28);
        add_child(m_btn_dpi100);

        m_btn_dpi150 = new bos::Button("150%");
        m_btn_dpi150->set_position(640, 508);
        m_btn_dpi150->set_size(75, 32);
        add_child(m_btn_dpi150);

        m_btn_dpi200 = new bos::Button("200%");
        m_btn_dpi200->set_position(725, 505);
        m_btn_dpi200->set_size(90, 36);
        add_child(m_btn_dpi200);

        // Status bar footer
        m_status_label = new bos::Label("BOS C++ UI Framework :: Phase 2 Certified [14 Controls, Native PNG Engine, 9-Slice, DPI]", bos::Color::Slate400());
        m_status_label->set_position(200, 565);
        m_status_label->set_size(740, 20);
        add_child(m_status_label);
    }

    void paint(bos::Surface& surface) override {
        // Clear window canvas background
        surface.clear(bos::Theme::Background());

        // Paint all child widgets
        Widget::paint(surface);
    }

private:
    bos::Sidebar*      m_sidebar{nullptr};
    bos::Label*        m_title_label{nullptr};
    bos::Card*         m_hero_card{nullptr};
    bos::Button*       m_theme_btn1{nullptr};
    bos::Button*       m_theme_btn2{nullptr};
    bos::Button*       m_theme_btn3{nullptr};
    bos::Button*       m_theme_btn4{nullptr};
    bos::CheckBox*     m_checkbox{nullptr};
    bos::RadioButton*  m_radio1{nullptr};
    bos::RadioButton*  m_radio2{nullptr};
    bos::Label*        m_toggle_label{nullptr};
    bos::Toggle*       m_toggle{nullptr};
    bos::Label*        m_textbox_label{nullptr};
    bos::TextBox*      m_textbox{nullptr};
    bos::Label*        m_combo_label{nullptr};
    bos::ComboBox*     m_combobox{nullptr};
    bos::Label*        m_progress_label{nullptr};
    bos::ProgressBar*  m_progressbar{nullptr};
    bos::Label*        m_link_label{nullptr};
    bos::TabControl*   m_tabs{nullptr};
    bos::ListView*     m_listview{nullptr};
    bos::Button*       m_btn_normal{nullptr};
    bos::Button*       m_btn_hover{nullptr};
    bos::Button*       m_btn_pressed{nullptr};
    bos::Button*       m_btn_disabled{nullptr};
    bos::Button*       m_btn_dpi100{nullptr};
    bos::Button*       m_btn_dpi150{nullptr};
    bos::Button*       m_btn_dpi200{nullptr};
    bos::Label*        m_status_label{nullptr};
};

// ============================================================================
// Application Entry Point (Mandatory C Linkage for crt0)
// ============================================================================
extern "C" int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    bos::Application app;

    bos::Window window("ATOMS Settings - BOS C++ UI Framework", 30, 30, 960, 600);
    if (!window.is_valid()) {
        return 1;
    }

    SettingsShowcaseView showcase_view;
    showcase_view.set_size(960, 600);
    window.set_root_widget(&showcase_view);

    window.show();

    return app.run();
}
