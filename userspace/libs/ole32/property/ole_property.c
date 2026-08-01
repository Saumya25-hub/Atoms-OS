#include "../include/ole32_api.h"

int OLE32CreatePropertySetStorage(IStorage* pStg, void** ppPropSetStg) {
    (void)pStg;
    if (ppPropSetStg) *ppPropSetStg = NULL;
    return 0;
}
