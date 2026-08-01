#include "../include/bsom_api.h"

BSOMObject* BSOM_GetVirtualRoot(void) {
    return BSOM_CreateObject("virtual://ThisPC", BSOM_CLASS_VIRTUAL);
}
