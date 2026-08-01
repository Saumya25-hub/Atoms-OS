#include "../include/kernel32_api.h"

uint64_t kernel32_performance_get_cpu_time(void) {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (uint64_t)li.QuadPart;
}
