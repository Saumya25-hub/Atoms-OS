#ifndef ATRIX_HTML_DOCUMENT_H
#define ATRIX_HTML_DOCUMENT_H

#include "html_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX HTML Document Loader & Node Tree Subsystem
// ============================================================

typedef struct {
    char     title[128];
    DOMNode* root_node;
    DOMNode* head_node;
    DOMNode* body_node;
    uint32_t total_links_count;
    uint32_t total_images_count;
    uint32_t total_inputs_count;
} HTMLDocument;

void          ATRIX_HTMLDocument_Init(void);
HTMLDocument* ATRIX_HTMLDocument_CreateFromStream(const char* html_stream);
void          ATRIX_HTMLDocument_Free(HTMLDocument* doc);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_HTML_DOCUMENT_H
