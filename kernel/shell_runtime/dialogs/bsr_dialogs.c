#include "../include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BSR_ShowFileDialog(BSR_Runtime* rt, const char* title, bool is_save, char* out_selected_path, size_t max_len) {
    if (!rt || !out_selected_path || max_len == 0) return -1;
    (void)title;
    (void)is_save;
    strcpy(out_selected_path, "/DESKTOP/sample.txt");
    return 0;
}
