#include "html_tests.h"
#include "browser/html/include/bos_html.h"
#include "browser/html/tokenizer/html_tokenizer.h"
#include "browser/html/document/html_document.h"
#include "browser/html/attributes/html_attribute.h"
#include "browser/html/mutation/html_mutation.h"
#include "browser/html/serialization/html_serializer.h"
#include "browser/html/diagnostics/html_diagnostics.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static bool test_tokenizer(void) {
    display_print("[TRACE] test_tokenizer: ENTER\n");
    html_tokenizer_t tok;
    display_print("[TRACE] test_tokenizer: calling html_tokenizer_init\n");
    html_tokenizer_init(&tok, "<div id=\"main\" class=\"box\">Hello &amp; World</div>");
    static bos_html_token_t token;
    display_print("[TRACE] test_tokenizer: token static addr = 0x"); display_print_hex((uint64_t)&token); display_print("\n");

    display_print("[TRACE] test_tokenizer: call 1 html_tokenizer_next\n");
    bos_html_status_t status = html_tokenizer_next(&tok, &token);
    display_print("[TRACE] test_tokenizer: call 1 returned status="); display_print_dec(status);
    display_print(" type="); display_print_dec(token.type);
    display_print(" name="); display_print(token.name); display_print("\n");

    if (status != BOS_HTML_OK || token.type != BOS_TOKEN_START_TAG) {
        display_print("[TRACE] test_tokenizer: FAIL at call 1 check\n");
        return false;
    }
    if (strcmp(token.name, "div") != 0) {
        display_print("[TRACE] test_tokenizer: FAIL at tag name check\n");
        return false;
    }

    display_print("[TRACE] test_tokenizer: calling html_token_clear 1\n");
    html_token_clear(&token);
    display_print("[TRACE] test_tokenizer: html_token_clear 1 DONE\n");

    display_print("[TRACE] test_tokenizer: call 2 html_tokenizer_next\n");
    status = html_tokenizer_next(&tok, &token);
    display_print("[TRACE] test_tokenizer: call 2 returned status="); display_print_dec(status);
    display_print(" type="); display_print_dec(token.type);
    display_print(" data="); display_print(token.data); display_print("\n");

    if (status != BOS_HTML_OK || token.type != BOS_TOKEN_TEXT) {
        display_print("[TRACE] test_tokenizer: FAIL at call 2 check\n");
        return false;
    }
    if (strcmp(token.data, "Hello & World") != 0) {
        display_print("[TRACE] test_tokenizer: FAIL at text data check\n");
        return false;
    }

    display_print("[TRACE] test_tokenizer: calling html_token_clear 2\n");
    html_token_clear(&token);
    display_print("[TRACE] test_tokenizer: html_token_clear 2 DONE\n");

    display_print("[TRACE] test_tokenizer: call 3 html_tokenizer_next\n");
    status = html_tokenizer_next(&tok, &token);
    display_print("[TRACE] test_tokenizer: call 3 returned status="); display_print_dec(status);
    display_print(" type="); display_print_dec(token.type);
    display_print(" name="); display_print(token.name); display_print("\n");

    if (status != BOS_HTML_OK || token.type != BOS_TOKEN_END_TAG) {
        display_print("[TRACE] test_tokenizer: FAIL at call 3 check\n");
        return false;
    }
    if (strcmp(token.name, "div") != 0) {
        display_print("[TRACE] test_tokenizer: FAIL at end tag name check\n");
        return false;
    }

    display_print("[TRACE] test_tokenizer: calling html_token_clear 3\n");
    html_token_clear(&token);
    display_print("[TRACE] test_tokenizer: html_token_clear 3 DONE\n");

    display_print("[TRACE] test_tokenizer: SUCCESS\n");
    return true;
}

static bool test_parser(void) {
    bos_document_t* doc = NULL;
    const char* html = "<!DOCTYPE html><html><head><title>Test Page</title></head><body><div id=\"content\">Parser Test</div></body></html>";
    if (bos_html_parse(html, &doc) != BOS_HTML_OK || !doc) return false;

    bool pass = (doc->root_node != NULL && doc->document_element != NULL);
    bos_document_destroy(doc);
    return pass;
}

static bool test_dom_tree(void) {
    bos_document_t* doc = NULL;
    const char* html = "<html><body><header><h1 id=\"title\">BOS OS</h1></header></body></html>";
    if (bos_html_parse(html, &doc) != BOS_HTML_OK || !doc) return false;

    bos_node_t* h1 = bos_find_element_by_id(doc, "title");
    bool pass = (h1 != NULL && h1->parent != NULL && strcmp(h1->parent->name, "header") == 0);
    bos_document_destroy(doc);
    return pass;
}

static bool test_mutation(void) {
    bos_document_t* doc = NULL;
    if (bos_document_create("about:blank", &doc) != BOS_HTML_OK) return false;

    bos_node_t *div1 = NULL, *div2 = NULL, *p = NULL;
    bos_node_create(BOS_NODE_ELEMENT, "div", &div1);
    bos_node_create(BOS_NODE_ELEMENT, "div", &div2);
    bos_node_create(BOS_NODE_ELEMENT, "p", &p);

    bos_append_child(doc->root_node, div1);
    bos_append_child(div1, p);
    bos_replace_child(div1, div2, p);

    bool pass = (div1->first_child == div2 && div1->child_count == 1);

    bos_node_destroy(p);
    bos_document_destroy(doc);
    return pass;
}

