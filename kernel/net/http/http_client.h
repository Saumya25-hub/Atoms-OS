#ifndef ATOMS_HTTP_CLIENT_H
#define ATOMS_HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS HTTP 1.1 Client Engine (Phase 11)
// ============================================================

typedef struct {
    uint16_t status_code;         // e.g. 200, 404, 301
    char     content_type[64];
    uint32_t content_length;
    bool     chunked_transfer;
    uint8_t* body_buffer;
    uint32_t body_size;
} ATOMS_HTTP_Response;

void                ATOMS_HTTP_Init(void);
ATOMS_HTTP_Response* ATOMS_HTTP_Get(const char* host, uint16_t port, const char* path);
ATOMS_HTTP_Response* ATOMS_HTTP_Post(const char* host, uint16_t port, const char* path, const void* body, uint32_t body_len);
void                ATOMS_HTTP_FreeResponse(ATOMS_HTTP_Response* resp);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_HTTP_CLIENT_H
