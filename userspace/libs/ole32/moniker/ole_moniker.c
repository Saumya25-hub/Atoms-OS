#include "../include/ole32_api.h"

int OLE32CreateItemMoniker(const char* lpszDelim, const char* lpszItem, void** ppmk) {
    (void)lpszDelim; (void)lpszItem;
    if (ppmk) *ppmk = NULL;
    return 0;
}
