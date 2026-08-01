#include "../include/bsom_api.h"

static BSOMObject* s_drag_obj = NULL;

int32_t BSOM_BeginDrag(BSOMObject* obj) {
    s_drag_obj = obj;
    return 0;
}

int32_t BSOM_EndDrag(BSOMObject* target_obj) {
    (void)target_obj;
    s_drag_obj = NULL;
    return 0;
}
