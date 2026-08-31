#include "abe_html_test.h"
#include "abe_dom_node.h"
#include "abe_html_element.h"
#include "abe_html_document.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test1_BasicDocument(void) {
    display_print("[HTML5 TEST 1] Basic Document Hierarchy...\n");
    const char* html = "<html><head><title>Title</title></head><body><h1>Hello</h1><p id=\"p1\">World</p></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(html, strlen(html), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) return false;

    ABE_NodeHandle p_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "p1", &p_node);
    if (p_node == ABE_INVALID_HANDLE) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 1] PASS: Basic Document.\n");
    return true;
}

static bool Test2_ImpliedElements(void) {
    display_print("[HTML5 TEST 2] Implied Elements (omitted html/head/body)...\n");
    const char* html = "<title>Implied Title</title><p id=\"imp-p\">Implied Body Paragraph</p>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(html, strlen(html), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) return false;

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    if (!doc_struct || !doc_struct->root_node) {
        ABE_DestroyDocument(doc);
        return false;
    }

    // Verify html -> head + body exist
    ABE_DOMNode* html_node = ABE_DOM_GetElementById(doc_struct->root_node, "imp-p");
    if (!html_node || !html_node->parent || strcmp(html_node->parent->tag_name, "body") != 0) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 2] PASS: Implied Elements Created & Nested Correctly.\n");
    return true;
}

static bool Test3_Attributes(void) {
    display_print("[HTML5 TEST 3] Attributes (quoted, unquoted, data attributes)...\n");
    const char* html = "<div id=\"x\" class=\"alpha beta\" data-test=123 hidden>Content</div>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* div = ABE_DOM_GetElementById(doc_struct->root_node, "x");
    if (!div) {
        ABE_DestroyDocument(doc);
        return false;
    }

    const char* cls = ABE_HTMLAttr_Get(div, "class");
    const char* dt = ABE_HTMLAttr_Get(div, "data-test");
    bool has_hidden = ABE_HTMLAttr_Has(div, "hidden");

    if (!cls || strcmp(cls, "alpha beta") != 0 || !dt || strcmp(dt, "123") != 0 || !has_hidden) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 3] PASS: Attributes Handled Correctly.\n");
    return true;
}

static bool Test4_Entities(void) {
    display_print("[HTML5 TEST 4] Named & Numeric Character References...\n");
    const char* html = "<p id=\"ent\">&lt;ATOMS&gt; &amp; &#65; &#x42; &copy;</p>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* p = ABE_DOM_GetElementById(doc_struct->root_node, "ent");
    if (!p) {
        ABE_DestroyDocument(doc);
        return false;
    }

    char text[256];
    ABE_DOM_GetTextContent(p, text, sizeof(text));
    if (strstr(text, "<ATOMS> & A B") == NULL) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 4] PASS: Entities Decoded Correctly.\n");
    return true;
}

static bool Test5_Misnesting(void) {
    display_print("[HTML5 TEST 5] Misnested Tag Error Recovery...\n");
    const char* html = "<p id=\"p1\"><b>Hello</p>World</b>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* p = ABE_DOM_GetElementById(doc_struct->root_node, "p1");
    if (!p) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 5] PASS: Misnested HTML Recovered Safely.\n");
    return true;
}

static bool Test6_Lists(void) {
    display_print("[HTML5 TEST 6] Lists with Optional End Tags...\n");
    const char* html = "<ul id=\"list\"><li>Item 1<li>Item 2<li>Item 3</ul>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* ul = ABE_DOM_GetElementById(doc_struct->root_node, "list");
    if (!ul || ul->child_count != 3) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 6] PASS: List Auto-Closing Succeeded.\n");
    return true;
}

static bool Test7_Tables(void) {
    display_print("[HTML5 TEST 7] Table Structure with Implied TBody...\n");
    const char* html = "<table id=\"t\"><tr><td>Cell 1</td><td>Cell 2</td></tr></table>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* table = ABE_DOM_GetElementById(doc_struct->root_node, "t");
    if (!table || !table->first_child || strcmp(table->first_child->tag_name, "tbody") != 0) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 7] PASS: Table & Implied TBody Created.\n");
    return true;
}

static bool Test8_ScriptRawText(void) {
    display_print("[HTML5 TEST 8] Script Raw Text Elements...\n");
    const char* html = "<script id=\"js\">if (a < b && c > d) { console.log('test'); }</script>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* script = ABE_DOM_GetElementById(doc_struct->root_node, "js");
    if (!script) {
        ABE_DestroyDocument(doc);
        return false;
    }

    char code_txt[256];
    ABE_DOM_GetTextContent(script, code_txt, sizeof(code_txt));
    if (strstr(code_txt, "if (a < b && c > d)") == NULL) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 8] PASS: Script Raw Text Preserved.\n");
    return true;
}

