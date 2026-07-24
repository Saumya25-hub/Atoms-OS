#ifndef ABE_API_H
#define ABE_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS OS — ABE (ATOMS Browser Engine V1.0) Public API
// ============================================================

typedef struct ABE_Engine ABE_Engine;

// Lifecycle Management
ABE_Engine* ABE_CreateEngine(void);
void        ABE_DestroyEngine(ABE_Engine* engine);

// Web Content Pipelines
bool ABE_LoadURL(ABE_Engine* engine, const char* url);
bool ABE_ParseHTML(ABE_Engine* engine, const char* html);
bool ABE_ApplyCSS(ABE_Engine* engine, const char* css);
bool ABE_RunJavaScript(ABE_Engine* engine, const char* js);

// Rendering & Compositing
bool ABE_Render(ABE_Engine* engine, void* target_buffer, int32_t width, int32_t height);

// Diagnostics & Telemetry
void ABE_GetDiagnostics(ABE_Engine* engine, char* buffer, uint32_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_API_H
