#ifndef ATOMS_ABE_H
#define ATOMS_ABE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS OS — ABE (ATOMS Browser Engine V1.0) Public SDK API
// ============================================================

#define ABE_VERSION_MAJOR 1
#define ABE_VERSION_MINOR 0
#define ABE_VERSION_PATCH 0
#define ABE_VERSION_STRING "1.0.0-Phase8"

#define ABE_INVALID_HANDLE 0xFFFFFFFF
#define ABE_MAX_URL_LEN    2048
#define ABE_MAX_TITLE_LEN  256
#define ABE_MAX_HEADERS    32
#define ABE_MAX_ATTRIBUTES 32

// Error Codes
typedef enum {
    ABE_SUCCESS = 0,
    ABE_ERR_NOT_INITIALIZED = -1,
    ABE_ERR_ALREADY_INITIALIZED = -2,
    ABE_ERR_INVALID_PARAM = -3,
    ABE_ERR_OUT_OF_MEMORY = -4,
    ABE_ERR_WINDOW_FAILED = -5,
    ABE_ERR_TAB_NOT_FOUND = -6,
    ABE_ERR_INVALID_URL = -7,
    ABE_ERR_NAVIGATION_FAILED = -8,
    ABE_ERR_RESOURCE_EXHAUSTED = -9,
    ABE_ERR_NOT_SUPPORTED = -10,
    ABE_ERR_INTERNAL_FAILURE = -11,
    
    // Networking Error Codes
    ABE_ERR_NET_DNS_FAILED = -20,
    ABE_ERR_NET_CONNECT_FAILED = -21,
    ABE_ERR_NET_TLS_FAILED = -22,
    ABE_ERR_NET_SEND_FAILED = -23,
    ABE_ERR_NET_RECV_FAILED = -24,
    ABE_ERR_NET_PARSE_FAILED = -25,
    ABE_ERR_NET_TOO_MANY_REDIRECTS = -26,
    ABE_ERR_NET_TIMEOUT = -27,
    ABE_ERR_NET_CANCELLED = -28,

    // HTML5 & DOM Error Codes
    ABE_ERR_HTML_TOKENIZER_FAILED = -40,
    ABE_ERR_HTML_PARSER_FAILED = -41,
    ABE_ERR_DOM_NODE_NOT_FOUND = -42,
    ABE_ERR_DOM_HIERARCHY_ERROR = -43,
    ABE_ERR_DOM_INDEX_OUT_OF_BOUNDS = -44,

    // CSS Engine Error Codes
    ABE_ERR_CSS_TOKENIZER_FAILED = -60,
    ABE_ERR_CSS_PARSER_FAILED = -61,
    ABE_ERR_CSS_INVALID_SELECTOR = -62,
    ABE_ERR_CSS_STYLE_COMPUTATION_FAILED = -63,

    // Layout Engine Error Codes
    ABE_ERR_LAYOUT_NOT_INITIALIZED = -80,
    ABE_ERR_RENDER_TREE_FAILED = -81,
    ABE_ERR_LAYOUT_BOX_NOT_FOUND = -82,
    ABE_ERR_LAYOUT_COMPUTATION_FAILED = -83,

    // JS Runtime Error Codes
    ABE_ERR_JS_LEXER_FAILED = -100,
    ABE_ERR_JS_PARSER_FAILED = -101,
    ABE_ERR_JS_COMPILER_FAILED = -102,
    ABE_ERR_JS_VM_EXCEPTION = -103,
    ABE_ERR_JS_TYPE_ERROR = -104,
    ABE_ERR_JS_REFERENCE_ERROR = -105,
    ABE_ERR_JS_SYNTAX_ERROR = -106,
    ABE_ERR_JS_PROMISE_REJECTED = -107,

    // Web Platform Error Codes
    ABE_ERR_WEB_FETCH_FAILED = -120,
    ABE_ERR_WEB_XHR_FAILED = -121,
    ABE_ERR_WEB_STORAGE_FULL = -122,
    ABE_ERR_WEB_OBSERVER_FAILED = -123
} ABE_Error;

// Core Handles
typedef uint32_t ABE_WindowHandle;
typedef uint32_t ABE_TabHandle;
typedef uint32_t ABE_ResourceHandle;
typedef uint32_t ABE_ConnHandle;
typedef uint32_t ABE_RequestHandle;
typedef uint32_t ABE_DownloadHandle;

