#ifndef ATRIX_BROWSER_URL_H
#define ATRIX_BROWSER_URL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Real Address Bar & URL Scheme Parser Subsystem
// ============================================================

typedef enum {
    URL_SCHEME_UNKNOWN = 0,
    URL_SCHEME_HTTP,
    URL_SCHEME_HTTPS,
    URL_SCHEME_FILE,
    URL_SCHEME_ATRIX
} URLSchemeType;

typedef struct {
    char          raw_url[256];
    char          host[128];
    char          path[128];
    uint16_t      port;
    URLSchemeType scheme;
    bool          is_valid;
} ATRIX_ParsedURL;

void            ATRIX_URL_Init(void);
ATRIX_ParsedURL ATRIX_URL_Parse(const char* raw_input);
bool            ATRIX_URL_ValidateScheme(const char* input);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_URL_H
