#include "abe_core.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ABE_Core_Init(void) {
    bwe_log("INFO", "ABE Engine Core Subsystem V1.0 Initialized");
}
