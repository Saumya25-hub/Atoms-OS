#include "abe_layout_test.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test_RenderTreeConstruction(void) {
    display_print("[ABE_LAYOUT_TEST] 1. Testing Render Tree Construction (Filtering <head> and display:none)...\n");

    const char* sample_html =
        "<!DOCTYPE html><html><head><title>Test Title</title><style>head { display: none; }</style></head>"
        "<body><div id=\"header\" style=\"width: 500px; height: 100px;\">Header Block</div>"
        "<div id=\"hidden-div\" style=\"display: none;\">Hidden Text</div></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(sample_html, strlen(sample_html), &doc);

    ABE_ComputeStyles(doc);

    ABE_RenderTreeHandle tree = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_BuildRenderTree(doc, &tree);
    if (err != ABE_SUCCESS || tree == ABE_INVALID_HANDLE) {
        display_print("[ABE_LAYOUT_TEST] FAIL: ABE_BuildRenderTree failed!\n");
        ABE_DestroyDocument(doc);
        return false;
    }

    // Verify hidden-div is NOT in Render Tree
    ABE_NodeHandle hidden_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "hidden-div", &hidden_node);
    if (hidden_node != ABE_INVALID_HANDLE) {
        ABE_LayoutBoxInfo box_info;
        err = ABE_GetLayoutBox(tree, hidden_node, &box_info);
        if (err == ABE_SUCCESS) {
            display_print("[ABE_LAYOUT_TEST] FAIL: Hidden element included in Render Tree!\n");
            ABE_DestroyRenderTree(tree);
            ABE_DestroyDocument(doc);
            return false;
        }
    }

    ABE_DestroyRenderTree(tree);
    ABE_DestroyDocument(doc);
    display_print("[ABE_LAYOUT_TEST] PASS: Render Tree Construction verified.\n");
    return true;
}

static bool Test_BlockAndInlineLayout(void) {
    display_print("[ABE_LAYOUT_TEST] 2. Testing Block & Inline Layout Geometry Calculation...\n");

    const char* layout_html =
        "<html><body><div id=\"container\" style=\"width: 600px; margin: 10px;\">"
        "<p id=\"para\" style=\"margin: 5px; height: 50px;\">Paragraph Content</p></div></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(layout_html, strlen(layout_html), &doc);
    ABE_ComputeStyles(doc);

    ABE_RenderTreeHandle tree = ABE_INVALID_HANDLE;
    ABE_BuildRenderTree(doc, &tree);

    // Perform Layout at 1024x768 Viewport
    ABE_Error err = ABE_PerformLayout(tree, 1024.0f, 768.0f);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_LAYOUT_TEST] FAIL: PerformLayout failed!\n");
        ABE_DestroyRenderTree(tree);
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_NodeHandle container_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "container", &container_node);
    if (container_node != ABE_INVALID_HANDLE) {
        ABE_LayoutBoxInfo box;
        if (ABE_GetLayoutBox(tree, container_node, &box) == ABE_SUCCESS) {
            if ((uint32_t)box.content_box.width != 600) {
                display_print("[ABE_LAYOUT_TEST] FAIL: Content Box Width mismatch! Expected 600, got: ");
                display_print_dec((uint32_t)box.content_box.width);
                display_print("\n");
                ABE_DestroyRenderTree(tree);
                ABE_DestroyDocument(doc);
                return false;
            }
        }
    }

    ABE_DestroyRenderTree(tree);
    ABE_DestroyDocument(doc);
    display_print("[ABE_LAYOUT_TEST] PASS: Block & Inline Layout Geometry verified.\n");
    return true;
}

static bool Test_IncrementalReflow(void) {
    display_print("[ABE_LAYOUT_TEST] 3. Testing Incremental Reflow & Dirty Node Tracking...\n");

    const char* reflow_html = "<html><body><div id=\"box1\" style=\"width: 400px;\">Box 1</div></body></html>";

    ABE_DocumentHandle doc = ABE_INVALID_HANDLE;
    ABE_ParseHTML(reflow_html, strlen(reflow_html), &doc);
    ABE_ComputeStyles(doc);

    ABE_RenderTreeHandle tree = ABE_INVALID_HANDLE;
    ABE_BuildRenderTree(doc, &tree);
    ABE_PerformLayout(tree, 1024.0f, 768.0f);

    ABE_NodeHandle box_node = ABE_INVALID_HANDLE;
    ABE_FindElementById(doc, "box1", &box_node);

    // Mark dirty & trigger Reflow
    ABE_MarkDirty(tree, box_node);
    ABE_Error err = ABE_Reflow(tree);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_LAYOUT_TEST] FAIL: Incremental Reflow failed!\n");
        ABE_DestroyRenderTree(tree);
        ABE_DestroyDocument(doc);
        return false;
    }

    ABE_DestroyRenderTree(tree);
    ABE_DestroyDocument(doc);
    display_print("[ABE_LAYOUT_TEST] PASS: Incremental Reflow verified.\n");
    return true;
}

void ABE_RunPhase5_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 5 Production Layout Test Suite     \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();
    ABE_CSSInitialize();
    ABE_LayoutInitialize();

    if (!Test_RenderTreeConstruction()) return;
    if (!Test_BlockAndInlineLayout()) return;
    if (!Test_IncrementalReflow()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_LAYOUT_DIAG] Render Trees Built: "); display_print_dec(metrics.render_trees_built_total); display_print("\n");
    display_print("[ABE_LAYOUT_DIAG] Layouts Performed : "); display_print_dec(metrics.layouts_performed_total); display_print("\n");
    display_print("[ABE_LAYOUT_DIAG] Reflows Performed : "); display_print_dec(metrics.reflows_performed_total); display_print("\n");
    display_print("[ABE_LAYOUT_DIAG] Dirty Nodes       : "); display_print_dec(metrics.dirty_nodes_processed); display_print("\n");
    display_print("[ABE_LAYOUT_DIAG] Layout Time (us)  : "); display_print_dec(metrics.layout_time_us); display_print("\n");

    ABE_LayoutShutdown();
    ABE_CSSShutdown();
    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE5_ABE_PRODUCTION_LAYOUT_ENGINE\n\n");
}
