#include "../include/bsom_api.h"

uint32_t BSOM_GetIcon(BSOMObject* obj) {
    if (!obj) return 1;
    return obj->icon_id;
}
