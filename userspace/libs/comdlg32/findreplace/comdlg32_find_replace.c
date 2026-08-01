#include "../include/comdlg32_api.h"

HANDLE FindText(FINDREPLACE* lpfr) {
    if (!lpfr) return 0;
    return (HANDLE)1;
}

HANDLE ReplaceText(FINDREPLACE* lpfr) {
    if (!lpfr) return 0;
    return (HANDLE)2;
}
