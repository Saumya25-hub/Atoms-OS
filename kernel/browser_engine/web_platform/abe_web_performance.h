#ifndef ABE_WEB_PERFORMANCE_H
#define ABE_WEB_PERFORMANCE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_WebPerformance_Init(void);
ABE_Error ABE_WebPerformance_Shutdown(void);

double ABE_WebPerformance_Now(void);
ABE_Error ABE_WebPerformance_Mark(const char* mark_name);
ABE_Error ABE_WebPerformance_Measure(const char* measure_name, const char* start_mark, const char* end_mark);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_PERFORMANCE_H
