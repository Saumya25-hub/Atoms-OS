#include "../include/user32_api.h"

int32_t user32_input_dispatch_event(const MSG* msg) {
    if (!msg) return -1;
    return (int32_t)DispatchMessage(msg);
}
