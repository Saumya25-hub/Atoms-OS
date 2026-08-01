#include "../include/bosll_api.h"

uint64_t BosQuerySystemTime(void) {
    static uint64_t s_time = 1000000;
    return s_time++;
}