// HTML5 & DOM Handles
typedef uint32_t ABE_DocumentHandle;
typedef uint32_t ABE_NodeHandle;

// CSS Handles
typedef uint32_t ABE_StylesheetHandle;

// Layout Handles
typedef uint32_t ABE_RenderTreeHandle;
typedef uint32_t ABE_LayoutBoxHandle;

// JavaScript Handles
typedef uint32_t ABE_JSContextHandle;
typedef uint32_t ABE_JSScriptHandle;

// Web Platform Handles
typedef uint32_t ABE_FetchRequestHandle;
typedef uint32_t ABE_XHRHandle;
typedef uint32_t ABE_URLHandle;
typedef uint32_t ABE_StorageHandle;
typedef uint32_t ABE_BlobHandle;
typedef uint32_t ABE_ObserverHandle;

// JS Value Types
typedef enum {
    ABE_JS_TYPE_UNDEFINED = 0,
    ABE_JS_TYPE_NULL = 1,
    ABE_JS_TYPE_BOOLEAN = 2,
    ABE_JS_TYPE_NUMBER = 3,
    ABE_JS_TYPE_STRING = 4,
    ABE_JS_TYPE_OBJECT = 5,
    ABE_JS_TYPE_FUNCTION = 6,
    ABE_JS_TYPE_PROMISE = 7,
    ABE_JS_TYPE_SYMBOL = 8
} ABE_JSType;

typedef struct {
    ABE_JSType type;
    union {
        bool boolean_val;
        double number_val;
        char string_val[256];
        uint32_t object_handle;
    } u;
} ABE_JSValue;

// Geometry Structs
typedef struct {
    float x;
    float y;
    float width;
    float height;
} ABE_Rect;

typedef struct {
    float top;
    float right;
    float bottom;
    float left;
} ABE_EdgeSizes;

// DOM Node Types
typedef enum {
    ABE_NODE_DOCUMENT = 1,
    ABE_NODE_ELEMENT = 2,
    ABE_NODE_TEXT = 3,
    ABE_NODE_COMMENT = 4,
    ABE_NODE_DOCUMENT_TYPE = 5,
    ABE_NODE_DOCUMENT_FRAGMENT = 6
} ABE_NodeType;

// CSS Computed Style Enums
typedef enum {
    ABE_DISPLAY_BLOCK = 0,
    ABE_DISPLAY_INLINE = 1,
    ABE_DISPLAY_INLINE_BLOCK = 2,
    ABE_DISPLAY_NONE = 3,
    ABE_DISPLAY_FLEX = 4,
    ABE_DISPLAY_GRID = 5,
    ABE_DISPLAY_TABLE = 6
} ABE_DisplayType;

typedef enum {
    ABE_POSITION_STATIC = 0,
    ABE_POSITION_RELATIVE = 1,
    ABE_POSITION_ABSOLUTE = 2,
    ABE_POSITION_FIXED = 3,
    ABE_POSITION_STICKY = 4
} ABE_PositionType;

typedef enum {
    ABE_TEXT_ALIGN_LEFT = 0,
    ABE_TEXT_ALIGN_CENTER = 1,
    ABE_TEXT_ALIGN_RIGHT = 2,
    ABE_TEXT_ALIGN_JUSTIFY = 3
} ABE_TextAlignType;

typedef enum {
    ABE_VISIBILITY_VISIBLE = 0,
    ABE_VISIBILITY_HIDDEN = 1,
    ABE_VISIBILITY_COLLAPSE = 2
} ABE_VisibilityType;

typedef enum {
    ABE_OVERFLOW_VISIBLE = 0,
    ABE_OVERFLOW_HIDDEN = 1,
    ABE_OVERFLOW_SCROLL = 2,
    ABE_OVERFLOW_AUTO = 3
} ABE_OverflowType;

