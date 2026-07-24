#ifndef ABE_JS_DIAG_H
#define ABE_JS_DIAG_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_JSDiag_Init(void);
ABE_Error ABE_JSDiag_Shutdown(void);

void ABE_JSDiag_DumpVMStats(ABE_JSContextHandle ctx);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_DIAG_H
