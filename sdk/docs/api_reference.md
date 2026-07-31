# BOS OS Developer SDK — API Reference Manual

**Version:** 1.0.0-PHASE4  
**Target:** Third-Party Application Developers

---

## 1. Overview
The BOS Developer SDK provides C and modern C++ header libraries enabling application development without touching kernel code.

---

## 2. C API Reference (`#include <bos/bos.h>`)

### Application Lifecycle
- `BOS_Result BOS_SDK_Init(const char* app_name, const char* version)`
  - Initializes application context.
- `int BOS_SDK_Run(void)`
  - Starts the main event loop.
- `void BOS_SDK_Quit(int exit_code)`
  - Terminates event loop execution.

### Window Operations
- `BOS_Window* BOS_SDK_CreateWindow(int32_t x, int32_t y, uint32_t w, uint32_t h, const char* title)`
  - Creates a new window.
- `void BOS_SDK_ShowWindow(BOS_Window* win)`
  - Displays window on desktop.

---

## 3. C++ API Reference (`#include <bosui/bosui.hpp>`)

### `bosui::Application`
```cpp
bosui::Application app("AppName", "1.0.0");
int exit_code = app.Run();
```

### `bosui::Window`
```cpp
bosui::Window window("My App", 640, 480);
window.SetContent(element.GetNativeElement());
window.Show();
```

### `bosui::Button`, `bosui::Label`, `bosui::Grid`
```cpp
bosui::Grid grid;
bosui::Label lbl("Welcome!");
bosui::Button btn("Save", OnClick);
grid.AddChild(&lbl, 0, 0);
grid.AddChild(&btn, 1, 0);
```
