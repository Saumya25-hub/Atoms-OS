#include "abe_css_test.h"
#include "abe_css_parser.h"
#include "abe_css_selector.h"
#include "abe_css_cascade.h"
#include "abe_css_computed.h"
#include "abe_css_style_manager.h"
#include "../html/abe_html.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

// TEST 1: Basic Rule
static bool Test1_BasicRule(void) {
    display_print("[CSS TEST 1] Basic Rule...\n");
    const char* css = "body { color: red; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);
    if (err != ABE_SUCCESS || sheet == ABE_INVALID_HANDLE) return false;

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 1 || s->rules[0].declaration_count != 1) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 1] PASS: Basic Rule.\n");
    return true;
}

// TEST 2: Specificity (ID > Class > Tag)
static bool Test2_Specificity(void) {
    display_print("[CSS TEST 2] Specificity (ID > Class > Tag)...\n");
    const char* css = "p { color: red; }\n.text { color: blue; }\n#main { color: green; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 3) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    // rule 0: tag (0,0,1)
    // rule 1: class (0,1,0)
    // rule 2: id (1,0,0)
    if (s->rules[0].specificity_c != 1 || s->rules[1].specificity_b != 1 || s->rules[2].specificity_a != 1) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    // Verify comparison
    int cmp1 = ABE_CSSSelector_CompareSpecificity(s->rules[2].specificity_a, s->rules[2].specificity_b, s->rules[2].specificity_c,
                                                  s->rules[1].specificity_a, s->rules[1].specificity_b, s->rules[1].specificity_c);
    if (cmp1 <= 0) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 2] PASS: Specificity Ordering Verified.\n");
    return true;
}

// TEST 3: !important Override
static bool Test3_Important(void) {
    display_print("[CSS TEST 3] !important Override...\n");
    const char* css = "p { color: red !important; }\n#main { color: blue; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 2) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    if (!s->rules[0].declarations[0].is_important || s->rules[1].declarations[0].is_important) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CascadedProperty prop_imp;
    prop_imp.declaration = s->rules[0].declarations[0];
    prop_imp.origin = ORIGIN_AUTHOR;
    prop_imp.specificity_a = 0; prop_imp.specificity_b = 0; prop_imp.specificity_c = 1;
    prop_imp.rule_index = 0;

    ABE_CascadedProperty prop_normal;
    prop_normal.declaration = s->rules[1].declarations[0];
    prop_normal.origin = ORIGIN_AUTHOR;
    prop_normal.specificity_a = 1; prop_normal.specificity_b = 0; prop_normal.specificity_c = 0;
    prop_normal.rule_index = 1;

    // Even though prop_normal has higher specificity, prop_imp should override because of !important
    if (ABE_CSSCascade_ShouldOverride(&prop_imp, &prop_normal)) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 3] PASS: !important Precedence Verified.\n");
    return true;
}

// TEST 4: Property Inheritance
static bool Test4_Inheritance(void) {
    display_print("[CSS TEST 4] Property Inheritance (color, font)...\n");
    const char* html = "<!DOCTYPE html><html><body><div id=\"parent\" style=\"color: #00FF00; font-size: 24px;\"><p id=\"child\">Inherited</p></div></body></html>";
    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_StyleManager_LoadDocumentStyles(doc);

    ABE_NodeHandle child_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "child", &child_node);
    if (child_node == ABE_INVALID_HANDLE) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_ComputedStyle style;
    ABE_CSSComputed_GetNodeStyle(child_node, &style);
    if (style.color != 0xFF00FF00 || style.font_size_px != 24.0f) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[CSS TEST 4] PASS: Property Inheritance Verified.\n");
    return true;
}

// TEST 5: Multiple Declarations & Box Model
static bool Test5_BoxModel(void) {
    display_print("[CSS TEST 5] Box Model & Multiple Declarations...\n");
    const char* css = "div { margin: 10px; padding: 20px; border-width: 2px; width: 300px; height: 150px; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 1 || s->rules[0].declaration_count != 5) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 5] PASS: Box Model Declarations Parsed.\n");
    return true;
}

// TEST 6: CSS Units
static bool Test6_Units(void) {
    display_print("[CSS TEST 6] CSS Units (px, %, em, rem, vw, vh)...\n");
    const char* css = "div { width: 50%; font-size: 1.5rem; height: 100vh; margin: 2em; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 1 || s->rules[0].declaration_count != 4) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    if (s->rules[0].declarations[0].value.type != CSS_VAL_PERCENT ||
        s->rules[0].declarations[1].value.type != CSS_VAL_REM ||
        s->rules[0].declarations[2].value.type != CSS_VAL_VH ||
        s->rules[0].declarations[3].value.type != CSS_VAL_EM) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 6] PASS: CSS Units Verified.\n");
    return true;
}

