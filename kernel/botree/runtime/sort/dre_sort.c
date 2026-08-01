#include "../include/dre_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BDeRuntime_SetSort(BDeRuntime* rt, BDeSortField field, bool ascending) {
    if (!rt || !rt->active) return -1;
    rt->sort_field = field;
    rt->sort_ascending = ascending;

    // Simple Bubble Sort over runtime entries array
    for (uint32_t i = 0; i < rt->item_count; i++) {
        for (uint32_t j = i + 1; j < rt->item_count; j++) {
            bool swap = false;
            if (field == DRE_SORT_NAME) {
                int cmp = strcmp(rt->entries[i].name, rt->entries[j].name);
                swap = ascending ? (cmp > 0) : (cmp < 0);
            } else if (field == DRE_SORT_SIZE) {
                swap = ascending ? (rt->entries[i].size_bytes > rt->entries[j].size_bytes)
                                 : (rt->entries[i].size_bytes < rt->entries[j].size_bytes);
            }
            if (swap) {
                BDeDirEntry tmp = rt->entries[i];
                rt->entries[i] = rt->entries[j];
                rt->entries[j] = tmp;
            }
        }
    }
    return 0;
}
