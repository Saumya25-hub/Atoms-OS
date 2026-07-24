#include "abe_js_gc.h"
#include "abe_js_vm.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_JSGCHeap g_gc_heap;
static uint32_t g_next_obj_id = 8000;
static bool g_js_gc_initialized = false;

ABE_Error ABE_JSGC_Init(void) {
    memset(&g_gc_heap, 0, sizeof(ABE_JSGCHeap));
    g_js_gc_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSGC", "ABE JavaScript Mark-and-Sweep Garbage Collector initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSGC_Shutdown(void) {
    g_js_gc_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_JSGC_AllocObject(ABE_JSType type, uint32_t* out_handle) {
    if (!g_js_gc_initialized || !out_handle) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_GC_OBJECTS; i++) {
        if (!g_gc_heap.heap[i].in_use) {
            ABE_JSGCObject* obj = &g_gc_heap.heap[i];
            memset(obj, 0, sizeof(ABE_JSGCObject));
            obj->handle = (g_next_obj_id++) | (i << 16);
            obj->type = type;
            obj->in_use = true;
            g_gc_heap.active_objects++;

            *out_handle = obj->handle;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_JSGC_Run(ABE_JSContextHandle ctx_handle) {
    if (!g_js_gc_initialized) return ABE_ERR_NOT_INITIALIZED;

    // 1. Unmark all objects
    for (uint32_t i = 0; i < ABE_JS_MAX_GC_OBJECTS; i++) {
        if (g_gc_heap.heap[i].in_use) {
            g_gc_heap.heap[i].is_marked = false;
        }
    }

    // 2. Mark roots from stack frames
    ABE_JSContext* ctx = ABE_JSVM_GetContext(ctx_handle);
    if (ctx) {
        for (uint32_t f = 0; f < ctx->stack_depth; f++) {
            for (uint32_t r = 0; r < ABE_JS_MAX_REGISTERS; r++) {
                if (ctx->call_stack[f].registers[r].type == ABE_JS_TYPE_OBJECT) {
                    uint32_t handle = ctx->call_stack[f].registers[r].u.object_handle;
                    uint32_t slot = (handle >> 16) & 0xFFFF;
                    if (slot < ABE_JS_MAX_GC_OBJECTS && g_gc_heap.heap[slot].handle == handle) {
                        g_gc_heap.heap[slot].is_marked = true;
                    }
                }
            }
        }
    }

    // 3. Sweep unreferenced objects
    size_t reclaimed_bytes = 0;
    for (uint32_t i = 0; i < ABE_JS_MAX_GC_OBJECTS; i++) {
        if (g_gc_heap.heap[i].in_use && !g_gc_heap.heap[i].is_marked) {
            g_gc_heap.heap[i].in_use = false;
            g_gc_heap.active_objects--;
            reclaimed_bytes += sizeof(ABE_JSGCObject);
        }
    }

    ABE_Diag_RecordJSGCRun(reclaimed_bytes);
    ABE_LogVal(ABE_LOG_INFO, "JSGC", "Mark-and-Sweep GC completed, Active heap objects remaining: ", g_gc_heap.active_objects);
    return ABE_SUCCESS;
}
