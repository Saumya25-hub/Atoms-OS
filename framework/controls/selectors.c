#include "framework/include/bos_ui_controls.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_ListView* BOS_ListView_Create(void) {
    BOS_ListView* lv = (BOS_ListView*)kmalloc(sizeof(BOS_ListView));
    if (!lv) return NULL;

    memset(lv, 0, sizeof(BOS_ListView));
    BOS_UIElement_Init(&lv->base, "ListView");
    lv->selected_index = -1;
    lv->base.min_size.width = 150;
    lv->base.min_size.height = 100;
    return lv;
}

void BOS_ListView_AddItem(BOS_ListView* list, const char* item) {
    if (!list || !item || list->item_count >= 32) return;
    strncpy(list->items[list->item_count], item, 63);
    list->items[list->item_count][63] = '\0';
    list->item_count++;
    BOS_UIElement_InvalidateLayout(&list->base);
}

BOS_ProgressBar* BOS_ProgressBar_Create(int32_t min_val, int32_t max_val) {
    BOS_ProgressBar* pb = (BOS_ProgressBar*)kmalloc(sizeof(BOS_ProgressBar));
    if (!pb) return NULL;

    memset(pb, 0, sizeof(BOS_ProgressBar));
    BOS_UIElement_Init(&pb->base, "ProgressBar");
    pb->min_val = min_val;
    pb->max_val = max_val;
    pb->value = min_val;
    pb->base.min_size.width = 100;
    pb->base.min_size.height = 18;
    return pb;
}
