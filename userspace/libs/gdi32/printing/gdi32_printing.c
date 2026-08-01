#include "../include/gdi32_api.h"

int32_t StartDoc(HDC hdc, const void* lpdi) {
    (void)hdc; (void)lpdi;
    return 1;
}

int32_t EndDoc(HDC hdc) {
    (void)hdc;
    return 1;
}
