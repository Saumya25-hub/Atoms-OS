# ATRIX Browser Engine v1.0 — API Reference

## Public Engine C API

- `void ATRIX_BrowserEngine_Init(void)`: Initialize all engine subsystems.
- `void ATRIX_BrowserEngine_GetMetrics(ATRIX_BrowserEngineMetrics* out_metrics)`: Query runtime memory and tab telemetry.
- `ATRIX_BrowserTab* ATRIX_TabManager_CreateTab(const char* initial_url)`: Open a new tab session.
- `bool ATRIX_TabManager_CloseTab(uint32_t tab_id)`: Close tab session.
- `DOMNode* ATRIX_HTMLParser_ParseString(const char* html_str)`: Parse HTML into DOM Tree.
- `CSSStyleSheet* ATRIX_CSSParser_ParseString(const char* css_str)`: Parse CSS stylesheet rules.
- `bool ATRIX_JS_ExecuteScript(const char* script_code)`: Execute JavaScript string in Stack VM.
