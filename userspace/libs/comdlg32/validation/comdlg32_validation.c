#include "../include/comdlg32_api.h"

BOOL comdlg32_validate_filename(const char* name) {
    if (!name || name[0] == '\0') return false;
    return true;
}
