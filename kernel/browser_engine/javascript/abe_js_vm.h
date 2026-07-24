#ifndef ABE_JS_VM_H
#define ABE_JS_VM_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_js_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_JS_MAX_REGISTERS 16
#define ABE_JS_MAX_CALL_STACK 64
#define ABE_MAX_JS_CONTEXTS 16

typedef struct {
    ABE_JSValue registers[ABE_JS_MAX_REGISTERS];
    uint32_t pc;
    ABE_JSBytecode* bytecode;
} ABE_JSStackFrame;

typedef struct {
    ABE_JSContextHandle handle;
    ABE_DocumentHandle doc_handle;
    ABE_JSStackFrame call_stack[ABE_JS_MAX_CALL_STACK];
    uint32_t stack_depth;
    ABE_JSValue global_scope;
    bool in_use;
} ABE_JSContext;

ABE_Error ABE_JSVM_Init(void);
ABE_Error ABE_JSVM_Shutdown(void);

ABE_Error ABE_JSVM_CreateContext(ABE_DocumentHandle doc_handle, ABE_JSContextHandle* out_ctx);
ABE_Error ABE_JSVM_DestroyContext(ABE_JSContextHandle handle);
ABE_JSContext* ABE_JSVM_GetContext(ABE_JSContextHandle handle);

ABE_Error ABE_JSVM_ExecuteBytecode(ABE_JSContextHandle ctx_handle, ABE_JSBytecode* bytecode, ABE_JSValue* out_val);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_VM_H
