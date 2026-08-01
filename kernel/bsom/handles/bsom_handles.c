#include "../include/bsom_api.h"

static BSOMObject* s_handle_table[BSOM_MAX_HANDLES];
static uint32_t   s_next_handle = 1;

BSOMHandle BSOM_OpenObject(const char* path_or_uri) {
    if (!path_or_uri) return 0;
    BSOMObject* obj = BSOM_CreateObject(path_or_uri, BSOM_CLASS_FILE);
    if (!obj) return 0;
    if (s_next_handle < BSOM_MAX_HANDLES) {
        s_handle_table[s_next_handle] = obj;
        return s_next_handle++;
    }
    return 0;
}

void BSOM_CloseObject(BSOMHandle handle) {
    if (handle > 0 && handle < BSOM_MAX_HANDLES) {
        if (s_handle_table[handle]) {
            BSOM_Release(s_handle_table[handle]);
            s_handle_table[handle] = NULL;
        }
    }
}
