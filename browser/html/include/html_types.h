#ifndef BOS_HTML_TYPES_H
#define BOS_HTML_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// BOS OS — Phase 5A: HTML5 Parser & DOM Core Engine Types
// ============================================================================

typedef enum {
    BOS_HTML_OK                     = 0,
    BOS_HTML_ERR_INVALID_PARAM      = -1,
    BOS_HTML_ERR_OUT_OF_MEMORY      = -2,
    BOS_HTML_ERR_PARSE_ERROR        = -3,
    BOS_HTML_ERR_MALFORMED_TOKEN    = -4,
    BOS_HTML_ERR_NODE_MISMATCH      = -5,
    BOS_HTML_ERR_NOT_FOUND          = -6,
    BOS_HTML_ERR_BUFFER_OVERFLOW    = -7
} bos_html_status_t;

typedef enum {
    BOS_NODE_ELEMENT                = 1,
    BOS_NODE_ATTRIBUTE              = 2,
    BOS_NODE_TEXT                   = 3,
    BOS_NODE_CDATA_SECTION          = 4,
    BOS_NODE_PROCESSING_INSTRUCTION = 7,
    BOS_NODE_COMMENT                = 8,
    BOS_NODE_DOCUMENT               = 9,
    BOS_NODE_DOCTYPE                = 10,
    BOS_NODE_DOCUMENT_FRAGMENT      = 11
} bos_node_type_t;

typedef enum {
    BOS_TOKEN_DOCTYPE               = 1,
    BOS_TOKEN_START_TAG             = 2,
    BOS_TOKEN_END_TAG               = 3,
    BOS_TOKEN_TEXT                  = 4,
    BOS_TOKEN_COMMENT               = 5,
    BOS_TOKEN_EOF                   = 6
} bos_token_type_t;

// Attribute Structure
typedef struct bos_attr {
    char name[64];
    char value[256];
    struct bos_attr* next;
} bos_attr_t;

struct bos_document;

// Core DOM Node Structure
typedef struct bos_node {
    uint32_t id;
    bos_node_type_t type;
    char name[64];              // Tag name (lowercase) or #text, #comment, #document
    char value[512];            // Text data or comment value or doctype raw string
    struct bos_document* owner_document;
    struct bos_node* parent;
    struct bos_node* first_child;
    struct bos_node* last_child;
    struct bos_node* previous_sibling;
    struct bos_node* next_sibling;
    bos_attr_t* attributes;
    uint32_t child_count;
    uint32_t ref_count;
    bool self_closing;
} bos_node_t;

// Document Structure
typedef struct bos_document {
    bos_node_t* root_node;      // Document Node
    bos_node_t* document_element; // <html> Element
    bos_node_t* head;           // <head> Element
    bos_node_t* body;           // <body> Element
    char title[128];
    char document_uri[256];
    char character_encoding[32];
    char ready_state[32];       // "loading", "interactive", "complete"
    uint32_t total_nodes;
} bos_document_t;

// Tokenizer Token Structure
typedef struct bos_html_token {
    bos_token_type_t type;
    char name[64];
    char data[512];
    bos_attr_t* attributes;
    bool self_closing;
} bos_html_token_t;

// Diagnostic Statistics Structure
typedef struct {
    uint32_t nodes_allocated;
    uint32_t nodes_freed;
    uint32_t active_nodes;
    uint32_t attributes_allocated;
    uint32_t parse_time_us;
    uint32_t tokens_processed;
    uint32_t errors_recovered;
} html_diag_stats_t;

#endif // BOS_HTML_TYPES_H
