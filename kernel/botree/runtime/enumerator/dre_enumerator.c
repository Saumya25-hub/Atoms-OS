#include "../include/dre_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

int32_t BDeRuntime_Enumerate(BDeRuntime* rt, BDeDirEntry** out_entries, uint32_t* out_count) {
    if (!rt || !rt->active || !out_entries || !out_count) return -1;

    uint32_t count = rt->item_count;
    if (count == 0) {
        *out_entries = NULL;
        *out_count = 0;
        return 0;
    }

    BDeDirEntry* copy = (BDeDirEntry*)kcalloc(count, sizeof(BDeDirEntry));
    if (!copy) return -1;

    memcpy(copy, rt->entries, count * sizeof(BDeDirEntry));
    *out_entries = copy;
    *out_count = count;
    return 0;
}
