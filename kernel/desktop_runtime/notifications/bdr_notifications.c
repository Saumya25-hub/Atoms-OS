#include "../include/bdr_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BDR_PushNotification(BDrSession* session, const char* title, const char* message, BDrNotificationType type) {
    if (!session || !session->active || !title || !message) return -1;

    if (session->notification_count < BDR_MAX_NOTIFICATIONS) {
        BDrNotification* notif = &session->notifications[session->notification_count++];
        strcpy(notif->title, title);
        strcpy(notif->message, message);
        notif->type = type;
        notif->active = true;
        return 0;
    }
    return -1;
}
