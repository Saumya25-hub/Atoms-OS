#ifndef ABE_WEB_DIAG_H
#define ABE_WEB_DIAG_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_WebDiag_Init(void);
ABE_Error ABE_WebDiag_Shutdown(void);

void ABE_WebDiag_DumpStats(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_DIAG_H
