#include "../include/brt_api.h"
#include "kernel/shell_runtime/include/bsr_api.h"

int32_t BRT_PushNotification(const char* title, const char* message) {
    return BSR_PushNotification(title, message);
}
