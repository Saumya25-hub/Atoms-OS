#ifndef ATRIX_BROWSER_HTTP_H
#define ATRIX_BROWSER_HTTP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Browser Networking & Protocol Layer
// ============================================================

void ATRIX_BrowserHTTP_Init(void);
bool ATRIX_BrowserHTTP_FetchURL(const char* url, uint8_t** out_data, uint32_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_HTTP_H