// Computed Style Representation
typedef struct {
    ABE_DisplayType display;
    ABE_PositionType position;

    // Box Model
    float width_px;
    bool  width_auto;
    float height_px;
    bool  height_auto;

    float margin_top_px;
    float margin_right_px;
    float margin_bottom_px;
    float margin_left_px;

    float padding_top_px;
    float padding_right_px;
    float padding_bottom_px;
    float padding_left_px;

    float border_top_width_px;
    float border_right_width_px;
    float border_bottom_width_px;
    float border_left_width_px;

    // Colors (RGBA 32-bit uint)
    uint32_t color;
    uint32_t background_color;

    // Typography
    float font_size_px;
    uint32_t font_weight; // 100..900
    float line_height_px;
    ABE_TextAlignType text_align;
    char font_family[64];

    // Visuals
    float opacity; // 0.0 .. 1.0
    ABE_VisibilityType visibility;
    ABE_OverflowType overflow;
} ABE_ComputedStyle;

// Layout Box Representation
typedef struct {
    ABE_LayoutBoxHandle handle;
    ABE_NodeHandle node_handle;
    ABE_Rect content_box;
    ABE_EdgeSizes margin;
    ABE_EdgeSizes padding;
    ABE_EdgeSizes border;
    ABE_Rect overflow_box;
    bool is_anonymous;
    bool is_inline;
    bool is_flex;
    bool is_dirty;
    ABE_VisibilityType visibility;
} ABE_LayoutBoxInfo;

// Config structure
typedef struct {
    char     user_agent[256];
    char     default_home_url[256];
    uint32_t max_tabs_per_window;
    uint32_t max_windows;
    size_t   max_memory_pool_bytes;
    bool     enable_diagnostics;
    bool     enable_cache;
    uint32_t log_level;

    // Network Config
    uint32_t dns_timeout_ms;
    uint32_t connect_timeout_ms;
    uint32_t response_timeout_ms;
    uint32_t max_connections_per_host;
    uint32_t max_total_connections;
    uint32_t max_redirects;
    bool     enable_gzip;
    bool     enable_keep_alive;

    // HTML5 Parser Config
    bool     enable_scripting;
    bool     strict_doctype;
    uint32_t max_dom_depth;
    uint32_t max_dom_nodes;

    // CSS Config
    bool     enable_user_agent_stylesheet;
    bool     enable_css_custom_properties;
    uint32_t max_stylesheets_per_doc;

    // Layout Config
    float default_viewport_width;
    float default_viewport_height;
    bool  enable_flexbox;
    bool  enable_incremental_reflow;

    // JS Runtime Config
    size_t   js_heap_size_bytes;
    uint32_t js_max_call_stack_depth;
    bool     enable_es_modules;
    bool     enable_gc;

    // Web Platform Config
    bool     enable_local_storage;
    bool     enable_cookies;
    uint32_t max_storage_bytes;
} ABE_Config;

// Diagnostics Metrics
typedef struct {
    uint32_t total_windows_created;
    uint32_t active_windows;
    uint32_t total_tabs_created;
    uint32_t active_tabs;
    uint32_t total_navigations;
    uint32_t urls_parsed_count;
    size_t   allocated_memory_bytes;
    size_t   peak_memory_bytes;
    uint32_t active_resource_handles;
    uint32_t memory_pool_block_count;
    uint32_t parse_time_us;
    uint32_t render_time_us;

    // Network Telemetry
    uint32_t dns_resolutions_total;
    uint32_t dns_cache_hits;
    uint32_t active_connections;
    uint32_t reused_connections;
    uint32_t http_requests_sent;
    uint32_t http_responses_rcvd;
    uint32_t redirects_followed;
    uint32_t total_bytes_downloaded;
    uint32_t total_bytes_uploaded;
    uint32_t dns_lookup_time_us;
    uint32_t tcp_connect_time_us;
    uint32_t tls_handshake_time_us;
    uint32_t ttfb_us;

    // HTML5 & DOM Telemetry
    uint32_t documents_parsed_total;
    uint32_t dom_nodes_allocated;
    uint32_t dom_nodes_active;
    uint32_t dom_tree_max_depth;
    uint32_t html_tokens_generated;
    uint32_t html_parse_errors_recovered;
    uint32_t html_parse_time_us;

    // CSS Engine Telemetry
    uint32_t stylesheets_parsed_total;
    uint32_t css_rules_total;
    uint32_t css_selectors_matched;
    uint32_t css_styles_computed_total;
    uint32_t css_parse_errors_recovered;
    uint32_t css_compute_time_us;

    // Layout Engine Telemetry
    uint32_t render_trees_built_total;
    uint32_t render_nodes_active;
    uint32_t layouts_performed_total;
    uint32_t reflows_performed_total;
    uint32_t dirty_nodes_processed;
    uint32_t layout_time_us;

    // JS Runtime Telemetry
    uint32_t js_contexts_created;
    uint32_t js_scripts_executed;
    uint32_t js_bytecode_instructions_executed;
    uint32_t js_gc_runs_total;
    size_t   js_heap_allocated_bytes;
    uint32_t js_promises_resolved;
    uint32_t js_event_loop_ticks;
    uint32_t js_execution_time_us;

    // Web Platform Telemetry
    uint32_t fetch_requests_total;
    uint32_t xhr_requests_total;
    uint32_t storage_read_ops;
    uint32_t storage_write_ops;
    uint32_t animation_frames_requested;
    uint32_t mutation_observer_triggers;
} ABE_DiagnosticsMetrics;

