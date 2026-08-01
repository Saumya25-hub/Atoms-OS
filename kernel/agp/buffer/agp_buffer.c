// Engine 9: Buffer Manager (VBO / EBO / UBO)
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

typedef struct {
    AGPBufferType type;
    size_t        size;
    void*         data;
    bool          in_use;
} AGPBufferInternal;

static AGPBufferInternal s_buffer_pool[AGP_MAX_BUFFERS];

AGPBufferID AGP_CreateBuffer(AGPBufferType type, size_t size, const void* data) {
    for (uint32_t i = 0; i < AGP_MAX_BUFFERS; i++) {
        if (!s_buffer_pool[i].in_use) {
            AGPBufferInternal* buf = &s_buffer_pool[i];
            buf->in_use = true;
            buf->type = type;
            buf->size = size;

            buf->data = kmalloc(size > 0 ? size : 16);
            if (buf->data && data && size > 0) {
                memcpy(buf->data, data, size);
            }
            return i + 1;
        }
    }
    return 0;
}

void AGP_BindBuffer(AGPBufferType type, AGPBufferID buf_id) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;
    if (type == AGP_BUFFER_VERTEX) ctx->active_vbo = buf_id;
    else if (type == AGP_BUFFER_INDEX) ctx->active_ebo = buf_id;
}

void AGP_DeleteBuffer(AGPBufferID buf_id) {
    if (buf_id == 0 || buf_id > AGP_MAX_BUFFERS) return;
    uint32_t idx = buf_id - 1;
    if (s_buffer_pool[idx].in_use) {
        if (s_buffer_pool[idx].data) {
            kfree(s_buffer_pool[idx].data);
            s_buffer_pool[idx].data = NULL;
        }
        s_buffer_pool[idx].in_use = false;
    }
}
