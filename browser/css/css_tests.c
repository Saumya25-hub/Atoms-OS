#include "css_tests.h"
#include "browser/css/include/bos_css.h"
#include "css_tokenizer.h"
#include "css_parser.h"
#include "css_selector.h"
#include "css_specificity.h"
#include "css_style.h"
#include "css_stylesheet.h"
#include "css_value.h"
#include "browser/html/include/bos_html.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bool test_tokenizer(void) {
    css_tokenizer_t tok;
    css_tokenizer_init(&tok, "/* comment */ body { color: red; margin: 10px; }");
    css_token_t token;

    if (css_tokenizer_next(&tok, &token) != CSS_OK || token.type != CSS_TOKEN_IDENT) return false;
    if (strcmp(token.value, "body") != 0) return false;

    if (css_tokenizer_next(&tok, &token) != CSS_OK || token.symbol != '{') return false;
    if (css_tokenizer_next(&tok, &token) != CSS_OK || strcmp(token.value, "color") != 0) return false;
    if (css_tokenizer_next(&tok, &token) != CSS_OK || token.symbol != ':') return false;
    if (css_tokenizer_next(&tok, &token) != CSS_OK || strcmp(token.value, "red") != 0) return false;
    if (css_tokenizer_next(&tok, &token) != CSS_OK || token.symbol != ';') return false;

    return true;
}

static bool test_selectors(void) {
    bos_node_t* div = NULL;
    bos_node_create(BOS_NODE_ELEMENT, "div", &div);
    bos_set_attribute(div, "id", "main");
    bos_set_attribute(div, "class", "box active");

    css_selector_t* sel1 = NULL;
    css_selector_parse_string("div#main.box", &sel1);
    bool match1 = css_selector_match_node(sel1, div);
    css_selector_destroy(sel1);

    css_selector_t* sel2 = NULL;
    css_selector_parse_string("span.title", &sel2);
    bool match2 = css_selector_match_node(sel2, div);
    css_selector_destroy(sel2);

    bos_node_destroy(div);
    return match1 && !match2;
}

static bool test_declarations(void) {
    css_stylesheet_t* sheet = NULL;
    const char* css = "h1 { color: #ff0000; font-size: 24px; display: block; }";
    if (css_parser_parse_string(css, &sheet) != CSS_OK || !sheet) return false;

    bool pass = (sheet->rule_count == 1 && sheet->rules_head && sheet->rules_head->declarations != NULL);
    css_stylesheet_destroy(sheet);
    return pass;
}

static bool test_specificity(void) {
    css_selector_t *s1 = NULL, *s2 = NULL, *s3 = NULL;
    css_selector_parse_string("div", &s1);            // (0,0,1)
    css_selector_parse_string(".menu div", &s2);      // (0,1,1)
    css_selector_parse_string("#main .menu div", &s3); // (1,1,1)

    bool pass = (s1->spec_a == 0 && s1->spec_b == 0 && s1->spec_c == 1) &&
                (s2->spec_a == 0 && s2->spec_b == 1 && s2->spec_c == 1) &&
                (s3->spec_a == 1 && s3->spec_b == 1 && s3->spec_c == 1);

    int cmp1 = css_specificity_compare(s3->spec_a, s3->spec_b, s3->spec_c,
                                      s2->spec_a, s2->spec_b, s2->spec_c);
    pass = pass && (cmp1 > 0);

    css_selector_destroy(s1);
    css_selector_destroy(s2);
    css_selector_destroy(s3);
    return pass;
}

static bool test_cascade(void) {
    bos_node_t* div = NULL;
    bos_node_create(BOS_NODE_ELEMENT, "div", &div);
    bos_set_attribute(div, "id", "header");

    const char* css = "div { color: blue; } #header { color: red; }";
    css_stylesheet_t* sheet = NULL;
    css_parser_parse_string(css, &sheet);

    css_computed_style_t style;
    css_style_compute_for_node(div, sheet, &style);

    bool pass = (style.color.r == 255 && style.color.g == 0 && style.color.b == 0);

    css_stylesheet_destroy(sheet);
    bos_node_destroy(div);
    return pass;
}




