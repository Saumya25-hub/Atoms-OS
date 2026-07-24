#ifndef ABE_JS_PARSER_H
#define ABE_JS_PARSER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_js_lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AST_PROGRAM = 0,
    AST_VAR_DECLARATION,
    AST_EXPRESSION_STATEMENT,
    AST_BLOCK_STATEMENT,
    AST_IF_STATEMENT,
    AST_RETURN_STATEMENT,
    AST_FUNCTION_DECLARATION,
    AST_BINARY_EXPR,
    AST_CALL_EXPR,
    AST_MEMBER_EXPR,
    AST_IDENTIFIER,
    AST_LITERAL_NUMBER,
    AST_LITERAL_STRING,
    AST_OBJECT_LITERAL,
    AST_ARRAY_LITERAL
} ABE_ASTNodeType;

typedef struct ABE_ASTNode {
    ABE_ASTNodeType type;
    char val_str[128];
    double val_num;

    struct ABE_ASTNode* first_child;
    struct ABE_ASTNode* next_sibling;
} ABE_ASTNode;

typedef struct {
    ABE_ASTNode* root;
    uint32_t node_count;
} ABE_ASTTree;

ABE_Error ABE_JSParser_Init(void);
ABE_Error ABE_JSParser_Shutdown(void);

ABE_Error ABE_JSParser_Parse(const char* source, size_t length, ABE_ASTTree* out_ast);
void ABE_JSParser_FreeAST(ABE_ASTNode* node);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_PARSER_H
