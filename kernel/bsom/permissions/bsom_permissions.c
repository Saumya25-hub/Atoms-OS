#include "../include/bsom_api.h"

int32_t BSOM_Lock(BSOMObject* obj, uint32_t lock_type) {
    if (!obj) return -1;
    (void)lock_type;
    return 0;
}

int32_t BSOM_Unlock(BSOMObject* obj) {
    if (!obj) return -1;
    return 0;
}
