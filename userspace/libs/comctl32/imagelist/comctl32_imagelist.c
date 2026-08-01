#include "../include/comctl32_api.h"

static uint32_t g_iml_counter = 1;

HIMAGELIST CreateImageList(int cx, int cy, uint32_t flags, int cInitial, int cGrow) {
    (void)cx; (void)cy; (void)flags; (void)cInitial; (void)cGrow;
    return (HIMAGELIST)(g_iml_counter++);
}

int ImageList_Add(HIMAGELIST himl, HANDLE hbmImage, HANDLE hbmMask) {
    (void)himl; (void)hbmImage; (void)hbmMask;
    return 0;
}

BOOL ImageList_Draw(HIMAGELIST himl, int i, HANDLE hdcDst, int x, int y, uint32_t fStyle) {
    (void)himl; (void)i; (void)hdcDst; (void)x; (void)y; (void)fStyle;
    return true;
}
