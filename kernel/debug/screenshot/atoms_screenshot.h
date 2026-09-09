#ifndef ATOMS_SCREENSHOT_H
#define ATOMS_SCREENSHOT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SCREENSHOT_MAGIC         0x534D5053 // "SPMS" (Screen Packet Magic Stream)
#define SCREENSHOT_UDP_PORT      9998
#define SCREENSHOT_CHUNK_PAYLOAD 1400

#pragma pack(push, 1)

// Windows BMP File Header (14 bytes)
typedef struct {
    uint16_t bfType;          // 0x4D42 ("BM")
    uint32_t bfSize;          // Total size of BMP file in bytes
    uint16_t bfReserved1;     // 0
    uint16_t bfReserved2;     // 0
    uint32_t bfOffBits;       // Offset from beginning of file to bitmap bits (54)
} BmpFileHeader;

// Windows BMP Info Header (40 bytes)
typedef struct {
    uint32_t biSize;          // Size of this header (40)
    int32_t  biWidth;         // Width in pixels
    int32_t  biHeight;        // Height in pixels (positive = bottom-up)
    uint16_t biPlanes;        // Number of color planes (1)
    uint16_t biBitCount;      // Number of bits per pixel (32)
    uint32_t biCompression;   // Compression type (0 = BI_RGB, uncompressed)
    uint32_t biSizeImage;     // Image data size in bytes
    int32_t  biXPelsPerMeter; // Horizontal resolution (2835 = 72 DPI)
    int32_t  biYPelsPerMeter; // Vertical resolution (2835 = 72 DPI)
    uint32_t biClrUsed;       // Number of colors used (0)
    uint32_t biClrImportant;  // Important colors (0)
} BmpInfoHeader;

// UDP Packet Chunk Header (16 bytes)
typedef struct {
    uint32_t magic;           // 0x534D5053 ("SPMS")
    uint32_t session_id;      // Unique test session ID
    uint16_t total_chunks;    // Total chunks for this screenshot
    uint16_t chunk_index;     // 0, 1, 2, ..., total_chunks - 1
    uint32_t offset;          // Byte offset in complete BMP file
    uint16_t data_len;        // Length of pixel payload in this packet
    uint16_t flags;           // Bit 0: START, Bit 1: END
} ScreenshotChunkHeader;

#pragma pack(pop)

void atoms_screenshot_init(void);
bool atoms_screenshot_capture_and_send(uint32_t session_id);
bool atoms_screenshot_capture_sync(uint32_t session_id);
bool atoms_screenshot_request(uint32_t session_id);
bool atoms_screenshot_step(void);
bool atoms_screenshot_is_busy(void);

#endif /* ATOMS_SCREENSHOT_H */