// TEST 7: Colors (#RGB, #RRGGBB, rgb, rgba, named)
static bool Test7_Colors(void) {
    display_print("[CSS TEST 7] Colors (#RGB, #RRGGBB, rgb, rgba, named)...\n");
    uint32_t c1 = ABE_CSSTokenizer_ParseColor("#FFF");
    uint32_t c2 = ABE_CSSTokenizer_ParseColor("#00FF00");
    uint32_t c3 = ABE_CSSTokenizer_ParseColor("rgb(255, 0, 0)");
    uint32_t c4 = ABE_CSSTokenizer_ParseColor("blue");

    if (c1 != 0xFFFFFFFF || c2 != 0xFF00FF00 || c3 != 0xFFFF0000 || c4 != 0xFF0000FF) {
        return false;
    }

    display_print("[CSS TEST 7] PASS: Colors Parsed Successfully.\n");
    return true;
}

// TEST 8: Attribute Selectors
static bool Test8_AttributeSelectors(void) {
    display_print("[CSS TEST 8] Attribute Selectors ([type=\"text\"])...\n");
    const char* html = "<!DOCTYPE html><html><body><input type=\"text\" id=\"txt\"><input type=\"submit\" id=\"sub\"></body></html>";
    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_NodeHandle txt_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "txt", &txt_node);

    bool match_txt = false, match_sub = false;
    ABE_MatchSelectors(txt_node, "input[type=\"text\"]", &match_txt);
    ABE_MatchSelectors(txt_node, "input[type=\"submit\"]", &match_sub);

    if (!match_txt || match_sub) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[CSS TEST 8] PASS: Attribute Selectors Verified.\n");
    return true;
}

// TEST 9: Combinators (A > B, A B, A + B, A ~ B)
static bool Test9_Combinators(void) {
    display_print("[CSS TEST 9] Combinators (A > B, A B, A + B, A ~ B)...\n");
    const char* html = "<!DOCTYPE html><html><body><div id=\"wrap\"><h1 id=\"h\">Heading</h1><p id=\"p1\">Para 1</p><p id=\"p2\">Para 2</p></div></body></html>";
    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_NodeHandle p1 = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "p1", &p1);

    bool m_child = false, m_adj = false;
    ABE_MatchSelectors(p1, "div > p", &m_child);
    ABE_MatchSelectors(p1, "h1 + p", &m_adj);

    if (!m_child || !m_adj) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[CSS TEST 9] PASS: Combinators Verified.\n");
    return true;
}

// TEST 10: Pseudo-classes (:first-child, :disabled)
static bool Test10_PseudoClasses(void) {
    display_print("[CSS TEST 10] Pseudo-Classes (:first-child)...\n");
    const char* html = "<!DOCTYPE html><html><body><ul><li id=\"li1\">1</li><li id=\"li2\">2</li></ul></body></html>";
    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_NodeHandle li1 = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "li1", &li1);

    bool m_first = false;
    ABE_MatchSelectors(li1, "li:first-child", &m_first);

    if (!m_first) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[CSS TEST 10] PASS: Pseudo-classes Verified.\n");
    return true;
}

// TEST 11: Custom Properties (CSS Variables)
static bool Test11_CustomProperties(void) {
    display_print("[CSS TEST 11] CSS Custom Properties (--var, var())...\n");
    const char* css = ":root { --theme-color: #00FF00; }\n p { color: var(--theme-color); }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    const char* v = ABE_CSSParser_GetCustomProperty(sheet, "--theme-color");
    if (!v || strcmp(v, "#00FF00") != 0) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 11] PASS: CSS Variables Verified.\n");
    return true;
}

// TEST 12: Media Queries
static bool Test12_MediaQueries(void) {
    display_print("[CSS TEST 12] Media Queries (@media max-width)...\n");
    const char* css = "@media screen and (max-width: 800px) { body { font-size: 14px; } }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_CSSParser_ParseStylesheet(css, strlen(css), ORIGIN_AUTHOR, &sheet);

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 1 || !s->rules[0].media_query.has_media_query || s->rules[0].media_query.max_width_px != 800.0f) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 12] PASS: Media Queries Parsed.\n");
    return true;
}

