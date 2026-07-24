#ifndef ABE_WEB_BLOB_H
#define ABE_WEB_BLOB_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_BLOBS 32

typedef struct {
    ABE_BlobHandle handle;
    char mime_type[64];
    size_t size_bytes;
    uint8_t* data;
    char object_url[128];
    bool in_use;
} ABE_WebBlobInstance;

ABE_Error ABE_WebBlob_Init(void);
ABE_Error ABE_WebBlob_Shutdown(void);

ABE_Error ABE_WebBlob_Create(const uint8_t* data, size_t len, const char* mime_type, ABE_BlobHandle* out_blob);
ABE_Error ABE_WebBlob_CreateObjectURL(ABE_BlobHandle blob, char* out_url_buf, size_t max_len);
ABE_Error ABE_WebBlob_TextEncodeUTF8(const char* str, uint8_t* out_buf, size_t max_len, size_t* out_written);
ABE_Error ABE_WebBlob_TextDecodeUTF8(const uint8_t* buf, size_t len, char* out_str_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_BLOB_H
