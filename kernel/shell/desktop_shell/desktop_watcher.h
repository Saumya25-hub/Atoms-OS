#ifndef DESKTOP_WATCHER_H
#define DESKTOP_WATCHER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    FS_EVENT_FILE_CREATED = 1,
    FS_EVENT_FILE_DELETED,
    FS_EVENT_FILE_RENAMED,
    FS_EVENT_FILE_MODIFIED,
    FS_EVENT_DRIVE_MOUNTED,
    FS_EVENT_DRIVE_UNMOUNTED
} FSEventType;

typedef struct {
    FSEventType type;
    char path[128];
    char extra_info[64];
} FSEvent;

// Watcher API
void desktop_watcher_init(void);
void desktop_watcher_notify(FSEventType type, const char* path, const char* extra);
void desktop_watcher_process_events(void);

#endif // DESKTOP_WATCHER_H
