#ifndef ABE_DIAGNOSTICS_H
#define ABE_DIAGNOSTICS_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_LOG_TRACE = 0,
    ABE_LOG_DEBUG = 1,
    ABE_LOG_INFO  = 2,
    ABE_LOG_WARN  = 3,
    ABE_LOG_ERROR = 4,
    ABE_LOG_FATAL = 5
} ABE_LogLevel;

void ABE_Diagnostics_Init(void);
void ABE_Diagnostics_Shutdown(void);
void ABE_Log(ABE_LogLevel level, const char* component, const char* message);
void ABE_LogVal(ABE_LogLevel level, const char* component, const char* message, uint64_t val);
void ABE_AssertFailed(const char* file, int line, const char* expr, const char* msg);

#define ABE_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            ABE_AssertFailed(__FILE__, __LINE__, #expr, (msg)); \
        } \
    } while(0)

void ABE_Diag_RecordWindowCreated(void);
void ABE_Diag_RecordWindowDestroyed(void);
void ABE_Diag_RecordTabCreated(void);
void ABE_Diag_RecordTabDestroyed(void);
void ABE_Diag_RecordNavigation(void);
void ABE_Diag_RecordURLParsed(void);
void ABE_Diag_RecordMemoryAlloc(size_t bytes);
void ABE_Diag_RecordMemoryFree(size_t bytes);
void ABE_Diag_RecordTimerUs(uint32_t parse_us, uint32_t render_us);

// Network Telemetry Hooks
void ABE_Diag_RecordDNSResolve(bool hit_cache, uint32_t time_us);
void ABE_Diag_RecordConnectionOpened(bool reused, uint32_t time_us);
void ABE_Diag_RecordTLSHandshake(uint32_t time_us);
void ABE_Diag_RecordHTTPRequest(size_t bytes_sent);
void ABE_Diag_RecordHTTPResponse(size_t bytes_rcvd, uint32_t ttfb_us);
void ABE_Diag_RecordRedirect(void);

// HTML5 & DOM Telemetry Hooks
void ABE_Diag_RecordDocumentParsed(uint32_t time_us);
void ABE_Diag_RecordDOMNodeAllocated(void);
void ABE_Diag_RecordDOMNodeFreed(void);
void ABE_Diag_RecordHTMLToken(void);
void ABE_Diag_RecordHTMLErrorRecovered(void);
void ABE_Diag_RecordDOMDepth(uint32_t depth);

// CSS Engine Telemetry Hooks
void ABE_Diag_RecordStylesheetParsed(uint32_t rule_count);
void ABE_Diag_RecordSelectorMatch(void);
void ABE_Diag_RecordStyleComputed(uint32_t time_us);
void ABE_Diag_RecordCSSErrorRecovered(void);

// Layout Engine Telemetry Hooks
void ABE_Diag_RecordRenderTreeBuilt(uint32_t node_count);
void ABE_Diag_RecordRenderNodeAllocated(void);
void ABE_Diag_RecordRenderNodeFreed(void);
void ABE_Diag_RecordLayoutPerformed(uint32_t time_us);
void ABE_Diag_RecordReflow(uint32_t dirty_nodes);

// JS Runtime Telemetry Hooks
void ABE_Diag_RecordJSContextCreated(void);
void ABE_Diag_RecordJSScriptExecuted(uint32_t time_us);
void ABE_Diag_RecordJSInstructionExecuted(void);
void ABE_Diag_RecordJSGCRun(size_t reclaimed_bytes);
void ABE_Diag_RecordJSPromiseResolved(void);
void ABE_Diag_RecordJSEventLoopTick(void);

// Web Platform Telemetry Hooks
void ABE_Diag_RecordFetchRequest(void);
void ABE_Diag_RecordXHRRequest(void);
void ABE_Diag_RecordStorageOp(bool is_write);
void ABE_Diag_RecordAnimationFrame(void);
void ABE_Diag_RecordMutationObserverTrigger(void);

ABE_DiagnosticsMetrics ABE_Diagnostics_GetMetrics(void);
void ABE_DumpMemoryDiagnostics(char* buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_DIAGNOSTICS_H
