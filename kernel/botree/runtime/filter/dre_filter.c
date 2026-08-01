#include "../include/dre_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BDeRuntime_SetFilter(BDeRuntime* rt, const char* extension_pattern, bool show_hidden) {
    if (!rt || !rt->active) return -1;
    rt->show_hidden = show_hidden;
    if (extension_pattern) {
        strcpy(rt->extension_filter, extension_pattern);
    } else {
        rt->extension_filter[0] = '\0';
    }
    return 0;
}
