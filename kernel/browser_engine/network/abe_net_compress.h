#ifndef ABE_NET_COMPRESS_H
#define ABE_NET_COMPRESS_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_COMPRESS_NONE = 0,
    ABE_COMPRESS_GZIP = 1,
    ABE_COMPRESS_DEFLATE = 2,
    ABE_COMPRESS_BROTLI = 3
} ABE_CompressionFormat;

ABE_Error ABE_NetCompress_Init(void);
ABE_Error ABE_NetCompress_Shutdown(void);

ABE_Error ABE_NetCompress_Decompress(ABE_CompressionFormat format, const uint8_t* compressed_buf, size_t compressed_len, uint8_t** out_decompressed, size_t* out_decompressed_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_COMPRESS_H
