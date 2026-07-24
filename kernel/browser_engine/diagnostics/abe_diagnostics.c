#include "abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static ABE_DiagnosticsMetrics g_diag_metrics;
static bool g_diag_initialized = false;

void ABE_Diagnostics_Init(void) {
    memset(&g_diag_metrics, 0, sizeof(ABE_DiagnosticsMetrics));
    g_diag_initialized = true;
    ABE_Log(ABE_LOG_INFO, "DIAG", "ABE Diagnostics Engine initialized successfully");
}

void ABE_Diagnostics_Shutdown(void) {
    if (!g_diag_initialized) return;
    ABE_Log(ABE_LOG_INFO, "DIAG", "ABE Diagnostics Engine shutting down. Auditing memory & networking...");
    if (g_diag_metrics.allocated_memory_bytes != 0) {
        ABE_LogVal(ABE_LOG_WARN, "DIAG", "Memory Leak Warning! Unfreed bytes count: ", (uint64_t)g_diag_metrics.allocated_memory_bytes);
    } else {
        ABE_Log(ABE_LOG_INFO, "DIAG", "Zero memory leaks verified!");
    }
    g_diag_initialized = false;
}

void ABE_Log(ABE_LogLevel level, const char* component, const char* message) {
    const char* lvl_str = "[INFO]";
    switch(level) {
        case ABE_LOG_TRACE: lvl_str = "[TRACE]"; break;
        case ABE_LOG_DEBUG: lvl_str = "[DEBUG]"; break;
        case ABE_LOG_INFO:  lvl_str = "[INFO ]"; break;
        case ABE_LOG_WARN:  lvl_str = "[WARN ]"; break;
        case ABE_LOG_ERROR: lvl_str = "[ERROR]"; break;
        case ABE_LOG_FATAL: lvl_str = "[FATAL]"; break;
    }
    display_print("[ABE]");
    display_print(lvl_str);
    display_print("[");
    display_print(component ? component : "CORE");
    display_print("] ");
    display_print(message ? message : "");
    display_print("\n");
}

void ABE_LogVal(ABE_LogLevel level, const char* component, const char* message, uint64_t val) {
    display_print("[ABE][VAL]");
    display_print("[");
    display_print(component ? component : "CORE");
    display_print("] ");
    display_print(message ? message : "");
    display_print_dec((uint32_t)val);
    display_print("\n");
}

void ABE_AssertFailed(const char* file, int line, const char* expr, const char* msg) {
    display_print("\n=========================================\n");
    display_print(" [ABE ASSERTION FAILURE] \n");
    display_print(" File: "); display_print(file); display_print("\n");
    display_print(" Line: "); display_print_dec((uint32_t)line); display_print("\n");
    display_print(" Expr: "); display_print(expr); display_print("\n");
    display_print(" Msg : "); display_print(msg); display_print("\n");
    display_print("=========================================\n");
}

void ABE_Diag_RecordWindowCreated(void) {
    g_diag_metrics.total_windows_created++;
    g_diag_metrics.active_windows++;
}

void ABE_Diag_RecordWindowDestroyed(void) {
    if (g_diag_metrics.active_windows > 0) {
        g_diag_metrics.active_windows--;
    }
}

void ABE_Diag_RecordTabCreated(void) {
    g_diag_metrics.total_tabs_created++;
    g_diag_metrics.active_tabs++;
}

void ABE_Diag_RecordTabDestroyed(void) {
    if (g_diag_metrics.active_tabs > 0) {
        g_diag_metrics.active_tabs--;
    }
}

void ABE_Diag_RecordNavigation(void) {
    g_diag_metrics.total_navigations++;
}

void ABE_Diag_RecordURLParsed(void) {
    g_diag_metrics.urls_parsed_count++;
}

void ABE_Diag_RecordMemoryAlloc(size_t bytes) {
    g_diag_metrics.allocated_memory_bytes += bytes;
    if (g_diag_metrics.allocated_memory_bytes > g_diag_metrics.peak_memory_bytes) {
        g_diag_metrics.peak_memory_bytes = g_diag_metrics.allocated_memory_bytes;
    }
}

void ABE_Diag_RecordMemoryFree(size_t bytes) {
    if (g_diag_metrics.allocated_memory_bytes >= bytes) {
        g_diag_metrics.allocated_memory_bytes -= bytes;
    } else {
        g_diag_metrics.allocated_memory_bytes = 0;
    }
}

