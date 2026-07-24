#ifndef ABE_JS_H
#define ABE_JS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// JavaScript Bytecode Interpreter & DOM Bindings Subsystem
typedef struct {
    uint32_t instructions_executed;
    uint32_t heap_used_bytes;
    bool     has_error;
    char     error_msg[128];
} ABE_JSState;

void        ABE_JS_Init(void);
ABE_JSState ABE_JS_ExecuteScript(const char* script_code);
bool        ABE_JS_BindDOMElement(const char* element_id, void* node_ptr);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_H
