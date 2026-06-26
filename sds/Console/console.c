#include "../Include/console.h"


extern void _SDS_Core_DispatchOutput(const char* formatted_string, const SDS_DiagnosticObject* obj);
extern SDS_OutputLevel _SDS_Core_GetOutputLevel(void);

static const char* SeverityToString(SDS_Severity severity) {
    switch(severity) {
        case SDS_SEVERITY_VERBOSE:  return "VERBOSE";
        case SDS_SEVERITY_TRACE:    return "TRACE";
        case SDS_SEVERITY_INFO:     return "INFO";
        case SDS_SEVERITY_SUCCESS:  return "SUCCESS";
        case SDS_SEVERITY_WARNING:  return "WARNING";
        case SDS_SEVERITY_ERROR:    return "ERROR";
        case SDS_SEVERITY_CRITICAL: return "CRITICAL";
        case SDS_SEVERITY_FATAL:    return "FATAL";
        case SDS_SEVERITY_ASSERT:   return "ASSERT";
        default:                    return "UNKNOWN";
    }
}

static const char* SeverityToColor(SDS_Severity severity) {
    switch(severity) {
        case SDS_SEVERITY_FATAL:
        case SDS_SEVERITY_CRITICAL:
        case SDS_SEVERITY_ERROR:
        case SDS_SEVERITY_ASSERT:
            return "\x1B[31m";
        case SDS_SEVERITY_WARNING:
            return "\x1B[33m";
        case SDS_SEVERITY_SUCCESS:
            return "\x1B[32m";
        case SDS_SEVERITY_INFO:
        case SDS_SEVERITY_TRACE:
        case SDS_SEVERITY_VERBOSE:
        default:
            return "\x1B[36m";
    }
}

static void append_str(char* dst, const char* src, uint32_t max_len) {
    uint32_t len = 0;
    while (dst[len]) len++;
    while (*src && len < max_len - 1) {
        dst[len++] = *src++;
    }
    dst[len] = '\0';
}

static void append_num(char* dst, uint32_t num) {
    char buf[16];
    int i = 14;
    buf[15] = '\0';
    if (num == 0) {
        buf[i--] = '0';
    } else {
        while (num > 0 && i >= 0) {
            buf[i--] = (num % 10) + '0';
            num /= 10;
        }
    }
    append_str(dst, &buf[i + 1], SDS_MAX_FORMATTED_LEN);
}

void _SDS_Console_FormatAndPrint(const SDS_DiagnosticObject* obj) {
    if (!obj) return;

    SDS_OutputLevel level = _SDS_Core_GetOutputLevel();
    if (level == SDS_LEVEL_SILENT) return;

    char buffer[SDS_MAX_FORMATTED_LEN] = {0};
    const char* code = obj->diagnostic_code ? obj->diagnostic_code : "N/A";
    const char* desc = obj->description ? obj->description : "N/A";
    const char* fix = obj->suggested_fix ? obj->suggested_fix : "No fix available.";

    append_str(buffer, SeverityToColor(obj->severity), SDS_MAX_FORMATTED_LEN);

    switch (level) {
        case SDS_LEVEL_SHORT:
            append_str(buffer, "[", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, SeverityToString(obj->severity), SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "]\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, code, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, desc, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            break;

        case SDS_LEVEL_NORMAL:
            append_str(buffer, "[", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, SeverityToString(obj->severity), SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "]\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, code, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->owner ? obj->owner : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, desc, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nFix: ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, fix, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            break;

        case SDS_LEVEL_VERBOSE:
            append_str(buffer, "[", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, SeverityToString(obj->severity), SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "]\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, code, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nOwner: ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->owner ? obj->owner : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nDesc : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, desc, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nFile : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->source_file ? obj->source_file : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, ":", SDS_MAX_FORMATTED_LEN);
            append_num(buffer, obj->line_number);
            append_str(buffer, "\nFix  : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, fix, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n", SDS_MAX_FORMATTED_LEN);
            break;

        case SDS_LEVEL_DEVELOPER:
        default:
            append_str(buffer, "==============================================\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "             SDS Developer Report             \n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "==============================================\n[", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, SeverityToString(obj->severity), SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "]\n\nTimestamp : ", SDS_MAX_FORMATTED_LEN);
            append_num(buffer, obj->metrics.timestamp_ms);
            append_str(buffer, " ms\nCPU Core  : CPU", SDS_MAX_FORMATTED_LEN);
            append_num(buffer, obj->metrics.cpu_core);
            append_str(buffer, "\nThread    : Thread ", SDS_MAX_FORMATTED_LEN);
            append_num(buffer, obj->metrics.thread_id);
            append_str(buffer, "\n\nCode      : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, code, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nOwner     : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->owner ? obj->owner : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nFile      : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->source_file ? obj->source_file : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\nFunction  : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->function_name ? obj->function_name : "Unknown", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "()\nLine      : ", SDS_MAX_FORMATTED_LEN);
            append_num(buffer, obj->line_number);
            append_str(buffer, "\n\nDescription:\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, desc, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n\nMessage/Args:\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->user_message ? obj->user_message : "N/A", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n\nSuggested Fix:\n", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, fix, SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n\nDocs      : ", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, obj->wiki_id ? obj->wiki_id : "N/A", SDS_MAX_FORMATTED_LEN);
            append_str(buffer, "\n==============================================\n", SDS_MAX_FORMATTED_LEN);
            break;
    }

    append_str(buffer, "\x1B[0m", SDS_MAX_FORMATTED_LEN);
    _SDS_Core_DispatchOutput(buffer, obj);
}
