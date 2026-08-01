#include "../include/comdlg32_api.h"
#include "kernel/core/lib/include/string.h"

static char g_history[512] = "C:\\Documents;C:\\Downloads;";

DWORD GetDialogHistory(char* buffer, uint32_t max_count) {
    if (!buffer || max_count == 0) return 0;
    strcpy(buffer, g_history);
    return (DWORD)strlen(g_history);
}

BOOL ClearDialogHistory(void) {
    g_history[0] = '\0';
    return true;
}