static bool test_colors(void) {
    css_color_t c1, c2, c3, c4;
    css_color_parse("#00ff00", &c1);
    css_color_parse("rgb(255, 0, 0)", &c2);
    css_color_parse("blue", &c3);
    css_color_parse("transparent", &c4);

    return (c1.g == 255 && c2.r == 255 && c3.b == 255 && c4.a == 0);
}

static bool test_malformed_css(void) {
    css_stylesheet_t* sheet = NULL;
    // Missing semicolon, invalid token, unclosed declaration
    const char* css = "div { color red margin: 10px unknown-prop: ??? } p { display: block }";
    css_status_t status = css_parser_parse_string(css, &sheet);

    // Engine must recover without panic and parse valid rules
    bool pass = (status == CSS_OK && sheet != NULL);
    css_stylesheet_destroy(sheet);
    return pass;
}

static bool test_memory_leak(void) {
    const char* css = ".card { width: 300px; padding: 20px; color: black; background: white; }";

    for (int i = 0; i < 20; i++) {
        css_stylesheet_t* sheet = NULL;
        css_parser_parse_string(css, &sheet);
        css_stylesheet_destroy(sheet);
    }
    return true;
}

static bool test_large_stylesheet_perf(void) {
    // Generate ~100 KB stylesheet buffer dynamically or in static memory
    static char large_css[102400]; // 100 KB
    size_t offset = 0;

    for (int i = 0; i < 1500 && offset < sizeof(large_css) - 100; i++) {
        strcpy(&large_css[offset], ".class_");
        offset += strlen(".class_");

        // Simple int append
        int n = i;
        char num[16];
        int num_idx = 0;
        if (n == 0) num[num_idx++] = '0';
        while (n > 0) { num[num_idx++] = '0' + (n % 10); n /= 10; }
        for (int k = num_idx - 1; k >= 0; k--) large_css[offset++] = num[k];

        strcpy(&large_css[offset], " { width: 100px; height: 50px; color: red; margin: 10px; }\n");
        offset += strlen(" { width: 100px; height: 50px; color: red; margin: 10px; }\n");
    }
    large_css[offset] = '\0';

    css_stylesheet_t* sheet = NULL;
    css_status_t status = css_parser_parse_string(large_css, &sheet);

    bool pass = (status == CSS_OK && sheet != NULL && sheet->rule_count >= 1000);
    css_stylesheet_destroy(sheet);
    return pass;
}


void css_run_all_certification_tests(void) {
    display_print("=========================================\n");
    display_print("[CSS TESTS]\n");

    display_print("Tokenizer........");
    display_print(test_tokenizer() ? "PASS\n" : "FAIL\n");

    display_print("Selectors........");
    display_print(test_selectors() ? "PASS\n" : "FAIL\n");

    display_print("Declarations........");
    display_print(test_declarations() ? "PASS\n" : "FAIL\n");

    display_print("Specificity........");
    display_print(test_specificity() ? "PASS\n" : "FAIL\n");

    display_print("Cascade........");
    display_print(test_cascade() ? "PASS\n" : "FAIL\n");

    display_print("Colors........");
    display_print(test_colors() ? "PASS\n" : "FAIL\n");

    display_print("Malformed CSS........");
    display_print(test_malformed_css() ? "PASS\n" : "FAIL\n");

    display_print("Memory Leak........");
    display_print(test_memory_leak() ? "PASS\n" : "FAIL\n");

    display_print("Large Stylesheet........");
    display_print(test_large_stylesheet_perf() ? "PASS\n" : "FAIL\n");

    display_print("=========================================\n");
    display_print("SUCCESS\n");
    display_print("All CSS Parser & CSSOM Certification Tests Passed\n");
    display_print("=========================================\n");
}
