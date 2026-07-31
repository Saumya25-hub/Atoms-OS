#include "platform/include/bos_types.h"
#include <stdio.h>
#include <stdarg.h>

void BOS_Platform_Log(const char* module, const char* fmt, ...) {
    if (!module || !fmt) return;

    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    /* Output log via platform diagnostic stream */
    printf("[PLATFORM::%s] %s\n", module, buffer);
}

const char* BOS_Platform_GetErrorString(BOS_Result result) {
    switch (result) {
        case BOS_SUCCESS:                 return "BOS_SUCCESS";
        case BOS_ERROR_INVALID_HANDLE:   return "BOS_ERROR_INVALID_HANDLE";
        case BOS_ERROR_INVALID_ARGUMENT: return "BOS_ERROR_INVALID_ARGUMENT";
        case BOS_ERROR_INVALID_ADDRESS:  return "BOS_ERROR_INVALID_ADDRESS";
        case BOS_ERROR_OUT_OF_MEMORY:     return "BOS_ERROR_OUT_OF_MEMORY";
        case BOS_ERROR_QUEUE_FULL:        return "BOS_ERROR_QUEUE_FULL";
        case BOS_ERROR_QUEUE_EMPTY:       return "BOS_ERROR_QUEUE_EMPTY";
        case BOS_ERROR_NOT_FOUND:         return "BOS_ERROR_NOT_FOUND";
        default:                          return "BOS_ERROR_UNKNOWN";
    }
}