// DOM Attribute Info
typedef struct {
    char name[64];
    char value[256];
} ABE_DOMAttributeInfo;

// DOM Node Info
typedef struct {
    ABE_NodeHandle handle;
    ABE_NodeType type;
    char tag_name[64];
    char node_value[512];
    uint32_t child_count;
    ABE_NodeHandle parent;
    ABE_NodeHandle first_child;
    ABE_NodeHandle last_child;
    ABE_NodeHandle prev_sibling;
    ABE_NodeHandle next_sibling;
    ABE_DOMAttributeInfo attributes[ABE_MAX_ATTRIBUTES];
    uint32_t attribute_count;
} ABE_DOMNodeInfo;

// Tab Info
typedef struct {
    ABE_TabHandle handle;
    ABE_WindowHandle window_handle;
    char title[ABE_MAX_TITLE_LEN];
    char url[ABE_MAX_URL_LEN];
    bool is_active;
    bool is_loading;
    uint32_t state;
    uint32_t load_progress;
} ABE_TabInfo;

// Window Info
typedef struct {
    ABE_WindowHandle handle;
    uint32_t bosurface_id;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    bool is_fullscreen;
    uint32_t tab_count;
    ABE_TabHandle active_tab;
} ABE_WindowInfo;

// HTTP Header
typedef struct {
    char name[64];
    char value[256];
} ABE_HTTPHeader;

// HTTP Method
typedef enum {
    ABE_HTTP_METHOD_GET = 0,
    ABE_HTTP_METHOD_HEAD = 1,
    ABE_HTTP_METHOD_POST = 2,
    ABE_HTTP_METHOD_OPTIONS = 3
} ABE_HTTPMethod;

// HTTP Request
typedef struct {
    ABE_HTTPMethod method;
    char url[ABE_MAX_URL_LEN];
    ABE_HTTPHeader headers[ABE_MAX_HEADERS];
    uint32_t header_count;
    uint8_t* body_data;
    size_t body_len;
} ABE_HTTPRequest;

// HTTP Response
typedef struct {
    uint32_t status_code;
    char status_text[64];
    ABE_HTTPHeader headers[ABE_MAX_HEADERS];
    uint32_t header_count;
    uint8_t* body_data;
    size_t body_len;
    bool is_chunked;
    bool is_gzipped;
    bool is_keep_alive;
} ABE_HTTPResponse;

// Download Progress Callback Prototype
typedef void (*ABE_DownloadProgressCallback)(ABE_DownloadHandle handle, size_t bytes_received, size_t total_bytes, void* user_data);

// Core Lifecycle APIs
ABE_Error ABE_Initialize(const ABE_Config* config);
ABE_Error ABE_Shutdown(void);
bool      ABE_IsInitialized(void);

// Configuration & Default Helper
void      ABE_GetDefaultConfig(ABE_Config* out_config);

// Browser Window APIs
ABE_Error ABE_CreateWindow(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height, ABE_WindowHandle* out_window);
ABE_Error ABE_DestroyWindow(ABE_WindowHandle window);
ABE_Error ABE_ResizeWindow(ABE_WindowHandle window, uint32_t width, uint32_t height);
ABE_Error ABE_SetWindowFullscreen(ABE_WindowHandle window, bool fullscreen);
ABE_Error ABE_GetWindowInfo(ABE_WindowHandle window, ABE_WindowInfo* out_info);

