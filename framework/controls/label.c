#include "framework/include/bos_ui_controls.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_Label* BOS_Label_Create(const char* text) {
    BOS_Label* lbl = (BOS_Label*)kmalloc(sizeof(BOS_Label));
    if (!lbl) return NULL;

    memset(lbl, 0, sizeof(BOS_Label));
    BOS_UIElement_Init(&lbl->base, "Label");
    if (text) {
        strncpy(lbl->text, text, 255);
        lbl->text[255] = '\0';
    }
    lbl->base.min_size.width = 60;
    lbl->base.min_size.height = 20;
    return lbl;
}

BOS_TextBox* BOS_TextBox_Create(const char* placeholder) {
    BOS_TextBox* tb = (BOS_TextBox*)kmalloc(sizeof(BOS_TextBox));
    if (!tb) return NULL;

    memset(tb, 0, sizeof(BOS_TextBox));
    BOS_UIElement_Init(&tb->base, "TextBox");
    if (placeholder) {
        strncpy(tb->placeholder, placeholder, 127);
        tb->placeholder[127] = '\0';
    }
    tb->base.min_size.width = 120;
    tb->base.min_size.height = 28;
    return tb;
}
