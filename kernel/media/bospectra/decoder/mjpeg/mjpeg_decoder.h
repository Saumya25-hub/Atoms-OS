#ifndef MJPEG_DECODER_H
#define MJPEG_DECODER_H

#include "../registry/decoder_registry.h"

// ISO/IEC 10918-1 JPEG Marker Identifiers
#define JPEG_MARKER_SOI  0xFFD8U // Start of Image
#define JPEG_MARKER_SOF0 0xFFC0U // Start of Frame (Baseline DCT)
#define JPEG_MARKER_DHT  0xFFC4U // Define Huffman Table
#define JPEG_MARKER_DQT  0xFFDBU // Define Quantization Table
#define JPEG_MARKER_SOS  0xFFDAU // Start of Scan
#define JPEG_MARKER_EOI  0xFFD9U // End of Image

// Export MJPEG Decoder Driver Struct
extern const BOSPECTRA_DecoderDriver g_mjpeg_decoder_driver;

#endif // MJPEG_DECODER_H
