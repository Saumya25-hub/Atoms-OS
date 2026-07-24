#include "abe_web_blob.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static ABE_WebBlobInstance g_blob_pool[ABE_MAX_BLOBS];
static uint32_t g_next_blob_id = 1500;
static bool g_web_blob_initialized = false;

ABE_Error ABE_WebBlob_Init(void) {
    memset(g_blob_pool, 0, sizeof(g_blob_pool));
    g_web_blob_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBBLOB", "ABE Blob, File & Text Encoding Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebBlob_Shutdown(void) {
    if (!g_web_blob_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_BLOBS; i++) {
        if (g_blob_pool[i].in_use && g_blob_pool[i].data) {
            kfree(g_blob_pool[i].data);
            g_blob_pool[i].data = NULL;
        }
    }
    g_web_blob_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebBlob_Create(const uint8_t* data, size_t len, const char* mime_type, ABE_BlobHandle* out_blob) {
    if (!g_web_blob_initialized || !out_blob) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_BLOBS; i++) {
        if (!g_blob_pool[i].in_use) {
            ABE_WebBlobInstance* b = &g_blob_pool[i];
            memset(b, 0, sizeof(ABE_WebBlobInstance));
            b->handle = (g_next_blob_id++) | (i << 16);
            strncpy(b->mime_type, mime_type ? mime_type : "application/octet-stream", sizeof(b->mime_type) - 1);
            b->size_bytes = len;
            b->in_use = true;

            *out_blob = b->handle;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_WebBlob_CreateObjectURL(ABE_BlobHandle blob, char* out_url_buf, size_t max_len) {
    if (!g_web_blob_initialized || blob == ABE_INVALID_HANDLE || !out_url_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    strncpy(out_url_buf, "blob:http://example.com/uuid-blob-1234", max_len - 1);
    return ABE_SUCCESS;
}

ABE_Error ABE_WebBlob_TextEncodeUTF8(const char* str, uint8_t* out_buf, size_t max_len, size_t* out_written) {
    if (!str || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    size_t len = strlen(str);
    if (len >= max_len) len = max_len - 1;
    memcpy(out_buf, str, len);
    if (out_written) *out_written = len;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebBlob_TextDecodeUTF8(const uint8_t* buf, size_t len, char* out_str_buf, size_t max_len) {
    if (!buf || !out_str_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    size_t copy_len = (len < max_len - 1) ? len : max_len - 1;
    memcpy(out_str_buf, buf, copy_len);
    out_str_buf[copy_len] = '\0';
    return ABE_SUCCESS;
}
