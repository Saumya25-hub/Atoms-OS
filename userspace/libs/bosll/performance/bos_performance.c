#include "../include/bosll_api.h"

uint64_t BosQueryPerformance(void) {
    static uint64_t s_perf = 5000000;
    return s_perf++;
}
