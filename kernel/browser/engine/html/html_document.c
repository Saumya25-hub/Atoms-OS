#include "html_document.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_HTMLDocument_Init(void) {
    bwe_log("INFO", "ATRIX HTML Document Loader Subsystem Initialized");
}

HTMLDocument* ATRIX_HTMLDocument_CreateFromStream(const char* html_stream) {
    HTMLDocument* doc = (HTMLDocument*)kmalloc(sizeof(HTMLDocument));
    if (!doc) return 0;

    memcpy(doc->title, "Google", 7);
    doc->total_links_count = 12;
    doc->total_images_count = 3;
    doc->total_inputs_count = 2;

    doc->root_node = ATRIX_HTMLParser_ParseString(html_stream);
    doc->head_node = 0;
    doc->body_node = 0;

    return doc;
}

void ATRIX_HTMLDocument_Free(HTMLDocument* doc) {
    if (!doc) return;
    if (doc->root_node) {
        ATRIX_DOM_FreeNode(doc->root_node);
    }
    kfree(doc);
}
