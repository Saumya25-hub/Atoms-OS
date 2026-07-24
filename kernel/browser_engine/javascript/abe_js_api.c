#include "../../../sdk/include/abe/abe.h"
#include "abe_js_parser.h"
#include "abe_js_compiler.h"
#include "abe_js_vm.h"
#include "abe_js_gc.h"
#include "abe_js_objects.h"
#include "abe_js_promise.h"
#include "abe_js_event_loop.h"
#include "abe_js_dom_binding.h"
#include "abe_js_module.h"
#include "abe_js_diag.h"
#include "../diagnostics/abe_diagnostics.h"

static bool g_js_api_initialized = false;

ABE_Error ABE_JSInitialize(void) {
    if (g_js_api_initialized) return ABE_ERR_ALREADY_INITIALIZED;

    ABE_JSParser_Init();
    ABE_JSCompiler_Init();
    ABE_JSVM_Init();
    ABE_JSGC_Init();
    ABE_JSObjects_Init();
    ABE_JSPromise_Init();
    ABE_JSEventLoop_Init();
    ABE_JSDOMBinding_Init();
    ABE_JSModule_Init();
    ABE_JSDiag_Init();

    g_js_api_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSAPI", "ABE ECMAScript Runtime Public SDK Bridge initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSShutdown(void) {
    if (!g_js_api_initialized) return ABE_ERR_NOT_INITIALIZED;

    ABE_JSDiag_Shutdown();
    ABE_JSModule_Shutdown();
    ABE_JSDOMBinding_Shutdown();
    ABE_JSEventLoop_Shutdown();
    ABE_JSPromise_Shutdown();
    ABE_JSObjects_Shutdown();
    ABE_JSGC_Shutdown();
    ABE_JSVM_Shutdown();
    ABE_JSCompiler_Shutdown();
    ABE_JSParser_Shutdown();

    g_js_api_initialized = false;
    ABE_Log(ABE_LOG_INFO, "JSAPI", "ABE ECMAScript Runtime shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_CreateContext(ABE_DocumentHandle doc, ABE_JSContextHandle* out_ctx) {
    return ABE_JSVM_CreateContext(doc, out_ctx);
}

ABE_Error ABE_DestroyContext(ABE_JSContextHandle ctx) {
    return ABE_JSVM_DestroyContext(ctx);
}

ABE_Error ABE_ExecuteScript(ABE_JSContextHandle ctx, const char* src, size_t len, ABE_JSValue* out_val) {
    if (ctx == ABE_INVALID_HANDLE || !src) return ABE_ERR_INVALID_PARAM;

    ABE_ASTTree ast;
    ABE_Error err = ABE_JSParser_Parse(src, len, &ast);
    if (err != ABE_SUCCESS) return err;

    ABE_JSBytecode bc;
    err = ABE_JSCompiler_Compile(ast.root, &bc);
    if (err != ABE_SUCCESS) {
        ABE_JSParser_FreeAST(ast.root);
        return err;
    }

    err = ABE_JSVM_ExecuteBytecode(ctx, &bc, out_val);
    ABE_JSParser_FreeAST(ast.root);
    return err;
}

ABE_Error ABE_CompileScript(const char* src, size_t len, ABE_JSScriptHandle* out_script) {
    if (!src || !out_script) return ABE_ERR_INVALID_PARAM;
    *out_script = 1001; // Script handle ID
    return ABE_SUCCESS;
}

ABE_Error ABE_RunEventLoop(ABE_JSContextHandle ctx) {
    return ABE_JSEventLoop_RunTick(ctx);
}

ABE_Error ABE_GarbageCollect(ABE_JSContextHandle ctx) {
    return ABE_JSGC_Run(ctx);
}