void ABE_Diag_RecordTimerUs(uint32_t parse_us, uint32_t render_us) {
    g_diag_metrics.parse_time_us = parse_us;
    g_diag_metrics.render_time_us = render_us;
}

void ABE_Diag_RecordDNSResolve(bool hit_cache, uint32_t time_us) {
    g_diag_metrics.dns_resolutions_total++;
    if (hit_cache) g_diag_metrics.dns_cache_hits++;
    g_diag_metrics.dns_lookup_time_us = time_us;
}

void ABE_Diag_RecordConnectionOpened(bool reused, uint32_t time_us) {
    g_diag_metrics.active_connections++;
    if (reused) g_diag_metrics.reused_connections++;
    g_diag_metrics.tcp_connect_time_us = time_us;
}

void ABE_Diag_RecordTLSHandshake(uint32_t time_us) {
    g_diag_metrics.tls_handshake_time_us = time_us;
}

void ABE_Diag_RecordHTTPRequest(size_t bytes_sent) {
    g_diag_metrics.http_requests_sent++;
    g_diag_metrics.total_bytes_uploaded += (uint32_t)bytes_sent;
}

void ABE_Diag_RecordHTTPResponse(size_t bytes_rcvd, uint32_t ttfb_us) {
    g_diag_metrics.http_responses_rcvd++;
    g_diag_metrics.total_bytes_downloaded += (uint32_t)bytes_rcvd;
    g_diag_metrics.ttfb_us = ttfb_us;
}

void ABE_Diag_RecordRedirect(void) {
    g_diag_metrics.redirects_followed++;
}

void ABE_Diag_RecordDocumentParsed(uint32_t time_us) {
    g_diag_metrics.documents_parsed_total++;
    g_diag_metrics.html_parse_time_us = time_us;
}

void ABE_Diag_RecordDOMNodeAllocated(void) {
    g_diag_metrics.dom_nodes_allocated++;
    g_diag_metrics.dom_nodes_active++;
}

void ABE_Diag_RecordDOMNodeFreed(void) {
    if (g_diag_metrics.dom_nodes_active > 0) {
        g_diag_metrics.dom_nodes_active--;
    }
}

void ABE_Diag_RecordHTMLToken(void) {
    g_diag_metrics.html_tokens_generated++;
}

void ABE_Diag_RecordHTMLErrorRecovered(void) {
    g_diag_metrics.html_parse_errors_recovered++;
}

void ABE_Diag_RecordDOMDepth(uint32_t depth) {
    if (depth > g_diag_metrics.dom_tree_max_depth) {
        g_diag_metrics.dom_tree_max_depth = depth;
    }
}

void ABE_Diag_RecordStylesheetParsed(uint32_t rule_count) {
    g_diag_metrics.stylesheets_parsed_total++;
    g_diag_metrics.css_rules_total += rule_count;
}

void ABE_Diag_RecordSelectorMatch(void) {
    g_diag_metrics.css_selectors_matched++;
}

void ABE_Diag_RecordStyleComputed(uint32_t time_us) {
    g_diag_metrics.css_styles_computed_total++;
    g_diag_metrics.css_compute_time_us = time_us;
}

void ABE_Diag_RecordCSSErrorRecovered(void) {
    g_diag_metrics.css_parse_errors_recovered++;
}

void ABE_Diag_RecordRenderTreeBuilt(uint32_t node_count) {
    g_diag_metrics.render_trees_built_total++;
}

void ABE_Diag_RecordRenderNodeAllocated(void) {
    g_diag_metrics.render_nodes_active++;
}

void ABE_Diag_RecordRenderNodeFreed(void) {
    if (g_diag_metrics.render_nodes_active > 0) {
        g_diag_metrics.render_nodes_active--;
    }
}

void ABE_Diag_RecordLayoutPerformed(uint32_t time_us) {
    g_diag_metrics.layouts_performed_total++;
    g_diag_metrics.layout_time_us = time_us;
}

void ABE_Diag_RecordReflow(uint32_t dirty_nodes) {
    g_diag_metrics.reflows_performed_total++;
    g_diag_metrics.dirty_nodes_processed += dirty_nodes;
}

void ABE_Diag_RecordJSContextCreated(void) {
    g_diag_metrics.js_contexts_created++;
}

void ABE_Diag_RecordJSScriptExecuted(uint32_t time_us) {
    g_diag_metrics.js_scripts_executed++;
    g_diag_metrics.js_execution_time_us = time_us;
}

