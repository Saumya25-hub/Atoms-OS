#ifndef SYSTEM_HUB_H
#define SYSTEM_HUB_H

#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/drivers/rtc/rtc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Panel Type Active Enum
typedef enum {
    SYSTEM_HUB_PANEL_NONE = 0,
    SYSTEM_HUB_PANEL_QUICK_SETTINGS,
    SYSTEM_HUB_PANEL_CALENDAR,
    SYSTEM_HUB_PANEL_NOTIFICATIONS
} SystemHubPanelType;

// Notification Item Structure (Zero Heap Ring Buffer)
typedef struct {
    uint32_t id;
    uint32_t app_id;
    char title[32];
    char message[64];
    char time_str[16];
    uint32_t icon_id;
    bool is_read;
} BOSNotification;

// Public System Hub API
void SystemHub_Initialize(void);
void SystemHub_TogglePanel(SystemHubPanelType panel);
void SystemHub_ClosePanel(void);
SystemHubPanelType SystemHub_GetActivePanel(void);

// Notification Service API
void bos_notify_send(uint32_t app_id, const char* title, const char* message, uint32_t icon_id);
uint32_t bos_notify_get_unread_count(void);
void bos_notify_clear_all(void);

// Renderer & Event Hooks for Desktop Shell Overlay
void SystemHub_RenderCapsule(const BVFramebuffer* fb, int32_t screen_w, int32_t screen_h);
bool SystemHub_HandleCapsuleClick(int32_t mx, int32_t my, int32_t screen_w, int32_t screen_h);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_HUB_H
