#ifndef STUDIO_PROPERTY_GRID_H
#define STUDIO_PROPERTY_GRID_H

#include "studio_designer.h"

typedef struct {
    char key[32];
    char value[64];
    char category[32];
} BOS_PropertyGridEntry;

void BOS_Studio_PropertyGridInit(void);
void BOS_Studio_InspectNode(const BOS_StudioDesignerNode* node, BOS_PropertyGridEntry* out_entries, uint32_t* out_count);
void BOS_Studio_UpdateProperty(BOS_StudioForm* form, uint32_t control_id, const char* key, const char* val);

#endif /* STUDIO_PROPERTY_GRID_H */
