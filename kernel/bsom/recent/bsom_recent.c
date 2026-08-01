#include "../include/bsom_api.h"

static BSOMObject* s_recent_items[16];
static uint32_t   s_recent_count = 0;

int32_t BSOM_GetRecent(BSOMObject*** out_items, uint32_t* out_count) {
    if (!out_items || !out_count) return -1;
    if (s_recent_count == 0) {
        s_recent_items[0] = BSOM_CreateObject("/DOCS/Doc.txt", BSOM_CLASS_DOCUMENT);
        s_recent_count = 1;
    }
    *out_items = s_recent_items;
    *out_count = s_recent_count;
    return 0;
}
