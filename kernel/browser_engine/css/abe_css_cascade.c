#include "abe_css_cascade.h"
#include "abe_css_selector.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_StylesheetHandle g_ua_stylesheet_handle = ABE_INVALID_HANDLE;
static bool g_cascade_initialized = false;

static const char* g_default_ua_css =
    "html { display: block; }\n"
    "body { display: block; margin: 8px; font-size: 16px; font-family: sans-serif; color: #000000; background-color: #FFFFFF; }\n"
    "div { display: block; }\n"
    "p { display: block; margin: 16px; }\n"
    "h1 { display: block; font-size: 32px; font-weight: 700; margin: 21px; }\n"
    "h2 { display: block; font-size: 24px; font-weight: 700; margin: 19px; }\n"
    "h3 { display: block; font-size: 18px; font-weight: 700; margin: 18px; }\n"
    "h4 { display: block; font-size: 16px; font-weight: 700; }\n"
    "h5 { display: block; font-size: 13px; font-weight: 700; }\n"
    "h6 { display: block; font-size: 10px; font-weight: 700; }\n"
    "a { color: #0000FF; }\n"
    "b { font-weight: 700; }\n"
    "strong { font-weight: 700; }\n"
    "table { display: table; }\n"
    "tr { display: table; }\n"
    "td { display: table; }\n"
    "ul { display: block; padding: 40px; }\n"
    "ol { display: block; padding: 40px; }\n"
    "li { display: block; }\n";

ABE_Error ABE_CSSCascade_Init(void) {
    if (g_cascade_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_Error err = ABE_CSSParser_ParseStylesheet(g_default_ua_css, strlen(g_default_ua_css), ORIGIN_USER_AGENT, &g_ua_stylesheet_handle);
    if (err != ABE_SUCCESS) return err;

    g_cascade_initialized = true;
    ABE_Log(ABE_LOG_INFO, "CASCADE", "ABE Cascade Engine initialized with default User-Agent stylesheet");
    return ABE_SUCCESS;
}

ABE_Error ABE_CSSCascade_Shutdown(void) {
    if (!g_cascade_initialized) return ABE_ERR_NOT_INITIALIZED;
    if (g_ua_stylesheet_handle != ABE_INVALID_HANDLE) {
        ABE_CSSParser_DestroyStylesheet(g_ua_stylesheet_handle);
        g_ua_stylesheet_handle = ABE_INVALID_HANDLE;
    }
    g_cascade_initialized = false;
    return ABE_SUCCESS;
}

ABE_StylesheetHandle ABE_CSSCascade_GetUserAgentStylesheet(void) {
    return g_ua_stylesheet_handle;
}

bool ABE_CSSCascade_ShouldOverride(const ABE_CascadedProperty* existing, const ABE_CascadedProperty* incoming) {
    if (!existing) return true;
    if (!incoming) return false;

    // 1. Importance Check
    if (incoming->declaration.is_important && !existing->declaration.is_important) return true;
    if (!incoming->declaration.is_important && existing->declaration.is_important) return false;

    // 2. Origin Check: Inline > Author > User-Agent
    if (incoming->origin > existing->origin) return true;
    if (incoming->origin < existing->origin) return false;

    // 3. Specificity Comparison (a, b, c)
    int spec_diff = ABE_CSSSelector_CompareSpecificity(incoming->specificity_a, incoming->specificity_b, incoming->specificity_c,
                                                        existing->specificity_a, existing->specificity_b, existing->specificity_c);
    if (spec_diff > 0) return true;
    if (spec_diff < 0) return false;

    // 4. Source Order (Later rules override earlier rules)
    return (incoming->rule_index >= existing->rule_index);
}
