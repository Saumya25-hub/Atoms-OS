#ifndef SDS_TYPES_H
#define SDS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "sds_config.h"

// Output Levels for Formatting
typedef enum {
    SDS_LEVEL_SILENT    = 0,
    SDS_LEVEL_SHORT     = 1, // End Users: Just Code & Message
    SDS_LEVEL_NORMAL    = 2, // Basic Dev: Code, Owner, Msg, Fix
    SDS_LEVEL_VERBOSE   = 3, // Advanced: + Env info
    SDS_LEVEL_DEVELOPER = 4  // Full Power: Everything
} SDS_OutputLevel;

// Engine Prefixes (Permanent Identifiers)
typedef enum {
    SDS_ENGINE_KR = 0,
    SDS_ENGINE_MM,
    SDS_ENGINE_FS,
    SDS_ENGINE_AT,
    SDS_ENGINE_RK,
    SDS_ENGINE_BV,
    SDS_ENGINE_VD,
    SDS_ENGINE_APP,
    SDS_ENGINE_UNKNOWN
} SDS_EnginePrefix;

// Severity Levels
typedef enum {
    SDS_SEVERITY_VERBOSE = 0,
    SDS_SEVERITY_TRACE,
    SDS_SEVERITY_INFO,
    SDS_SEVERITY_SUCCESS,
    SDS_SEVERITY_WARNING,
    SDS_SEVERITY_ERROR,
    SDS_SEVERITY_CRITICAL,
    SDS_SEVERITY_FATAL,
    SDS_SEVERITY_ASSERT
} SDS_Severity;

// Environmental Performance Data
typedef struct {
    uint32_t timestamp_ms; 
    uint32_t thread_id;
    uint32_t cpu_core;
} SDS_PerformanceMetrics;

// The Database representation of an error
typedef struct {
    const char* code;
    const char* description;
    const char* fix;
    SDS_Severity default_level;
    const char* owner;       
    const char* wiki_id;     
} SDS_DatabaseEntry;

// Core diagnostic object structure used internally during routing
typedef struct {
    const char* diagnostic_code; 
    SDS_Severity severity;
    SDS_EnginePrefix engine;
    
    // DB Populated
    const char* owner;
    const char* description;
    const char* suggested_fix;
    const char* wiki_id;
    
    // Environment
    const char* source_file;
    const char* function_name;
    uint32_t line_number;
    
    // Context Metrics
    SDS_PerformanceMetrics metrics;
    
    // Formatted User Message
    const char* user_message;
} SDS_DiagnosticObject;

// Output Provider Interface
typedef struct {
    const char* provider_name;
    void (*write_string)(const char* string);
    void (*write_raw)(const SDS_DiagnosticObject* obj);
} SDS_OutputProvider;

#endif // SDS_TYPES_H
