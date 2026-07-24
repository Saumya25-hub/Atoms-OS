#ifndef ABE_JS_COMPILER_H
#define ABE_JS_COMPILER_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_js_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_NOP = 0,
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CALL,
    OP_RETURN,
    OP_GET_PROP,
    OP_SET_PROP,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_AWAIT
} ABE_JSOpcode;

typedef struct {
    uint8_t opcode;
    uint8_t r_dest;
    uint8_t r_src1;
    uint8_t r_src2;
    uint16_t immediate;
} ABE_JSInstruction;

typedef struct {
    ABE_JSInstruction instructions[256];
    uint32_t instruction_count;
    ABE_JSValue constants[64];
    uint32_t constant_count;
} ABE_JSBytecode;

ABE_Error ABE_JSCompiler_Init(void);
ABE_Error ABE_JSCompiler_Shutdown(void);

ABE_Error ABE_JSCompiler_Compile(ABE_ASTNode* ast_root, ABE_JSBytecode* out_bytecode);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_COMPILER_H
