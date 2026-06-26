#ifndef SDS_ASSERT_H
#define SDS_ASSERT_H

#include "sds_config.h"

// External hook to halt the system when an assertion fails.
extern void _SDS_Halt_System(void);
extern void _SDS_Report_Assert_Internal(const char* condition_str, const char* source_file, const char* function_name, uint32_t line_number);

#if SDS_ENABLE_ASSERTIONS

#define SDS_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            _SDS_Report_Assert_Internal(#condition, __FILE__, __func__, __LINE__); \
            _SDS_Halt_System(); \
        } \
    } while(0)

#else

// Assertions are removed in release builds with zero overhead
#define SDS_ASSERT(condition) do {} while(0)

#endif // SDS_ENABLE_ASSERTIONS

#endif // SDS_ASSERT_H
