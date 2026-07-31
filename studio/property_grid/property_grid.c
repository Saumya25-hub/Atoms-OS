#include "studio/include/studio_property_grid.h"
#include "kernel/core/lib/include/string.h"

void BOS_Studio_PropertyGridInit(void) {
    /* Initialize inspector */
}

void BOS_Studio_InspectNode(const BOS_StudioDesignerNode* node, BOS_PropertyGridEntry* out_entries, uint32_t* out_count) {
    if (!node || !out_entries || !out_count) return;

    uint32_t idx = 0;

    strncpy(out_entries[idx].category, "Identity", 31);
    strncpy(out_entries[idx].key, "Name", 31);
    strncpy(out_entries[idx].value, node->name, 63);
    idx++;

    strncpy(out_entries[idx].category, "Identity", 31);
    strncpy(out_entries[idx].key, "Type", 31);
    strncpy(out_entries[idx].value, node->type_name, 63);
    idx++;

    strncpy(out_entries[idx].category, "Layout", 31);
    strncpy(out_entries[idx].key, "X", 31);
    strncpy(out_entries[idx].value, "100", 63);
    idx++;

    strncpy(out_entries[idx].category, "Layout", 31);
    strncpy(out_entries[idx].key, "Y", 31);
    strncpy(out_entries[idx].value, "100", 63);
    idx++;

    *out_count = idx;
}

void BOS_Studio_UpdateProperty(BOS_StudioForm* form, uint32_t control_id, const char* key, const char* val) {
    if (!form || !key || !val) return;
    (void)control_id;
}
