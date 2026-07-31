#include "framework/include/bos_ui_controls.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_Button* BOS_Button_Create(const char* text, void (*on_click)(BOS_UIElement*)) {
    BOS_Button* btn = (BOS_Button*)kmalloc(sizeof(BOS_Button));
    if (!btn) return NULL;

    memset(btn, 0, sizeof(BOS_Button));
    BOS_UIElement_Init(&btn->base, "Button");
    if (text) {
        strncpy(btn->text, text, 127);
        btn->text[127] = '\0';
    }
    btn->on_click = on_click;
    btn->base.min_size.width = 80;
    btn->base.min_size.height = 30;
    return btn;
}

BOS_CheckBox* BOS_CheckBox_Create(const char* text, void (*on_toggle)(BOS_UIElement*, bool)) {
    BOS_CheckBox* chk = (BOS_CheckBox*)kmalloc(sizeof(BOS_CheckBox));
    if (!chk) return NULL;

    memset(chk, 0, sizeof(BOS_CheckBox));
    BOS_UIElement_Init(&chk->base, "CheckBox");
    if (text) {
        strncpy(chk->text, text, 127);
        chk->text[127] = '\0';
    }
    chk->on_toggle = on_toggle;
    chk->base.min_size.width = 100;
    chk->base.min_size.height = 24;
    return chk;
}