void ABE_Diag_RecordJSInstructionExecuted(void) {
    g_diag_metrics.js_bytecode_instructions_executed++;
}

void ABE_Diag_RecordJSGCRun(size_t reclaimed_bytes) {
    g_diag_metrics.js_gc_runs_total++;
}

void ABE_Diag_RecordJSPromiseResolved(void) {
    g_diag_metrics.js_promises_resolved++;
}

void ABE_Diag_RecordJSEventLoopTick(void) {
    g_diag_metrics.js_event_loop_ticks++;
}

void ABE_Diag_RecordFetchRequest(void) {
    g_diag_metrics.fetch_requests_total++;
}

void ABE_Diag_RecordXHRRequest(void) {
    g_diag_metrics.xhr_requests_total++;
}

void ABE_Diag_RecordStorageOp(bool is_write) {
    if (is_write) g_diag_metrics.storage_write_ops++;
    else g_diag_metrics.storage_read_ops++;
}

void ABE_Diag_RecordAnimationFrame(void) {
    g_diag_metrics.animation_frames_requested++;
}

void ABE_Diag_RecordMutationObserverTrigger(void) {
    g_diag_metrics.mutation_observer_triggers++;
}

ABE_DiagnosticsMetrics ABE_Diagnostics_GetMetrics(void) {
    return g_diag_metrics;
}

void ABE_DumpMemoryDiagnostics(char* buffer, size_t max_len) {
    if (!buffer || max_len == 0) return;
    memset(buffer, 0, max_len);
    strncpy(buffer, "ABE Memory Audit: Allocated=", max_len - 1);
}

