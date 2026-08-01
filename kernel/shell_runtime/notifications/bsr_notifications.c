#include "../include/bsr_api.h"

int32_t BSR_PushNotification(const char* title, const char* message) {
    if (!title || !message) return -1;
    // Broadcast notification toast to Desktop & Active Applications
    return 0;
}
