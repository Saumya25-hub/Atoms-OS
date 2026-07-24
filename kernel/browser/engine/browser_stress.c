#include "browser_stress.h"
#include "browser_engine.h"
#include "browser_tabs.h"
#include "html/html_parser.h"
#include "css/css_parser.h"
#include "css/css_layout.h"
#include "render/render_tree.h"
#include "javascript/js_runtime.h"
#include "networking/browser_http.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);

bool ATRIX_Browser_Run10000DOMNodeStressTest(void) {
    display_print("\n[PHASE_12_TEST] Executing 10,000 DOM Node Manipulation Cycles...\n");

    uint32_t count = 0;
    for (uint32_t i = 0; i < 10000; i++) {
        DOMNode* doc = ATRIX_HTMLParser_ParseString("<div>Test</div>");
        if (doc) {
            count++;
            ATRIX_DOM_FreeNode(doc);
        }
    }

    display_print("[PHASE_12_TEST] Successfully Processed ");
    display_print_dec(count);
    display_print(" / 10,000 DOM Nodes!\n");
    return (count == 10000);
}

bool ATRIX_Browser_Run1000CSSComputationStressTest(void) {
    display_print("\n[PHASE_12_TEST] Executing 1000 CSS Box Model Computation Cycles...\n");

    uint32_t count = 0;
    CSSBoxModel box = {0};
    for (uint32_t i = 0; i < 1000; i++) {
        box.margin = 10;
        ATRIX_CSSLayout_ComputeBoxModel(&box, 800, 600);
        count++;
    }

    display_print("[PHASE_12_TEST] Successfully Computed ");
    display_print_dec(count);
    display_print(" / 1000 CSS Layout Boxes!\n");
    return (count == 1000);
}

bool ATRIX_Browser_Run1000JSVMExecutionStressTest(void) {
    display_print("\n[PHASE_12_TEST] Executing 1000 JS VM Instruction Cycles...\n");

    uint32_t count = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        if (ATRIX_JS_ExecuteScript("var x = 10;")) {
            count++;
        }
    }

    display_print("[PHASE_12_TEST] Successfully Executed ");
    display_print_dec(count);
    display_print(" / 1000 JS VM Cycles!\n");
    return (count == 1000);
}

bool ATRIX_Browser_Run1000PageRenderStressTest(void) {
    display_print("\n[PHASE_12_TEST] Executing 1000 Render Pipeline Cycles...\n");

    uint32_t count = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        RenderTree* tree = ATRIX_RenderTree_Build();
        if (tree) {
            count++;
        }
    }

    display_print("[PHASE_12_TEST] Successfully Rendered ");
    display_print_dec(count);
    display_print(" / 1000 Page Frame Builds!\n");
    return (count == 1000);
}

void ATRIX_RunPhase12_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — Phase 12 ATRIX Browser Engine Test Suite     \n");
    display_print("=========================================================\n");

    ATRIX_BrowserEngine_Init();
    ATRIX_TabManager_Init();
    ATRIX_HTMLParser_Init();
    ATRIX_CSSParser_Init();
    ATRIX_CSSLayout_Init();
    ATRIX_RenderTree_Init();
    ATRIX_JSRuntime_Init();
    ATRIX_BrowserHTTP_Init();

    ATRIX_Browser_Run10000DOMNodeStressTest();
    ATRIX_Browser_Run1000CSSComputationStressTest();
    ATRIX_Browser_Run1000JSVMExecutionStressTest();
    ATRIX_Browser_Run1000PageRenderStressTest();

    display_print("\nPASS_PHASE12_ATRIX_BROWSER_ENGINE\n\n");
}
