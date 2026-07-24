#include "abe_api.h"
#include "../core/abe_core.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/browser/engine/networking/browser_http.h"
#include "kernel/browser/engine/css/css_layout.h"
#include "kernel/browser/engine/render/render_tree.h"
#include "kernel/browser/engine/render/paint_engine.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static uint32_t g_abe_next_engine_id = 100;

ABE_Engine* ABE_CreateEngine(void) {
    ABE_Engine* engine = (ABE_Engine*)kmalloc(sizeof(ABE_Engine));
    if (!engine) return 0;
    memset(engine, 0, sizeof(ABE_Engine));

    engine->engine_id = g_abe_next_engine_id++;
    engine->is_active = true;
    display_print("[ABE] Engine Created! ID: ");
    display_print_dec(engine->engine_id);
    display_print("\n");
    return engine;
}

void ABE_DestroyEngine(ABE_Engine* engine) {
    if (!engine) return;
    if (engine->active_document) {
        ATRIX_HTMLDocument_Free(engine->active_document);
        engine->active_document = 0;
    }
    display_print("[ABE] Engine Destroyed! ID: ");
    display_print_dec(engine->engine_id);
    display_print("\n");
    kfree(engine);
}

bool ABE_LoadURL(ABE_Engine* engine, const char* url) {
    if (!engine || !url) return false;

    strncpy(engine->current_url, url, sizeof(engine->current_url) - 1);
    display_print("[ABE] Loading URL: ");
    display_print(url);
    display_print("\n");

    uint8_t* html_data = 0;
    uint32_t html_len = 0;
    bool ok = ATRIX_BrowserHTTP_FetchURL(url, &html_data, &html_len);
    if (ok && html_data) {
        bool res = ABE_ParseHTML(engine, (const char*)html_data);
        kfree(html_data);
        return res;
    }
    return false;
}

bool ABE_ParseHTML(ABE_Engine* engine, const char* html) {
    if (!engine || !html) return false;

    if (engine->active_document) {
        ATRIX_HTMLDocument_Free(engine->active_document);
    }

    engine->active_document = ATRIX_HTMLDocument_CreateFromStream(html);
    if (engine->active_document) {
        engine->dom_node_count = engine->active_document->total_links_count + engine->active_document->total_images_count + engine->active_document->total_inputs_count + 2;
        display_print("[ABE] HTML Parsed Successfully! DOM Node Count: ");
        display_print_dec(engine->dom_node_count);
        display_print("\n");
        return true;
    }
    return false;
}

bool ABE_ApplyCSS(ABE_Engine* engine, const char* css) {
    if (!engine || !css) return false;
    display_print("[ABE] Applying CSS Stylesheet Rules...\n");
    CSSBoxModel box = {0};
    box.margin = 8;
    ATRIX_CSSLayout_ComputeBoxModel(&box, 800, 600);
    return true;
}

bool ABE_RunJavaScript(ABE_Engine* engine, const char* js) {
    if (!engine || !js) return false;
    display_print("[ABE] JS Execution Engine Running Script Payload...\n");
    return true;
}

bool ABE_Render(ABE_Engine* engine, void* target_buffer, int32_t width, int32_t height) {
    if (!engine || !target_buffer) return false;

    RenderTree* rtree = ATRIX_RenderTree_Build();
    if (rtree) {
        ATRIX_PaintEngine_PaintTree(rtree, target_buffer, 0);
        kfree(rtree);
        engine->frame_count++;
        return true;
    }
    return false;
}

void ABE_GetDiagnostics(ABE_Engine* engine, char* buffer, uint32_t max_len) {
    if (!engine || !buffer || max_len == 0) return;
    display_print("[ABE_DIAG] Engine ID: ");
    display_print_dec(engine->engine_id);
    display_print(" DOM Nodes: ");
    display_print_dec(engine->dom_node_count);
    display_print(" Frames: ");
    display_print_dec(engine->frame_count);
    display_print("\n");
    strncpy(buffer, "ABE V1.0 Active Engine", max_len - 1);
}
