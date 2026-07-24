#ifndef ABE_JS_GC_H
#define ABE_JS_GC_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_JS_MAX_GC_OBJECTS 512

typedef struct {
    uint32_t handle;
    ABE_JSType type;
    bool is_marked;
    bool in_use;
    char data_str[256];
} ABE_JSGCObject;

typedef struct {
    ABE_JSGCObject heap[ABE_JS_MAX_GC_OBJECTS];
    uint32_t active_objects;
} ABE_JSGCHeap;

ABE_Error ABE_JSGC_Init(void);
ABE_Error ABE_JSGC_Shutdown(void);

ABE_Error ABE_JSGC_AllocObject(ABE_JSType type, uint32_t* out_handle);
ABE_Error ABE_JSGC_Run(ABE_JSContextHandle ctx_handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_GC_H
