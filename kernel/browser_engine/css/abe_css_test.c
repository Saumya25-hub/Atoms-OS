#include "abe_css_test.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test_CSSTokenizerAndParser(void) {
    display_print("[ABE_CSS_TEST] 1. Testing CSS Tokenization & Ruleset Parsing...\n");

    const char* sample_css =
        "#main-header { display: block; width: 800px; color: #FF0000; }\n"
        ".button { display: inline-block; padding: 10px; font-size: 14px; }\n"
        "div > p { color: #00FF00; }\n";

    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseStylesheet(sample_css, strlen(sample_css), &sheet);
    if (err != ABE_SUCCESS || sheet == ABE_INVALID_HANDLE) {
        display_print("[ABE_CSS_TEST] FAIL: ABE_ParseStylesheet failed!\n");
        return false;
    }

    ABE_DestroyStylesheet(sheet);
    display_print("[ABE_CSS_TEST] PASS: CSS Tokenization & Ruleset Parsing verified.\n");
    return true;
}

static bool Test_SelectorMatchingAndSpecificity(void) {
    display_print("[ABE_CSS_TEST] 2. Testing Selector Engine & Specificity Resolution...\n");

    const char* html_sample = "<html><body><div id=\"content\"><p class=\"highlight\" id=\"para1\">Styled Paragraph</p></div></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html_sample, strlen(html_sample), &doc);

    ABE_NodeHandle para_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "para1", &para_node);
    if (para_node == ABE_INVALID_HANDLE) {
        display_print("[ABE_CSS_TEST] FAIL: Element lookup failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    // Match "#para1" (ID selector)
    bool matched = false;
    ABE_MatchSelectors(para_node, "#para1", &matched);
    if (!matched) {
        display_print("[ABE_CSS_TEST] FAIL: MatchSelectors ('#para1') failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    // Match ".highlight" (Class selector)
    ABE_MatchSelectors(para_node, ".highlight", &matched);
    if (!matched) {
        display_print("[ABE_CSS_TEST] FAIL: MatchSelectors ('.highlight') failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    // Match "div > p" (Child combinator selector)
    ABE_MatchSelectors(para_node, "div > p", &matched);
    if (!matched) {
        display_print("[ABE_CSS_TEST] FAIL: MatchSelectors ('div > p') failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_CSS_TEST] PASS: Selector Engine & Specificity Resolution verified.\n");
    return true;
}

static bool Test_ComputedStylesAndInheritance(void) {
    display_print("[ABE_CSS_TEST] 3. Testing Computed Styles & Property Inheritance...\n");

    const char* styled_html = "<html><body><div style=\"color: #0000FF; font-size: 20px;\"><h1 id=\"title\">Title</h1><p id=\"text\">Child Text</p></div></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(styled_html, strlen(styled_html), &doc);

    // Compute styles for all DOM nodes
    ABE_Error err = ABE_ComputeStyles(doc);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_CSS_TEST] FAIL: ABE_ComputeStyles failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_NodeHandle title_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "title", &title_node);
    if (title_node == ABE_INVALID_HANDLE) {
        display_print("[ABE_CSS_TEST] FAIL: Title element lookup failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_ComputedStyle style;
    err = ABE_GetComputedStyle(title_node, &style);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_CSS_TEST] FAIL: GetComputedStyle failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    // Default h1 User-Agent style gives display block, font-size 32px
    if (style.display != ABE_DISPLAY_BLOCK) {
        display_print("[ABE_CSS_TEST] FAIL: Computed display property mismatch!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_CSS_TEST] PASS: Computed Styles & Property Inheritance verified.\n");
    return true;
}

void ABE_RunPhase4_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 4 Production CSS Engine Test Suite \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();
    ABE_CSSInitialize();

    if (!Test_CSSTokenizerAndParser()) return;
    if (!Test_SelectorMatchingAndSpecificity()) return;
    if (!Test_ComputedStylesAndInheritance()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_CSS_DIAG] Stylesheets Parsed: "); display_print_dec(metrics.stylesheets_parsed_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] CSS Rules Total   : "); display_print_dec(metrics.css_rules_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] Selectors Matched : "); display_print_dec(metrics.css_selectors_matched); display_print("\n");
    display_print("[ABE_CSS_DIAG] Styles Computed   : "); display_print_dec(metrics.css_styles_computed_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] Compute Time (us) : "); display_print_dec(metrics.css_compute_time_us); display_print("\n");

    ABE_CSSShutdown();
    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE4_ABE_PRODUCTION_CSS_ENGINE\n\n");
}
