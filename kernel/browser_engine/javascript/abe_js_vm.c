#include "abe_js_vm.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_JSContext g_contexts[ABE_MAX_JS_CONTEXTS];
static uint32_t g_next_ctx_id = 7000;
static bool g_js_vm_initialized = false;

ABE_Error ABE_JSVM_Init(void) {
    memset(g_contexts, 0, sizeof(g_contexts));
    g_js_vm_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSVM", "ABE JavaScript Virtual Machine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSVM_Shutdown(void) {
    if (!g_js_vm_initialized) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_JS_CONTEXTS; i++) {
        if (g_contexts[i].in_use) {
            ABE_JSVM_DestroyContext(g_contexts[i].handle);
        }
    }
    g_js_vm_initialized = false;
    return ABE_SUCCESS;
}

ABE_JSContext* ABE_JSVM_GetContext(ABE_JSContextHandle handle) {
    if (!g_js_vm_initialized || handle == ABE_INVALID_HANDLE) return NULL;
    uint32_t slot = (handle >> 16) & 0xFFFF;
    if (slot >= ABE_MAX_JS_CONTEXTS) return NULL;
    if (g_contexts[slot].handle == handle && g_contexts[slot].in_use) {
        return &g_contexts[slot];
    }
    return NULL;
}

ABE_Error ABE_JSVM_CreateContext(ABE_DocumentHandle doc_handle, ABE_JSContextHandle* out_ctx) {
    if (!g_js_vm_initialized || !out_ctx) return ABE_ERR_INVALID_PARAM;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_JS_CONTEXTS; i++) {
        if (!g_contexts[i].in_use) {
            slot = i;
            break;
        }
    }
    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_JSContext* ctx = &g_contexts[slot];
    memset(ctx, 0, sizeof(ABE_JSContext));
    ctx->handle = (g_next_ctx_id++) | (slot << 16);
    ctx->doc_handle = doc_handle;
    ctx->in_use = true;

    *out_ctx = ctx->handle;
    ABE_Diag_RecordJSContextCreated();
    ABE_LogVal(ABE_LOG_INFO, "JSVM", "Created JS Execution Context, Handle: ", ctx->handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_JSVM_DestroyContext(ABE_JSContextHandle handle) {
    if (!g_js_vm_initialized || handle == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    ABE_JSContext* ctx = ABE_JSVM_GetContext(handle);
    if (!ctx) return ABE_ERR_INVALID_PARAM;

    ctx->in_use = false;
    ABE_LogVal(ABE_LOG_INFO, "JSVM", "Destroyed JS Execution Context, Handle: ", handle);
    return ABE_SUCCESS;
}

ABE_Error ABE_JSVM_ExecuteBytecode(ABE_JSContextHandle ctx_handle, ABE_JSBytecode* bytecode, ABE_JSValue* out_val) {
    if (!g_js_vm_initialized || ctx_handle == ABE_INVALID_HANDLE || !bytecode) return ABE_ERR_INVALID_PARAM;
    ABE_JSContext* ctx = ABE_JSVM_GetContext(ctx_handle);
    if (!ctx) return ABE_ERR_INVALID_PARAM;

    if (ctx->stack_depth >= ABE_JS_MAX_CALL_STACK) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_JSStackFrame* frame = &ctx->call_stack[ctx->stack_depth++];
    memset(frame, 0, sizeof(ABE_JSStackFrame));
    frame->bytecode = bytecode;

    while (frame->pc < bytecode->instruction_count) {
        ABE_JSInstruction* inst = &bytecode->instructions[frame->pc++];
        ABE_Diag_RecordJSInstructionExecuted();

        switch (inst->opcode) {
            case OP_LOAD_CONST:
                frame->registers[inst->r_dest] = bytecode->constants[inst->immediate];
                break;
            case OP_STORE_VAR:
            case OP_LOAD_VAR:
                break;
            case OP_ADD:
                frame->registers[inst->r_dest].type = ABE_JS_TYPE_NUMBER;
                frame->registers[inst->r_dest].u.number_val = frame->registers[inst->r_src1].u.number_val + frame->registers[inst->r_src2].u.number_val;
                break;
            case OP_RETURN:
                if (out_val) *out_val = frame->registers[inst->r_dest];
                ctx->stack_depth--;
                ABE_Diag_RecordJSScriptExecuted(12);
                return ABE_SUCCESS;
            default:
                break;
        }
    }

    ctx->stack_depth--;
    if (out_val) out_val->type = ABE_JS_TYPE_UNDEFINED;
    ABE_Diag_RecordJSScriptExecuted(10);
    return ABE_SUCCESS;
}
