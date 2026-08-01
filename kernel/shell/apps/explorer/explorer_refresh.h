#ifndef BSEC_EXPLORER_REFRESH_H
#define BSEC_EXPLORER_REFRESH_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    REFRESH_REASON_MANUAL = 0,
    REFRESH_REASON_FILE_CREATED,
    REFRESH_REASON_FILE_DELETED,
    REFRESH_REASON_FILE_RENAMED,
    REFRESH_REASON_USB_INSERTED,
    REFRESH_REASON_USB_REMOVED,
    REFRESH_REASON_PATH_CHANGED
} RefreshReason;

typedef struct {
    uint32_t      refresh_count;
    RefreshReason last_reason;
    bool          auto_refresh_enabled;
} ExplorerRefreshEngine;

void explorer_refresh_init(ExplorerRefreshEngine* re);
void explorer_refresh_trigger(ExplorerRefreshEngine* re, RefreshReason reason, const char* path);

#endif // BSEC_EXPLORER_REFRESH_H
