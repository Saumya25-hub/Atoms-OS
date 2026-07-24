#include "abe_js_parser.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

static bool g_js_parser_initialized = false;

ABE_Error ABE_JSParser_Init(void) {
    g_js_parser_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSPARSER", "ABE JavaScript AST Parser initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSParser_Shutdown(void) {
    g_js_parser_initialized = false;
    return ABE_SUCCESS;
}

static ABE_ASTNode* CreateASTNode(ABE_ASTNodeType type) {
    ABE_ASTNode* node = (ABE_ASTNode*)kmalloc(sizeof(ABE_ASTNode));
    if (!node) return NULL;
    memset(node, 0, sizeof(ABE_ASTNode));
    node->type = type;
    return node;
}

void ABE_JSParser_FreeAST(ABE_ASTNode* node) {
    if (!node) return;
    ABE_ASTNode* child = node->first_child;
    while (child) {
        ABE_ASTNode* next = child->next_sibling;
        ABE_JSParser_FreeAST(child);
        child = next;
    }
    kfree(node);
}

static void AddChildNode(ABE_ASTNode* parent, ABE_ASTNode* child) {
    if (!parent || !child) return;
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        ABE_ASTNode* curr = parent->first_child;
        while (curr->next_sibling) {
            curr = curr->next_sibling;
        }
        curr->next_sibling = child;
    }
}

ABE_Error ABE_JSParser_Parse(const char* source, size_t length, ABE_ASTTree* out_ast) {
    if (!source || !out_ast) return ABE_ERR_INVALID_PARAM;

    ABE_JSLexer lexer;
    ABE_JSLexer_Init(&lexer, source, length);

    ABE_ASTNode* root = CreateASTNode(AST_PROGRAM);
    if (!root) return ABE_ERR_OUT_OF_MEMORY;

    ABE_JSToken token;
    while (ABE_JSLexer_NextToken(&lexer, &token) == ABE_SUCCESS && token.type != TOKEN_EOF) {
        if (token.type == TOKEN_KEYWORD && (strcmp(token.text, "var") == 0 || strcmp(token.text, "let") == 0 || strcmp(token.text, "const") == 0)) {
            ABE_ASTNode* decl = CreateASTNode(AST_VAR_DECLARATION);
            ABE_JSToken id_token;
            if (ABE_JSLexer_NextToken(&lexer, &id_token) == ABE_SUCCESS && id_token.type == TOKEN_IDENTIFIER) {
                strcpy(decl->val_str, id_token.text);
            }
            AddChildNode(root, decl);
        } else if (token.type == TOKEN_IDENTIFIER) {
            ABE_ASTNode* expr = CreateASTNode(AST_EXPRESSION_STATEMENT);
            ABE_ASTNode* id = CreateASTNode(AST_IDENTIFIER);
            strcpy(id->val_str, token.text);
            AddChildNode(expr, id);
            AddChildNode(root, expr);
        }
    }

    out_ast->root = root;
    out_ast->node_count = 1;
    return ABE_SUCCESS;
}
