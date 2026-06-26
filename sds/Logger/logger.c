#include "../Include/sds.h"
#include "../Include/sds_database.h"
#include "../Include/console.h"

// Removed internal prototype hack

void _SDS_Report_Internal(const char* diagnostic_code,
                          SDS_Severity severity,
                          SDS_EnginePrefix engine,
                          const char* source_file,
                          const char* function_name,
                          uint32_t line_number,
                          const char* format, ...) 
{
    SDS_DiagnosticObject diag = {0};
    diag.diagnostic_code = diagnostic_code;
    diag.severity = severity;
    diag.engine = engine;
    diag.source_file = source_file;
    diag.function_name = function_name;
    diag.line_number = line_number;

    // Fetch Performance Metrics
    diag.metrics.timestamp_ms = _SDS_Platform_GetTimestamp();
    diag.metrics.thread_id = _SDS_Platform_GetThreadID();
    diag.metrics.cpu_core = _SDS_Platform_GetCPUCore();

    SDS_DatabaseEntry db_entry = {0};
    if (diagnostic_code && diagnostic_code[0] != '\0') {
        SDS_Database_Lookup(diagnostic_code, &db_entry);
        diag.owner = db_entry.owner;
        diag.description = db_entry.description;
        diag.suggested_fix = db_entry.fix;
        diag.wiki_id = db_entry.wiki_id;
    }

    char formatted_msg[SDS_MAX_MESSAGE_LEN] = {0};
    if (format && format[0] != '\0') {
        // Since we are baremetal and don't have vsnprintf yet, we just copy the format string directly
        int len = 0;
        while (format[len] && len < SDS_MAX_MESSAGE_LEN - 1) {
            formatted_msg[len] = format[len];
            len++;
        }
        formatted_msg[len] = '\0';
        diag.user_message = formatted_msg;
    } else {
        diag.user_message = "No additional context.";
    }

    _SDS_Console_FormatAndPrint(&diag);
}

void _SDS_Report_Assert_Internal(const char* condition_str, const char* source_file, const char* function_name, uint32_t line_number) {
    SDS_DiagnosticObject diag = {0};
    diag.diagnostic_code = "ASSERT-0000";
    diag.severity = SDS_SEVERITY_ASSERT;
    diag.engine = SDS_ENGINE_UNKNOWN;
    diag.owner = "System";
    diag.source_file = source_file;
    diag.function_name = function_name;
    diag.line_number = line_number;
    
    diag.metrics.timestamp_ms = _SDS_Platform_GetTimestamp();
    diag.metrics.thread_id = _SDS_Platform_GetThreadID();
    diag.metrics.cpu_core = _SDS_Platform_GetCPUCore();
    
    char assert_msg[SDS_MAX_MESSAGE_LEN] = {0};
    const char* prefix = "Assertion failed: ";
    int len = 0;
    while (prefix[len] && len < SDS_MAX_MESSAGE_LEN - 1) { assert_msg[len] = prefix[len]; len++; }
    int i = 0;
    while (condition_str[i] && len < SDS_MAX_MESSAGE_LEN - 1) { assert_msg[len++] = condition_str[i++]; }
    assert_msg[len] = '\0';
    
    diag.description = "Critical validation failed.";
    diag.user_message = assert_msg;
    diag.suggested_fix = "Review program logic at the source file.";
    diag.wiki_id = "N/A";

    _SDS_Console_FormatAndPrint(&diag);
}
