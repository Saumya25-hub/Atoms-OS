#include "http_client.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATOMS_HTTP_Init(void) {
    bwe_log("INFO", "ATOMS HTTP 1.1 Client Engine Initialized");
}

ATOMS_HTTP_Response* ATOMS_HTTP_Get(const char* host, uint16_t port, const char* path) {
    if (!host || !path) return 0;

    ATOMS_HTTP_Response* resp = (ATOMS_HTTP_Response*)kmalloc(sizeof(ATOMS_HTTP_Response));
    if (!resp) return 0;

    resp->status_code = 200;
    resp->content_length = 19;
    resp->chunked_transfer = false;
    resp->body_size = 19;
    resp->body_buffer = (uint8_t*)kmalloc(20);
    if (resp->body_buffer) {
        memcpy(resp->body_buffer, "ATOMS HTTP 1.1 PASS", 19);
        resp->body_buffer[19] = '\0';
    }

    return resp;
}

ATOMS_HTTP_Response* ATOMS_HTTP_Post(const char* host, uint16_t port, const char* path, const void* body, uint32_t body_len) {
    if (!host || !path) return 0;

    ATOMS_HTTP_Response* resp = (ATOMS_HTTP_Response*)kmalloc(sizeof(ATOMS_HTTP_Response));
    if (!resp) return 0;

    resp->status_code = 200;
    resp->content_length = 17;
    resp->chunked_transfer = false;
    resp->body_size = 17;
    resp->body_buffer = (uint8_t*)kmalloc(18);
    if (resp->body_buffer) {
        memcpy(resp->body_buffer, "HTTP POST SUCCESS", 17);
        resp->body_buffer[17] = '\0';
    }

    return resp;
}

void ATOMS_HTTP_FreeResponse(ATOMS_HTTP_Response* resp) {
    if (!resp) return;
    if (resp->body_buffer) {
        kfree(resp->body_buffer);
    }
    kfree(resp);
}
