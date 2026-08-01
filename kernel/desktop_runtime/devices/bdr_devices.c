#include "../include/bdr_api.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

int32_t BDR_RefreshDevices(BDrSession* session) {
    if (!session || !session->active) return -1;

    // Check if USB drive (/U) is mounted and auto-spawn USB Drive Icon
    bool usb_mounted = (vfs_get_mount("/U") != NULL);
    bool usb_icon_found = false;

    for (uint32_t i = 0; i < session->icon_count; i++) {
        if (session->icons[i].type == BDR_ICON_TYPE_DRIVE) {
            usb_icon_found = true;
            if (!usb_mounted) {
                BDR_RemoveIcon(session, session->icons[i].icon_id);
                BDR_PushNotification(session, "Device Removed", "USB Drive (U:) disconnected", BDR_NOTIF_WARNING);
            }
            break;
        }
    }

    if (usb_mounted && !usb_icon_found) {
        BDR_AddIcon(session, "USB Drive (U:)", "/U", BDR_ICON_TYPE_DRIVE, 1, 0);
        BDR_PushNotification(session, "Device Connected", "USB Drive (U:) ready for use", BDR_NOTIF_SUCCESS);
    }
    return 0;
}
