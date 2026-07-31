#include "sdk/include/bosui/bosui.hpp"
#include "platform/include/bos_platform.h"

static void OnButtonClicked(BOS_UIElement* sender) {
    (void)sender;
    BOS_Platform_Log("DEMO", "C++ SDK Button Click Event Received!");
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    bosui::Application app("ButtonDemo", "1.0.0");
    bosui::Window window("BOS C++ SDK GUI Demo", 640, 480);

    bosui::Grid grid;
    grid.AddRow(40, BOS_GRID_UNIT_PIXEL);
    grid.AddRow(40, BOS_GRID_UNIT_PIXEL);

    bosui::Label lbl("Welcome to modern BOS C++ SDK Application Development!");
    bosui::Button btn("Click Me!", OnButtonClicked);

    grid.AddChild(&lbl, 0, 0);
    grid.AddChild(&btn, 1, 0);

    window.SetContent(grid.GetNativeElement());
    window.Show();

    return app.Run();
}
