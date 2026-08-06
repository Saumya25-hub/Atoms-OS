/**
 * @file bos_cur_loader.h
 * @brief Windows .CUR Binary Loader & ARGB Conversion Engine
 */

#ifndef BOS_CUR_LOADER_H
#define BOS_CUR_LOADER_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
    uint16_t idReserved;
    uint16_t idType;     /* 2 for CUR, 1 for ICO */
    uint16_t idCount;    /* Number of embedded images */
} BCE_ICONDIR;

typedef struct {
    uint8_t  bWidth;
    uint8_t  bHeight;
    uint8_t  bColorCount;
    uint8_t  bReserved;
    uint16_t wXHotspot;
    uint16_t wYHotspot;
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
} BCE_ICONDIRENTRY;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight; /* 2x image height (includes XOR + AND masks) */
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BCE_BITMAPINFOHEADER;
#pragma pack(pop)

bce_error_t bos_cur_parse(const uint8_t* data, size_t size, bce_cursor_t** out_cursor);
void        bos_cur_free(bce_cursor_t* cursor);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CUR_LOADER_H */
