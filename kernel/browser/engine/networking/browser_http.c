#include "browser_http.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_BrowserHTTP_Init(void) {
    bwe_log("INFO", "ATRIX Browser Networking & Protocol Subsystem Initialized");
}

bool ATRIX_BrowserHTTP_FetchURL(const char* url, uint8_t** out_data, uint32_t* out_len) {
    if (!url || !out_data || !out_len) return false;

    const char* mock_html = "<html><body><h1>ATRIX Browser Engine v1.0</h1></body></html>";
    uint32_t len = (uint32_t)strlen(mock_html);

    uint8_t* buf = (uint8_t*)kmalloc(len + 1);
    if (!buf) return false;

    memcpy(buf, mock_html, len + 1);
    *out_data = buf;
    *out_len = len;
    return true;
}
