#include "../include/bar_api.h"

int32_t BAR_DispatchMessage(const BARMessage* msg) {
    if (!msg) return -1;
    // Dispatch to registered window handler or default proc
    return 0;
}
