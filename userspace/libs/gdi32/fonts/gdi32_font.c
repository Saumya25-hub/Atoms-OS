#include "../include/gdi32_api.h"

static uint32_t g_font_counter = 1;

HFONT CreateFont(int32_t cHeight, int32_t cWidth, int32_t cEscapement, int32_t cOrientation, int32_t cWeight, uint32_t bItalic, uint32_t bUnderline, uint32_t bStrikeOut, uint32_t iCharSet, uint32_t iOutPrecision, uint32_t iClipPrecision, uint32_t iQuality, uint32_t iPitchAndFamily, const char* pszFaceName) {
    (void)cHeight; (void)cWidth; (void)cEscapement; (void)cOrientation; (void)cWeight;
    (void)bItalic; (void)bUnderline; (void)bStrikeOut; (void)iCharSet; (void)iOutPrecision;
    (void)iClipPrecision; (void)iQuality; (void)iPitchAndFamily; (void)pszFaceName;
    return (HFONT)(g_font_counter++);
}

HFONT CreateFontIndirect(const LOGFONT* lplf) {
    if (!lplf) return 0;
    return CreateFont(lplf->lfHeight, lplf->lfWidth, lplf->lfEscapement, lplf->lfOrientation, lplf->lfWeight, lplf->lfItalic, lplf->lfUnderline, lplf->lfStrikeOut, lplf->lfCharSet, lplf->lfOutPrecision, lplf->lfClipPrecision, lplf->lfQuality, lplf->lfPitchAndFamily, lplf->lfFaceName);
}

HFONT DeleteFont(HFONT hfont) {
    (void)hfont;
    return 0;
}