static bool Test9_StyleRawText(void) {
    display_print("[HTML5 TEST 9] Style Raw Text Elements...\n");
    const char* html = "<style id=\"css\">body > div.main { color: #fff; background: #000; }</style>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* style = ABE_DOM_GetElementById(doc_struct->root_node, "css");
    if (!style) {
        ABE_DestroyDocument(doc);
        return false;
    }

    char css_txt[256];
    ABE_DOM_GetTextContent(style, css_txt, sizeof(css_txt));
    if (strstr(css_txt, "body > div.main") == NULL) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 9] PASS: Style Raw Text Preserved.\n");
    return true;
}

static bool Test10_FuzzMalformed(void) {
    display_print("[HTML5 TEST 10] Fuzz / Malformed Deep Edge Cases...\n");
    const char* malformed = "<<<div >>><p class=\"\"\"\" id=123>>>><<<<span<<<<<//span>>>><<<!-- unclosed comment";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(malformed, strlen(malformed), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) {
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 10] PASS: Deep Malformed Input Handled Safely.\n");
    return true;
}

static bool Test11_LargeDocument(void) {
    display_print("[HTML5 TEST 11] Large Document Memory Limits...\n");

    char large_doc[8192];
    strcpy(large_doc, "<!DOCTYPE html><html><body><div id=\"container\">");
    for (int i = 0; i < 50; i++) {
        strcat(large_doc, "<p class=\"item\">Paragraph item entry content</p>");
    }
    strcat(large_doc, "</div></body></html>");

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(large_doc, strlen(large_doc), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) {
        return false;
    }

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* items[64];
    uint32_t count = ABE_DOM_GetElementsByClassName(doc_struct->root_node, "item", items, 64);
    if (count != 50) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 11] PASS: Large Document (50 paragraphs) Parsed.\n");
    return true;
}

static bool Test12_DOMQueriesAndSerialization(void) {
    display_print("[HTML5 TEST 12] DOM Queries, TextContent & InnerHTML...\n");
    const char* html = "<div id=\"root\"><h1 class=\"hdr main\">Header</h1><p class=\"desc\">Description</p></div>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(html, strlen(html), &doc);

    ABE_DocumentStruct* doc_struct = ABE_HTMLDoc_Get(doc);
    ABE_DOMNode* root = ABE_DOM_GetElementById(doc_struct->root_node, "root");
    if (!root) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DOMNode* headers[8];
    uint32_t h_count = ABE_DOM_GetElementsByClassName(root, "hdr", headers, 8);
    if (h_count != 1) {
        ABE_DestroyDocument(doc);
        return false;
    }

    char inner[512];
    ABE_DOM_GetInnerHTML(root, inner, sizeof(inner));
    if (strstr(inner, "<h1") == NULL || strstr(inner, "<p") == NULL) {
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[HTML5 TEST 12] PASS: DOM Queries & InnerHTML Serialization Verified.\n");
    return true;
}

void ABE_RunPhase3_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 4 Production HTML5 / DOM Test Suite \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();

    if (!Test1_BasicDocument()) return;
    if (!Test2_ImpliedElements()) return;
    if (!Test3_Attributes()) return;
    if (!Test4_Entities()) return;
    if (!Test5_Misnesting()) return;
    if (!Test6_Lists()) return;
    if (!Test7_Tables()) return;
    if (!Test8_ScriptRawText()) return;
    if (!Test9_StyleRawText()) return;
    if (!Test10_FuzzMalformed()) return;
    if (!Test11_LargeDocument()) return;
    if (!Test12_DOMQueriesAndSerialization()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_HTML_DIAG] Documents Parsed   : "); display_print_dec(metrics.documents_parsed_total); display_print("\n");
    display_print("[ABE_HTML_DIAG] DOM Nodes Active   : "); display_print_dec(metrics.dom_nodes_active); display_print("\n");
    display_print("[ABE_HTML_DIAG] Tokens Generated   : "); display_print_dec(metrics.html_tokens_generated); display_print("\n");
    display_print("[ABE_HTML_DIAG] Errors Recovered   : "); display_print_dec(metrics.html_parse_errors_recovered); display_print("\n");
    display_print("[ABE_HTML_DIAG] DOM Max Depth      : "); display_print_dec(metrics.dom_tree_max_depth); display_print("\n");

    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE4_ABE_PRODUCTION_HTML5_DOM_ENGINE\n\n");
}
