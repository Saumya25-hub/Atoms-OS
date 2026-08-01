#include "../include/bsom_api.h"

int32_t BSOM_RegisterObject(BSOMObject* obj, uint32_t owner_pid) {
    if (!obj) return -1;
    obj->owner_pid = owner_pid;
    return 0;
}