// Tab Engine APIs
ABE_Error ABE_CreateTab(ABE_WindowHandle window, const char* initial_url, ABE_TabHandle* out_tab);
ABE_Error ABE_CloseTab(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_SelectTab(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_GetActiveTab(ABE_WindowHandle window, ABE_TabHandle* out_tab);
ABE_Error ABE_GetTabInfo(ABE_TabHandle tab, ABE_TabInfo* out_info);

// Navigation Engine APIs
ABE_Error ABE_LoadURL(ABE_WindowHandle window, ABE_TabHandle tab, const char* url);
ABE_Error ABE_Reload(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_Stop(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_GoBack(ABE_WindowHandle window, ABE_TabHandle tab);
ABE_Error ABE_GoForward(ABE_WindowHandle window, ABE_TabHandle tab);
bool      ABE_CanGoBack(ABE_TabHandle tab);
bool      ABE_CanGoForward(ABE_TabHandle tab);

// Phase 2 Networking Public APIs
ABE_Error ABE_NetworkInitialize(void);
ABE_Error ABE_NetworkShutdown(void);
ABE_Error ABE_DNSResolve(const char* hostname, uint32_t* out_ip);
ABE_Error ABE_OpenConnection(const char* host, uint16_t port, bool use_tls, ABE_ConnHandle* out_conn);
ABE_Error ABE_CloseConnection(ABE_ConnHandle conn);
ABE_Error ABE_SendHTTPRequest(ABE_ConnHandle conn, const ABE_HTTPRequest* req, ABE_RequestHandle* out_req);
ABE_Error ABE_ReadHTTPResponse(ABE_RequestHandle req, ABE_HTTPResponse* out_resp);
ABE_Error ABE_FreeHTTPResponse(ABE_HTTPResponse* resp);
ABE_Error ABE_DownloadResource(const char* url, ABE_DownloadProgressCallback cb, void* user_data, ABE_DownloadHandle* out_dl);
ABE_Error ABE_CancelRequest(ABE_RequestHandle req);

// Phase 3 HTML5 & DOM Public APIs
ABE_Error ABE_HTMLInitialize(void);
ABE_Error ABE_HTMLShutdown(void);
ABE_Error ABE_ParseHTML(const char* html_str, size_t len, ABE_DocumentHandle* out_doc);
ABE_Error ABE_ParseHTMLStream(ABE_RequestHandle net_req, ABE_DocumentHandle* out_doc);
ABE_Error ABE_CreateDocument(ABE_DocumentHandle* out_doc);
ABE_Error ABE_DestroyDocument(ABE_DocumentHandle doc);
ABE_Error ABE_GetDocumentElement(ABE_DocumentHandle doc, ABE_NodeHandle* out_node);
ABE_Error ABE_GetBody(ABE_DocumentHandle doc, ABE_NodeHandle* out_node);
ABE_Error ABE_FindElementById(ABE_DocumentHandle doc, const char* id, ABE_NodeHandle* out_node);
ABE_Error ABE_FindElementsByTag(ABE_DocumentHandle doc, const char* tag_name, ABE_NodeHandle* out_buf, uint32_t max_buf, uint32_t* out_count);
ABE_Error ABE_CreateElement(ABE_DocumentHandle doc, const char* tag_name, ABE_NodeHandle* out_node);
ABE_Error ABE_CreateTextNode(ABE_DocumentHandle doc, const char* text, ABE_NodeHandle* out_node);
ABE_Error ABE_AppendChild(ABE_NodeHandle parent, ABE_NodeHandle child);
ABE_Error ABE_InsertBefore(ABE_NodeHandle parent, ABE_NodeHandle new_node, ABE_NodeHandle ref_node);
ABE_Error ABE_RemoveChild(ABE_NodeHandle parent, ABE_NodeHandle child);
ABE_Error ABE_GetNodeInfo(ABE_NodeHandle node, ABE_DOMNodeInfo* out_info);
ABE_Error ABE_SetAttribute(ABE_NodeHandle node, const char* name, const char* value);
ABE_Error ABE_GetAttribute(ABE_NodeHandle node, const char* name, char* out_buf, size_t max_len);

// Phase 4 CSS & Styled DOM Public APIs
ABE_Error ABE_CSSInitialize(void);
ABE_Error ABE_CSSShutdown(void);
ABE_Error ABE_ParseStylesheet(const char* css_str, size_t len, ABE_StylesheetHandle* out_sheet);
ABE_Error ABE_LoadStylesheet(ABE_DocumentHandle doc, ABE_StylesheetHandle sheet);
ABE_Error ABE_ComputeStyles(ABE_DocumentHandle doc);
ABE_Error ABE_GetComputedStyle(ABE_NodeHandle node, ABE_ComputedStyle* out_style);
ABE_Error ABE_MatchSelectors(ABE_NodeHandle node, const char* selector_str, bool* out_matched);
ABE_Error ABE_RecalculateStyles(ABE_DocumentHandle doc);
ABE_Error ABE_DestroyStylesheet(ABE_StylesheetHandle sheet);

// Phase 5 Layout & Render Tree Public APIs
ABE_Error ABE_LayoutInitialize(void);
ABE_Error ABE_LayoutShutdown(void);
ABE_Error ABE_BuildRenderTree(ABE_DocumentHandle doc, ABE_RenderTreeHandle* out_tree);
ABE_Error ABE_PerformLayout(ABE_RenderTreeHandle tree, float viewport_width, float viewport_height);
ABE_Error ABE_Reflow(ABE_RenderTreeHandle tree);
ABE_Error ABE_GetRenderTree(ABE_DocumentHandle doc, ABE_RenderTreeHandle* out_tree);
ABE_Error ABE_GetLayoutBox(ABE_RenderTreeHandle tree, ABE_NodeHandle node, ABE_LayoutBoxInfo* out_info);
ABE_Error ABE_MarkDirty(ABE_RenderTreeHandle tree, ABE_NodeHandle node);
ABE_Error ABE_DestroyRenderTree(ABE_RenderTreeHandle tree);

// Phase 7 ECMAScript Runtime Public APIs
ABE_Error ABE_JSInitialize(void);
ABE_Error ABE_JSShutdown(void);
ABE_Error ABE_CreateContext(ABE_DocumentHandle doc, ABE_JSContextHandle* out_ctx);
ABE_Error ABE_DestroyContext(ABE_JSContextHandle ctx);
ABE_Error ABE_ExecuteScript(ABE_JSContextHandle ctx, const char* src, size_t len, ABE_JSValue* out_val);
ABE_Error ABE_CompileScript(const char* src, size_t len, ABE_JSScriptHandle* out_script);
ABE_Error ABE_RunEventLoop(ABE_JSContextHandle ctx);
ABE_Error ABE_GarbageCollect(ABE_JSContextHandle ctx);

// ============================================================
// Phase 8 — Production Web Platform Public APIs
// ============================================================
ABE_Error ABE_WebPlatformInitialize(void);
ABE_Error ABE_WebPlatformShutdown(void);

ABE_Error ABE_Fetch(ABE_JSContextHandle ctx, const char* url, const ABE_HTTPRequest* init, ABE_FetchRequestHandle* out_req);
ABE_Error ABE_XMLHttpRequest_Create(ABE_XHRHandle* out_xhr);
ABE_Error ABE_CreateURL(const char* url_str, ABE_URLHandle* out_url);
ABE_Error ABE_GetLocalStorage(ABE_StorageHandle* out_storage);
ABE_Error ABE_GetSessionStorage(ABE_StorageHandle* out_storage);
ABE_Error ABE_Storage_SetItem(ABE_StorageHandle storage, const char* key, const char* val);
ABE_Error ABE_Storage_GetItem(ABE_StorageHandle storage, const char* key, char* out_buf, size_t max_len);
ABE_Error ABE_RequestAnimationFrame(ABE_JSContextHandle ctx, void (*cb)(void*), void* user_data, uint32_t* out_id);
ABE_Error ABE_CreateBlob(const uint8_t* data, size_t len, const char* mime_type, ABE_BlobHandle* out_blob);
ABE_Error ABE_CreateMutationObserver(ABE_ObserverHandle* out_obs);

// Diagnostics & Telemetry APIs
ABE_Error   ABE_GetDiagnosticsMetrics(ABE_DiagnosticsMetrics* out_metrics);
const char* ABE_GetErrorString(ABE_Error error);
void        ABE_DumpDiagnostics(char* buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_ABE_H
