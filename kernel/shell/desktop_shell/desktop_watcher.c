#include "desktop_watcher.h"
#include "dom.h"
#include "desktop_vfs_sync.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void Shell_ShowNotification(const char* title, const char* msg, uint32_t ms);

#define WATCHER_QUEUE_SIZE 16
static FSEvent s_event_queue[WATCHER_QUEUE_SIZE];
static uint32_t s_queue_head = 0;
static uint32_t s_queue_tail = 0;
static uint32_t s_queue_count = 0;

void desktop_watcher_init(void) {
    s_queue_head = 0;
    s_queue_tail = 0;
    s_queue_count = 0;
    display_print("[WATCHER] Event-Driven Filesystem Watcher Engine Initialized.\n");
}

void desktop_watcher_notify(FSEventType type, const char* path, const char* extra) {
    if (s_queue_count >= WATCHER_QUEUE_SIZE) return;

    FSEvent* ev = &s_event_queue[s_queue_tail];
    ev->type = type;
    if (path) strncpy(ev->path, path, sizeof(ev->path) - 1);
    else ev->path[0] = '\0';

    if (extra) strncpy(ev->extra_info, extra, sizeof(ev->extra_info) - 1);
    else ev->extra_info[0] = '\0';

    s_queue_tail = (s_queue_tail + 1) % WATCHER_QUEUE_SIZE;
    s_queue_count++;

    // Immediately process event
    desktop_watcher_process_events();
}

void desktop_watcher_process_events(void) {
    while (s_queue_count > 0) {
        FSEvent ev = s_event_queue[s_queue_head];
        s_queue_head = (s_queue_head + 1) % WATCHER_QUEUE_SIZE;
        s_queue_count--;

        switch (ev.type) {
            case FS_EVENT_DRIVE_MOUNTED: {
                char title[64];
                strcpy(title, "Drive Mounted: ");
                strcat(title, ev.path);
                Shell_ShowNotification("Storage Hot-Plug", title, 4000);
                
                // Automatically add USB/Drive icon to desktop
                DesktopObject* obj = dom_create_object(ev.path, ev.path, DOM_OBJ_USB, 0);
                if (obj) {
                    extern void create_desktop_icon_from_object(DesktopObject* obj);
                    create_desktop_icon_from_object(obj);
                }
                break;
            }
            case FS_EVENT_DRIVE_UNMOUNTED: {
                char title[64];
                strcpy(title, "Drive Removed: ");
                strcat(title, ev.path);
                Shell_ShowNotification("Storage Hot-Plug", title, 4000);
                
                DesktopObject* obj = dom_find_by_path(ev.path);
                if (obj) {
                    desktop_crud_delete(ev.path, true);
                }
                break;
            }
            case FS_EVENT_FILE_CREATED: {
                desktop_vfs_sync_scan();
                break;
            }
            case FS_EVENT_FILE_DELETED: {
                dom_destroy_by_path(ev.path);
                break;
            }
            default: break;
        }
    }
}
