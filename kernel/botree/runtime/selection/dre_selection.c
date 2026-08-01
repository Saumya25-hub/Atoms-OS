#include "../include/dre_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BDeRuntime_Select(BDeRuntime* rt, int32_t index) {
    if (!rt || !rt->active || index < 0 || (uint32_t)index >= rt->item_count) return -1;
    rt->selected_mask[index] = true;
    rt->focused_index = index;
    return 0;
}

int32_t BDeRuntime_DeselectAll(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    memset(rt->selected_mask, 0, sizeof(rt->selected_mask));
    rt->focused_index = -1;
    return 0;
}

int32_t BDeRuntime_SelectAll(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    for (uint32_t i = 0; i < rt->item_count; i++) {
        rt->selected_mask[i] = true;
    }
    return 0;
}

uint32_t BDeRuntime_GetSelectedCount(BDeRuntime* rt) {
    if (!rt || !rt->active) return 0;
    uint32_t count = 0;
    for (uint32_t i = 0; i < rt->item_count; i++) {
        if (rt->selected_mask[i]) count++;
    }
    return count;
}

BDeTxHandle BDeRuntime_CopySelected(BDeRuntime* rt, const char* target_dir) {
    if (!rt || !rt->active || !target_dir) return 0;
    extern BDeTxHandle BDe_TransactionCopy(const char* src_path, const char* dest_dir);

    BDeTxHandle last_tx = 0;
    for (uint32_t i = 0; i < rt->item_count; i++) {
        if (rt->selected_mask[i]) {
            last_tx = BDe_TransactionCopy(rt->entries[i].full_path, target_dir);
        }
    }
    return last_tx;
}

BDeTxHandle BDeRuntime_MoveSelected(BDeRuntime* rt, const char* target_dir) {
    if (!rt || !rt->active || !target_dir) return 0;
    extern BDeTxHandle BDe_TransactionMove(const char* src_path, const char* dest_dir);

    BDeTxHandle last_tx = 0;
    for (uint32_t i = 0; i < rt->item_count; i++) {
        if (rt->selected_mask[i]) {
            last_tx = BDe_TransactionMove(rt->entries[i].full_path, target_dir);
        }
    }
    return last_tx;
}

BDeTxHandle BDeRuntime_DeleteSelected(BDeRuntime* rt, bool send_to_recycle) {
    if (!rt || !rt->active) return 0;
    extern BDeTxHandle BDe_TransactionDelete(const char* path, bool send_to_recycle);

    BDeTxHandle last_tx = 0;
    for (uint32_t i = 0; i < rt->item_count; i++) {
        if (rt->selected_mask[i]) {
            last_tx = BDe_TransactionDelete(rt->entries[i].full_path, send_to_recycle);
        }
    }
    return last_tx;
}