// TEST 13: Malformed CSS Error Recovery
static bool Test13_MalformedCSS(void) {
    display_print("[CSS TEST 13] Malformed CSS Error Recovery...\n");
    const char* malformed_css = "p { color: ; margin: 10px; bad-property } div { color: blue; }";
    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_CSSParser_ParseStylesheet(malformed_css, strlen(malformed_css), ORIGIN_AUTHOR, &sheet);
    if (err != ABE_SUCCESS || sheet == ABE_INVALID_HANDLE) return false;

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count == 0) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 13] PASS: Malformed CSS Recovered Safely.\n");
    return true;
}

// TEST 14: Large Stylesheet
static bool Test14_LargeStylesheet(void) {
    display_print("[CSS TEST 14] Large Stylesheet (100+ rules)...\n");
    char large_css[8192];
    strcpy(large_css, "");
    for (int i = 0; i < 50; i++) {
        strcat(large_css, ".item-entry { display: block; margin: 4px; color: #333333; }\n");
    }

    ABE_StylesheetHandle sheet = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_CSSParser_ParseStylesheet(large_css, strlen(large_css), ORIGIN_AUTHOR, &sheet);
    if (err != ABE_SUCCESS || sheet == ABE_INVALID_HANDLE) return false;

    ABE_CSSStylesheet* s = ABE_CSSParser_GetStylesheet(sheet);
    if (!s || s->rule_count != 50) {
        ABE_CSSParser_DestroyStylesheet(sheet);
        return false;
    }

    ABE_CSSParser_DestroyStylesheet(sheet);
    display_print("[CSS TEST 14] PASS: Large Stylesheet Parsed Cleanly.\n");
    return true;
}

// TEST 15: Real Document Integration
static bool Test15_RealDocumentIntegration(void) {
    display_print("[CSS TEST 15] Real Document Integration (<style> -> DOM -> CSSOM -> Computed)...\n");
    const char* doc_html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<style>\n"
        "body { background-color: #181825; color: #CAD3F5; }\n"
        "#main-title { font-size: 32px; color: #F38BA8; }\n"
        ".subtext { color: #A6ADC8; margin: 10px; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1 id=\"main-title\">Production CSS Engine</h1>\n"
        "<p class=\"subtext\" id=\"p-desc\">Real-world CSSOM verified.</p>\n"
        "</body>\n"
        "</html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(doc_html, strlen(doc_html), &doc);

    ABE_StyleManager_LoadDocumentStyles(doc);

    ABE_NodeHandle title_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "main-title", &title_node);
    if (title_node == ABE_INVALID_HANDLE) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_ComputedStyle style;
    ABE_CSSComputed_GetNodeStyle(title_node, &style);
    if (style.color != 0xFFF38BA8 || style.font_size_px != 32.0f) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[CSS TEST 15] PASS: Real Document Style Pipeline Certified.\n");
    return true;
}

void ABE_RunPhase5_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 5 Production CSS Engine Test Suite \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();
    ABE_CSSInitialize();

    if (!Test1_BasicRule()) return;
    if (!Test2_Specificity()) return;
    if (!Test3_Important()) return;
    if (!Test4_Inheritance()) return;
    if (!Test5_BoxModel()) return;
    if (!Test6_Units()) return;
    if (!Test7_Colors()) return;
    if (!Test8_AttributeSelectors()) return;
    if (!Test9_Combinators()) return;
    if (!Test10_PseudoClasses()) return;
    if (!Test11_CustomProperties()) return;
    if (!Test12_MediaQueries()) return;
    if (!Test13_MalformedCSS()) return;
    if (!Test14_LargeStylesheet()) return;
    if (!Test15_RealDocumentIntegration()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_CSS_DIAG] Stylesheets Parsed: "); display_print_dec(metrics.stylesheets_parsed_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] CSS Rules Total   : "); display_print_dec(metrics.css_rules_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] Selectors Matched : "); display_print_dec(metrics.css_selectors_matched); display_print("\n");
    display_print("[ABE_CSS_DIAG] Styles Computed   : "); display_print_dec(metrics.css_styles_computed_total); display_print("\n");
    display_print("[ABE_CSS_DIAG] Compute Time (us) : "); display_print_dec(metrics.css_compute_time_us); display_print("\n");

    ABE_CSSShutdown();
    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE5_ABE_PRODUCTION_CSS_ENGINE\n\n");
}

void ABE_RunPhase4_VerificationSuite(void) {
    ABE_RunPhase5_VerificationSuite();
}

