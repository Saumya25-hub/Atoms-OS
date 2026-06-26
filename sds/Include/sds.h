#ifndef SDS_H
#define SDS_H

#include "sds_types.h"
#include "sds_assert.h"

// Initialize the diagnostic system.
void SDS_Init(void);

// Configure the formatting verbosity for text outputs
void SDS_SetOutputLevel(SDS_OutputLevel level);

// Register an output provider (e.g. Console, Serial, Log File, BOVISUAL Debug UI)
bool SDS_RegisterOutputProvider(SDS_OutputProvider provider);

// Clear all registered providers
void SDS_ClearOutputProviders(void);

// Internal function called by the macros below. Should not be used directly.
void _SDS_Report_Internal(const char* diagnostic_code,
                          SDS_Severity severity,
                          SDS_EnginePrefix engine,
                          const char* source_file,
                          const char* function_name,
                          uint32_t line_number,
                          const char* format, ...);

// Platform hooks for metrics (To be provided by Kernel/OS)
extern uint32_t _SDS_Platform_GetTimestamp(void);
extern uint32_t _SDS_Platform_GetThreadID(void);
extern uint32_t _SDS_Platform_GetCPUCore(void);

#if SDS_ENABLE_DEBUG

#define SDS_Info(engine, code, ...) \
    _SDS_Report_Internal(code, SDS_SEVERITY_INFO, engine, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define SDS_Success(engine, code, ...) \
    _SDS_Report_Internal(code, SDS_SEVERITY_SUCCESS, engine, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define SDS_Warning(engine, code, ...) \
    _SDS_Report_Internal(code, SDS_SEVERITY_WARNING, engine, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define SDS_Error(engine, code, ...) \
    _SDS_Report_Internal(code, SDS_SEVERITY_ERROR, engine, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define SDS_Fatal(engine, code, ...) \
    _SDS_Report_Internal(code, SDS_SEVERITY_FATAL, engine, __FILE__, __func__, __LINE__, __VA_ARGS__)

#else

// Zero overhead macros for Release builds
#define SDS_Info(engine, code, ...)    do {} while(0)
#define SDS_Success(engine, code, ...) do {} while(0)
#define SDS_Warning(engine, code, ...) do {} while(0)
#define SDS_Error(engine, code, ...)   do {} while(0)
#define SDS_Fatal(engine, code, ...)   do {} while(0)

#endif // SDS_ENABLE_DEBUG

#endif // SDS_H
