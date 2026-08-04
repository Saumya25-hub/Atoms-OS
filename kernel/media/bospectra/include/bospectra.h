#ifndef BOSPECTRA_H
#define BOSPECTRA_H

#include "bospectra_types.h"
#include "bospectra_errors.h"
#include "bospectra_media.h"

#ifdef __cplusplus
extern "C" {
#endif

// Master Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Init(void);
bospectra_error_t BOSPECTRA_Shutdown(void);
bospectra_state_t BOSPECTRA_GetState(void);

// Engine Semantic Versioning
void        BOSPECTRA_GetVersion(BOSPECTRA_Version* out_version);
const char* BOSPECTRA_GetVersionString(void);

// Engine Diagnostics & Self-Test APIs
bospectra_error_t BOSPECTRA_RunSelfTests(void);
void              BOSPECTRA_DumpDiagnostics(void);

#ifdef __cplusplus
}
#endif

#endif // BOSPECTRA_H
