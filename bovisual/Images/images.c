#include "../Include/images.h"
#include "../Include/graphics.h"

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPFileHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

bool BOVISUAL_Draw_BMP(int32_t x, int32_t y, const uint8_t* bmp_data, uint32_t data_size) {
    if (!bmp_data || data_size < sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)) {
        return false;
    }

    const BMPFileHeader* file_header = (const BMPFileHeader*)bmp_data;
    if (file_header->bfType != 0x4D42) { // 'BM'
        return false; 
    }

    const BMPInfoHeader* info_header = (const BMPInfoHeader*)(bmp_data + sizeof(BMPFileHeader));

    // Only support 24-bit and 32-bit uncompressed BMPs for V1
    if (info_header->biCompression != 0) return false;
    if (info_header->biBitCount != 24 && info_header->biBitCount != 32) return false;

    const uint8_t* pixel_data = bmp_data + file_header->bfOffBits;
    int32_t width = info_header->biWidth;
    int32_t height = info_header->biHeight;
    bool bottom_up = true;

    if (height < 0) {
        height = -height;
        bottom_up = false;
    }

    uint32_t bytes_per_pixel = info_header->biBitCount / 8;
    uint32_t row_padded = (width * bytes_per_pixel + 3) & (~3);

    for (int32_t row = 0; row < height; row++) {
        int32_t target_y = bottom_up ? (y + height - 1 - row) : (y + row);
        
        const uint8_t* row_data = pixel_data + (row * row_padded);
        
        for (int32_t col = 0; col < width; col++) {
            uint32_t offset = col * bytes_per_pixel;
            uint8_t b = row_data[offset];
            uint8_t g = row_data[offset + 1];
            uint8_t r = row_data[offset + 2];
            uint8_t a = 0xFF; // Default alpha for 24-bit
            
            if (bytes_per_pixel == 4) {
                a = row_data[offset + 3];
            }

            // Compose ARGB
            BOVISUAL_Color color = (a << 24) | (r << 16) | (g << 8) | b;
            
            // Simple transparency threshold for V1 (e.g. if alpha == 0 don't draw)
            if (a > 0) {
                BOVISUAL_Graphics_PutPixel(x + col, target_y, color);
            }
        }
    }

    return true;
}
