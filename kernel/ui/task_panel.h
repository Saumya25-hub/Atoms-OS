#ifndef TASK_PANEL_H
#define TASK_PANEL_H

#include <stdint.h>
#include <stdbool.h>

#define TASKBAR_MAX_APPS 16

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} TaskbarRect;

typedef struct {
    uint32_t app_id;
    uint32_t asset_id;
    uint32_t win_id;
    const char* name;
    TaskbarRect bounds;
    bool is_running;
    bool is_focused;
} TaskbarAppSlot;

typedef struct {
    int32_t screen_w;
    int32_t screen_h;
    
    TaskbarRect capsule;
    int32_t corner_radius;
    
    // Left Zone: Start Pill
    TaskbarRect start_pill;
    TaskbarRect start_icon;
    TaskbarRect start_text;
    
    // Center Zone: App Slots
    uint32_t app_count;
    TaskbarAppSlot apps[TASKBAR_MAX_APPS];
    
    // Right Zone: System Tray & RTC Clock
    TaskbarRect tray_wifi;
    TaskbarRect tray_vol;
    TaskbarRect tray_clock;
} Taskbar_Layout;

void TaskPanel_Initialize(void);
void TaskPanel_Update(void);
const Taskbar_Layout* TaskPanel_GetLayout(void);

#endif // TASK_PANEL_H
