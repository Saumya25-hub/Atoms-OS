#include "abe_js_compiler.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_js_compiler_initialized = false;

ABE_Error ABE_JSCompiler_Init(void) {
    g_js_compiler_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSCOMPILER", "ABE JavaScript Bytecode Compiler initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSCompiler_Shutdown(void) {
    g_js_compiler_initialized = false;
    return ABE_SUCCESS;
}

static void EmitInstruction(ABE_JSBytecode* bc, uint8_t op, uint8_t r_dest, uint8_t r_src1, uint8_t r_src2, uint16_t imm) {
    if (!bc || bc->instruction_count >= 256) return;
    ABE_JSInstruction* inst = &bc->instructions[bc->instruction_count++];
    inst->opcode = op;
    inst->r_dest = r_dest;
    inst->r_src1 = r_src1;
    inst->r_src2 = r_src2;
    inst->immediate = imm;
}

ABE_Error ABE_JSCompiler_Compile(ABE_ASTNode* ast_root, ABE_JSBytecode* out_bytecode) {
    if (!ast_root || !out_bytecode) return ABE_ERR_INVALID_PARAM;
    memset(out_bytecode, 0, sizeof(ABE_JSBytecode));

    ABE_ASTNode* child = ast_root->first_child;
    while (child) {
        if (child->type == AST_VAR_DECLARATION) {
            EmitInstruction(out_bytecode, OP_STORE_VAR, 0, 0, 0, 0);
        } else if (child->type == AST_EXPRESSION_STATEMENT) {
            EmitInstruction(out_bytecode, OP_LOAD_VAR, 0, 0, 0, 0);
        }
        child = child->next_sibling;
    }

    EmitInstruction(out_bytecode, OP_RETURN, 0, 0, 0, 0);
    return ABE_SUCCESS;
}
