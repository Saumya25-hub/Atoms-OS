#include "abe_html_test.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test_ValidHTML5Parsing(void) {
    display_print("[ABE_HTML_TEST] 1. Testing Valid HTML5 Document Parsing...\n");

    const char* sample_html = "<!DOCTYPE html><html><head><title>ATOMS OS</title></head><body><h1 id=\"main-title\">Welcome to ABE V1</h1><p class=\"intro\">Production HTML5 Engine</p></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(sample_html, strlen(sample_html), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) {
        display_print("[ABE_HTML_TEST] FAIL: ABE_ParseHTML failed!\n");
        return false;
    }

    ABE_NodeHandle title_node = ABE_INVALID_HANDLE;
    err = ABE_FindElementById(doc, "main-title", &title_node);
    if (err != ABE_SUCCESS || title_node == ABE_INVALID_HANDLE) {
        display_print("[ABE_HTML_TEST] FAIL: FindElementById ('main-title') failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DOMNodeInfo info;
    ABE_GetNodeInfo(title_node, &info);
    if (strcmp(info.tag_name, "h1") != 0) {
        display_print("[ABE_HTML_TEST] FAIL: Tag name mismatch for main-title!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_HTML_TEST] PASS: Valid HTML5 Document Parsing verified.\n");
    return true;
}

static bool Test_MalformedHTMLRecovery(void) {
    display_print("[ABE_HTML_TEST] 2. Testing Malformed HTML Error Recovery...\n");

    // Malformed HTML: unclosed <p> tag, missing </body></html>, unquoted attributes
    const char* malformed_html = "<div><p id=p1>Unclosed Paragraph 1<p id=p2>Unclosed Paragraph 2<span>Nested text";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(malformed_html, strlen(malformed_html), &doc);
    if (err != ABE_SUCCESS || doc == ABE_INVALID_HANDLE) {
        display_print("[ABE_HTML_TEST] FAIL: Malformed HTML parser crashed or failed!\n");
        return false;
    }

    ABE_NodeHandle p2_node = ABE_INVALID_HANDLE;
    err = ABE_FindElementById(doc, "p2", &p2_node);
    if (err != ABE_SUCCESS || p2_node == ABE_INVALID_HANDLE) {
        display_print("[ABE_HTML_TEST] FAIL: Auto-closed paragraph recovery failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_HTML_TEST] PASS: Malformed HTML Error Recovery verified.\n");
    return true;
}

static bool Test_EntityDecoding(void) {
    display_print("[ABE_HTML_TEST] 3. Testing Entity Reference Decoding (&amp;, &lt;, &gt;, &quot;)...\n");

    const char* entity_html = "<p id=\"ent\">ATOMS &amp; ABE &lt;Engine&gt; &quot;V1.0&quot;</p>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_ParseHTML(entity_html, strlen(entity_html), &doc);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_HTML_TEST] FAIL: Entity HTML parsing failed!\n");
        return false;
    }

    ABE_NodeHandle p_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "ent", &p_node);
    if (p_node == ABE_INVALID_HANDLE) {
        display_print("[ABE_HTML_TEST] FAIL: Entity element lookup failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DOMNodeInfo info;
    ABE_GetNodeInfo(p_node, &info);
    if (info.first_child != ABE_INVALID_HANDLE) {
        ABE_DOMNodeInfo txt_info;
        ABE_GetNodeInfo(info.first_child, &txt_info);
        if (strstr(txt_info.node_value, "ATOMS & ABE <Engine> \"V1.0\"") == NULL) {
            display_print("[ABE_HTML_TEST] FAIL: Entity decoding mismatch: ");
            display_print(txt_info.node_value);
            display_print("\n");
            ABE_DestroyDocument(doc);
            return false;
        }
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_HTML_TEST] PASS: Entity Reference Decoding verified.\n");
    return true;
}

static bool Test_DOMTagSearch(void) {
    display_print("[ABE_HTML_TEST] 4. Testing DOM FindElementsByTag...\n");

    const char* list_html = "<ul><li>Item 1</li><li>Item 2</li><li>Item 3</li></ul>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(list_html, strlen(list_html), &doc);

    ABE_NodeHandle li_nodes[16];
    uint32_t count = 0;
    ABE_Error err = ABE_FindElementsByTag(doc, "li", li_nodes, 16, &count);
    if (err != ABE_SUCCESS || count != 3) {
        display_print("[ABE_HTML_TEST] FAIL: FindElementsByTag ('li') failed, count=");
        display_print_dec(count);
        display_print("\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyDocument(doc);
    display_print("[ABE_HTML_TEST] PASS: DOM FindElementsByTag verified.\n");
    return true;
}

void ABE_RunPhase3_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 3 Production HTML5 Test Suite     \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();

    if (!Test_ValidHTML5Parsing()) return;
    if (!Test_MalformedHTMLRecovery()) return;
    if (!Test_EntityDecoding()) return;
    if (!Test_DOMTagSearch()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_HTML_DIAG] Documents Parsed   : "); display_print_dec(metrics.documents_parsed_total); display_print("\n");
    display_print("[ABE_HTML_DIAG] DOM Nodes Active   : "); display_print_dec(metrics.dom_nodes_active); display_print("\n");
    display_print("[ABE_HTML_DIAG] Tokens Generated   : "); display_print_dec(metrics.html_tokens_generated); display_print("\n");
    display_print("[ABE_HTML_DIAG] Errors Recovered   : "); display_print_dec(metrics.html_parse_errors_recovered); display_print("\n");
    display_print("[ABE_HTML_DIAG] DOM Max Depth      : "); display_print_dec(metrics.dom_tree_max_depth); display_print("\n");

    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE3_ABE_PRODUCTION_HTML5_ENGINE\n\n");
}
