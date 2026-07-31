#include "include/bos_sdk_runtime.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/ui/bosfsr32/include/bosfsr32.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);

typedef struct {
    BWE_Window* win;
    BOSFSR_Control* root_panel;
} BOS_AppWindowContext;

static BOS_AppWindowContext g_active_app_ctx;

static void sdk_app_render_callback(BWE_Window *self) {
    if (g_active_app_ctx.root_panel) {
        bosfsr32_render_window(self, g_active_app_ctx.root_panel);
    }
}

BOS_WindowHandle BOS_SDK_CreateWindow(int x, int y, int width, int height, const char* title) {
    uint32_t win_id;
    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, x, y, width, height,
                                        BWE_WINDOW_MOVABLE | BWE_WINDOW_RESIZABLE,
                                        &win_id);
    if (err != BWE_SUCCESS) return NULL;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (win) {
        if (title) strncpy(win->title, title, sizeof(win->title) - 1);
        win->on_render = sdk_app_render_callback;
    }

    g_active_app_ctx.win = win;
    g_active_app_ctx.root_panel = bosfsr_create_panel("RootPanel", 0, 0, width, height, 0xFF1E1E23);

    display_print("[BOS SDK RUNTIME] Window & BOSFSR32 Root Surface Created: ");
    display_print(title ? title : "Window");
    display_print("\n");

    return (BOS_WindowHandle)win;
}

BOS_ControlHandle BOS_SDK_CreateButton(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text) {
    BOSFSR_Control* btn = bosfsr_create_button("Button", x, y, width, height, text, 0xFF0078D7);
    if (g_active_app_ctx.root_panel && btn) {
        bosfsr_add_child(g_active_app_ctx.root_panel, btn);
    }
    display_print("[BOS SDK RUNTIME] Button instanced in BOSFSR32.\n");
    return (BOS_ControlHandle)btn;
}

BOS_ControlHandle BOS_SDK_CreateLabel(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text) {
    BOSFSR_Control* lbl = bosfsr_create_label("Label", x, y, width, height, text, 0xFFFFFFFF);
    if (g_active_app_ctx.root_panel && lbl) {
        bosfsr_add_child(g_active_app_ctx.root_panel, lbl);
    }
    display_print("[BOS SDK RUNTIME] Label instanced in BOSFSR32.\n");
    return (BOS_ControlHandle)lbl;
}

BOS_ControlHandle BOS_SDK_CreateTextBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* placeholder) {
    BOSFSR_Control* txt = bosfsr_create_button("TextBox", x, y, width, height, placeholder ? placeholder : "Text...", 0xFF2D2D34);
    if (g_active_app_ctx.root_panel && txt) {
        bosfsr_add_child(g_active_app_ctx.root_panel, txt);
    }
    return (BOS_ControlHandle)txt;
}

BOS_ControlHandle BOS_SDK_CreateCheckBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text, bool is_checked) {
    BOSFSR_Control* chk = bosfsr_create_checkbox("CheckBox", x, y, width, height, text, is_checked);
    if (g_active_app_ctx.root_panel && chk) {
        bosfsr_add_child(g_active_app_ctx.root_panel, chk);
    }
    return (BOS_ControlHandle)chk;
}

BOS_ControlHandle BOS_SDK_CreateRadioButton(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text, bool is_checked) {
    BOSFSR_Control* rad = bosfsr_create_radiobutton("RadioButton", x, y, width, height, text, is_checked);
    if (g_active_app_ctx.root_panel && rad) {
        bosfsr_add_child(g_active_app_ctx.root_panel, rad);
    }
    return (BOS_ControlHandle)rad;
}

BOS_ControlHandle BOS_SDK_CreateComboBox(BOS_WindowHandle parent, int x, int y, int width, int height, const char* text) {
    BOSFSR_Control* cb = bosfsr_create_button("ComboBox", x, y, width, height, text ? text : "Select Option", 0xFF282830);
    if (g_active_app_ctx.root_panel && cb) {
        bosfsr_add_child(g_active_app_ctx.root_panel, cb);
    }
    return (BOS_ControlHandle)cb;
}

BOSFSR_Control* bosfsr_create_listview(const char* name, int x, int y, int w, int h) {
    return bosfsr_create_gridview(name, x, y, w, h);
}

BOS_ControlHandle BOS_SDK_CreateListView(BOS_WindowHandle parent, int x, int y, int width, int height) {
    BOSFSR_Control* lv = bosfsr_create_gridview("ListView", x, y, width, height);
    if (g_active_app_ctx.root_panel && lv) {
        bosfsr_add_child(g_active_app_ctx.root_panel, lv);
    }
    return (BOS_ControlHandle)lv;
}

BOS_ControlHandle BOS_SDK_CreateGridView(BOS_WindowHandle parent, int x, int y, int width, int height) {
    BOSFSR_Control* gv = bosfsr_create_gridview("GridView", x, y, width, height);
    if (g_active_app_ctx.root_panel && gv) {
        bosfsr_add_child(g_active_app_ctx.root_panel, gv);
    }
    return (BOS_ControlHandle)gv;
}

BOS_ControlHandle BOS_SDK_CreateProgressBar(BOS_WindowHandle parent, int x, int y, int width, int height, int value) {
    BOSFSR_Control* pb = bosfsr_create_progressbar("ProgressBar", x, y, width, height, value);
    if (g_active_app_ctx.root_panel && pb) {
        bosfsr_add_child(g_active_app_ctx.root_panel, pb);
    }
    return (BOS_ControlHandle)pb;
}

BOS_ControlHandle BOS_SDK_CreateImage(BOS_WindowHandle parent, int x, int y, int width, int height, const char* asset_path) {
    BOSFSR_Control* img = bosfsr_create_image("Image", x, y, width, height);
    if (g_active_app_ctx.root_panel && img) {
        bosfsr_add_child(g_active_app_ctx.root_panel, img);
    }
    return (BOS_ControlHandle)img;
}
