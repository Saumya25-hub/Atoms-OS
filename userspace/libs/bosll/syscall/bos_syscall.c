#include "../include/bosll_api.h"

int64_t BosDispatchSyscall(uint64_t syscallNumber, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    (void)syscallNumber; (void)arg1; (void)arg2; (void)arg3; (void)arg4;
    return 0; // Return zero status
}