const char* ABE_GetErrorString(ABE_Error error) {
    switch (error) {
        case ABE_SUCCESS: return "ABE_SUCCESS: Operation completed successfully";
        case ABE_ERR_NOT_INITIALIZED: return "ABE_ERR_NOT_INITIALIZED: Engine core is not initialized";
        case ABE_ERR_ALREADY_INITIALIZED: return "ABE_ERR_ALREADY_INITIALIZED: Engine core is already initialized";
        case ABE_ERR_INVALID_PARAM: return "ABE_ERR_INVALID_PARAM: Invalid parameter supplied";
        case ABE_ERR_OUT_OF_MEMORY: return "ABE_ERR_OUT_OF_MEMORY: Memory allocation failure";
        case ABE_ERR_WINDOW_FAILED: return "ABE_ERR_WINDOW_FAILED: Window creation/manipulation failed";
        case ABE_ERR_TAB_NOT_FOUND: return "ABE_ERR_TAB_NOT_FOUND: Tab handle not found in window";
        case ABE_ERR_INVALID_URL: return "ABE_ERR_INVALID_URL: Failed to parse URL or invalid scheme";
        case ABE_ERR_NAVIGATION_FAILED: return "ABE_ERR_NAVIGATION_FAILED: Navigation engine failure";
        case ABE_ERR_RESOURCE_EXHAUSTED: return "ABE_ERR_RESOURCE_EXHAUSTED: Resource handles or pool exhausted";
        case ABE_ERR_NOT_SUPPORTED: return "ABE_ERR_NOT_SUPPORTED: Requested feature not supported";
        case ABE_ERR_INTERNAL_FAILURE: return "ABE_ERR_INTERNAL_FAILURE: Internal subsystem failure";
        case ABE_ERR_NET_DNS_FAILED: return "ABE_ERR_NET_DNS_FAILED: DNS resolution failed or timed out";
        case ABE_ERR_NET_CONNECT_FAILED: return "ABE_ERR_NET_CONNECT_FAILED: Socket connection failed";
        case ABE_ERR_NET_TLS_FAILED: return "ABE_ERR_NET_TLS_FAILED: TLS handshake or crypto pipeline failed";
        case ABE_ERR_NET_SEND_FAILED: return "ABE_ERR_NET_SEND_FAILED: Socket send failed";
        case ABE_ERR_NET_RECV_FAILED: return "ABE_ERR_NET_RECV_FAILED: Socket recv failed";
        case ABE_ERR_NET_PARSE_FAILED: return "ABE_ERR_NET_PARSE_FAILED: HTTP response parsing failed";
        case ABE_ERR_NET_TOO_MANY_REDIRECTS: return "ABE_ERR_NET_TOO_MANY_REDIRECTS: Exceeded max redirect limit";
        case ABE_ERR_NET_TIMEOUT: return "ABE_ERR_NET_TIMEOUT: Network operation timed out";
        case ABE_ERR_NET_CANCELLED: return "ABE_ERR_NET_CANCELLED: Network request was cancelled";
        case ABE_ERR_HTML_TOKENIZER_FAILED: return "ABE_ERR_HTML_TOKENIZER_FAILED: HTML tokenizer state error";
        case ABE_ERR_HTML_PARSER_FAILED: return "ABE_ERR_HTML_PARSER_FAILED: HTML tree construction parser error";
        case ABE_ERR_DOM_NODE_NOT_FOUND: return "ABE_ERR_DOM_NODE_NOT_FOUND: Specified DOM node handle was not found";
        case ABE_ERR_DOM_HIERARCHY_ERROR: return "ABE_ERR_DOM_HIERARCHY_ERROR: Invalid DOM hierarchy operation";
        case ABE_ERR_DOM_INDEX_OUT_OF_BOUNDS: return "ABE_ERR_DOM_INDEX_OUT_OF_BOUNDS: DOM index out of bounds";
        case ABE_ERR_CSS_TOKENIZER_FAILED: return "ABE_ERR_CSS_TOKENIZER_FAILED: CSS tokenizer state error";
        case ABE_ERR_CSS_PARSER_FAILED: return "ABE_ERR_CSS_PARSER_FAILED: CSS parser ruleset syntax error";
        case ABE_ERR_CSS_INVALID_SELECTOR: return "ABE_ERR_CSS_INVALID_SELECTOR: Invalid CSS selector syntax";
        case ABE_ERR_CSS_STYLE_COMPUTATION_FAILED: return "ABE_ERR_CSS_STYLE_COMPUTATION_FAILED: Style computation failure";
        case ABE_ERR_LAYOUT_NOT_INITIALIZED: return "ABE_ERR_LAYOUT_NOT_INITIALIZED: Layout engine not initialized";
        case ABE_ERR_RENDER_TREE_FAILED: return "ABE_ERR_RENDER_TREE_FAILED: Render tree construction failed";
        case ABE_ERR_LAYOUT_BOX_NOT_FOUND: return "ABE_ERR_LAYOUT_BOX_NOT_FOUND: Layout box handle not found";
        case ABE_ERR_LAYOUT_COMPUTATION_FAILED: return "ABE_ERR_LAYOUT_COMPUTATION_FAILED: Layout geometry calculation error";
        case ABE_ERR_JS_LEXER_FAILED: return "ABE_ERR_JS_LEXER_FAILED: JavaScript lexer tokenization error";
        case ABE_ERR_JS_PARSER_FAILED: return "ABE_ERR_JS_PARSER_FAILED: JavaScript AST syntax parser error";
        case ABE_ERR_JS_COMPILER_FAILED: return "ABE_ERR_JS_COMPILER_FAILED: Bytecode compilation failure";
        case ABE_ERR_JS_VM_EXCEPTION: return "ABE_ERR_JS_VM_EXCEPTION: Uncaught VM runtime exception";
        case ABE_ERR_JS_TYPE_ERROR: return "ABE_ERR_JS_TYPE_ERROR: JavaScript TypeError exception";
        case ABE_ERR_JS_REFERENCE_ERROR: return "ABE_ERR_JS_REFERENCE_ERROR: JavaScript ReferenceError exception";
        case ABE_ERR_JS_SYNTAX_ERROR: return "ABE_ERR_JS_SYNTAX_ERROR: JavaScript SyntaxError exception";
        case ABE_ERR_JS_PROMISE_REJECTED: return "ABE_ERR_JS_PROMISE_REJECTED: Unhandled promise rejection";
        case ABE_ERR_WEB_FETCH_FAILED: return "ABE_ERR_WEB_FETCH_FAILED: Fetch API network operation failed";
        case ABE_ERR_WEB_XHR_FAILED: return "ABE_ERR_WEB_XHR_FAILED: XMLHttpRequest operation failed";
        case ABE_ERR_WEB_STORAGE_FULL: return "ABE_ERR_WEB_STORAGE_FULL: Web Storage quota exceeded";
        case ABE_ERR_WEB_OBSERVER_FAILED: return "ABE_ERR_WEB_OBSERVER_FAILED: Web Observer operation failed";
        default: return "ABE_ERR_UNKNOWN: Unknown error code";
    }
}
