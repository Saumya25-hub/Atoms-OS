#include "../include/bsom_api.h"

static BSOMObject* s_clipboard_obj = NULL;

int32_t BSOM_SetClipboard(BSOMObject* obj) {
    s_clipboard_obj = obj;
    return 0;
}

BSOMObject* BSOM_GetClipboard(void) {
    return s_clipboard_obj;
}