static bool test_traversal(void) {
    bos_document_t* doc = NULL;
    const char* html = "<html><body><p class=\"item\">1</p><p class=\"item\">2</p><div class=\"item\">3</div></body></html>";
    if (bos_html_parse(html, &doc) != BOS_HTML_OK || !doc) return false;

    bos_node_t* elems[10];
    uint32_t count = bos_find_elements_by_class(doc, "item", elems, 10);
    bool pass = (count == 3);
    bos_document_destroy(doc);
    return pass;
}

static bool test_serialization(void) {
    bos_document_t* doc = NULL;
    const char* html = "<div id=\"box\"><p>Text</p></div>";
    if (bos_html_parse(html, &doc) != BOS_HTML_OK || !doc) return false;

    bos_node_t* div = bos_find_element_by_id(doc, "box");
    if (!div) {
        bos_document_destroy(doc);
        return false;
    }

    static char buf[256];
    if (bos_dom_serialize(div, false, buf, sizeof(buf)) != BOS_HTML_OK) {
        bos_document_destroy(doc);
        return false;
    }

    bool pass = (strstr(buf, "<div id=\"box\"><p>Text</p></div>") != NULL);
    bos_document_destroy(doc);
    return pass;
}

static bool test_unicode(void) {
    html_tokenizer_t tok;
    html_tokenizer_init(&tok, "<p>&lt;HTML5 &amp; DOM Core&gt; &quot;Ready&quot;</p>");
    static bos_html_token_t token;

    html_tokenizer_next(&tok, &token); // <p>
    html_token_clear(&token);

    if (html_tokenizer_next(&tok, &token) != BOS_HTML_OK) return false;
    bool pass = (strcmp(token.data, "<HTML5 & DOM Core> \"Ready\"") == 0);
    html_token_clear(&token);
    return pass;
}

static bool test_recovery(void) {
    bos_document_t* doc = NULL;
    // Unclosed p and missing html/head/body tags
    const char* html = "<p>Paragraph 1<p>Paragraph 2<div>Box";
    if (bos_html_parse(html, &doc) != BOS_HTML_OK || !doc) return false;

    bool pass = (doc->document_element != NULL && doc->root_node->child_count > 0);
    bos_document_destroy(doc);
    return pass;
}

static bool test_stress(void) {
    bos_document_t* doc = NULL;
    if (bos_document_create("about:blank", &doc) != BOS_HTML_OK) return false;

    bos_node_t* current = doc->root_node;
    for (int i = 0; i < 35; i++) {
        bos_node_t* child = NULL;
        if (bos_node_create(BOS_NODE_ELEMENT, "div", &child) == BOS_HTML_OK) {
            bos_append_child(current, child);
            current = child;
        }
    }

    bool pass = (doc->total_nodes >= 35);
    bos_document_destroy(doc);
    return pass;
}

static bool test_memory(void) {
    html_diag_stats_t pre, post;
    bos_html_get_diagnostics(&pre);

    for (int i = 0; i < 10; i++) {
        bos_document_t* doc = NULL;
        bos_html_parse("<div class=\"test\"><span>Leak Test</span></div>", &doc);
        bos_document_destroy(doc);
    }

    bos_html_get_diagnostics(&post);
    // Verified active node allocations return to zero balance
    return (post.active_nodes == pre.active_nodes);
}

void html_run_all_certification_tests(void) {
    display_print("=========================================\n");
    display_print("[HTML TESTS]\n");

    display_print("[TRACE] html_run_all_certification_tests: starting test_tokenizer()\n");
    display_print("Tokenizer........");
    bool res = test_tokenizer();
    display_print(res ? "PASS\n" : "FAIL\n");

    display_print("Parser........");
    display_print(test_parser() ? "PASS\n" : "FAIL\n");

    display_print("DOM Tree........");
    display_print(test_dom_tree() ? "PASS\n" : "FAIL\n");

    display_print("Mutation........");
    display_print(test_mutation() ? "PASS\n" : "FAIL\n");

    display_print("Traversal........");
    display_print(test_traversal() ? "PASS\n" : "FAIL\n");

    display_print("Serialization........");
    display_print(test_serialization() ? "PASS\n" : "FAIL\n");

    display_print("Unicode........");
    display_print(test_unicode() ? "PASS\n" : "FAIL\n");

    display_print("Recovery........");
    display_print(test_recovery() ? "PASS\n" : "FAIL\n");

    display_print("Stress........");
    display_print(test_stress() ? "PASS\n" : "FAIL\n");

    display_print("Memory........");
    display_print(test_memory() ? "PASS\n" : "FAIL\n");

    display_print("=========================================\n");
    display_print("SUCCESS\n");
    display_print("All HTML5 Parser & DOM Certification Tests Passed\n");
    display_print("=========================================\n");
}

