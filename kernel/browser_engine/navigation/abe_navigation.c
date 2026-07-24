#include "abe_navigation.h"
#include "../diagnostics/abe_diagnostics.h"
#include "../resource/abe_resource.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

ABE_Error ABE_Navigation_Init(void) {
    ABE_Log(ABE_LOG_INFO, "NAV", "ABE Navigation Engine V1.0 initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_CreateHistory(ABE_NavigationHistory** out_history) {
    if (!out_history) return ABE_ERR_INVALID_PARAM;

    ABE_NavigationHistory* history = (ABE_NavigationHistory*)kmalloc(sizeof(ABE_NavigationHistory));
    if (!history) return ABE_ERR_OUT_OF_MEMORY;

    memset(history, 0, sizeof(ABE_NavigationHistory));
    history->state = NAV_STATE_IDLE;
    history->current_index = 0;
    history->total_count = 0;

    ABE_Diag_RecordMemoryAlloc(sizeof(ABE_NavigationHistory));
    *out_history = history;
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_FreeHistory(ABE_NavigationHistory* history) {
    if (!history) return ABE_ERR_INVALID_PARAM;
    kfree(history);
    ABE_Diag_RecordMemoryFree(sizeof(ABE_NavigationHistory));
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_LoadURL(ABE_NavigationHistory* history, const char* raw_url) {
    if (!history || !raw_url) return ABE_ERR_INVALID_PARAM;

    ABE_URL parsed_url;
    ABE_Error err = ABE_ParseURL(raw_url, &parsed_url);
    if (err != ABE_SUCCESS) {
        history->state = NAV_STATE_ERROR;
        return err;
    }

    history->state = NAV_STATE_CONNECTING;
    history->load_progress = 10;

    // If loading a new URL while viewing history entry, truncate forward stack
    if (history->total_count > 0 && history->current_index < history->total_count - 1) {
        history->total_count = history->current_index + 1;
    }

    // Check bounds
    if (history->total_count >= ABE_MAX_NAV_HISTORY) {
        // Shift history left
        for (uint32_t i = 0; i < ABE_MAX_NAV_HISTORY - 1; i++) {
            history->entries[i] = history->entries[i + 1];
        }
        history->total_count = ABE_MAX_NAV_HISTORY - 1;
        history->current_index = history->total_count - 1;
    }

    uint32_t new_idx = (history->total_count == 0) ? 0 : history->current_index + 1;
    if (history->total_count == 0) {
        new_idx = 0;
        history->total_count = 1;
    } else {
        history->total_count++;
    }
    history->current_index = new_idx;

    ABE_NavigationEntry* entry = &history->entries[new_idx];
    memset(entry, 0, sizeof(ABE_NavigationEntry));
    entry->url = parsed_url;
    strncpy(entry->title, parsed_url.raw_url, ABE_MAX_TITLE_LEN - 1);
    entry->timestamp = 1000;

    history->state = NAV_STATE_LOADING;
    history->load_progress = 50;

    // Simulate completion pipeline transition for Phase 1
    history->state = NAV_STATE_COMPLETE;
    history->load_progress = 100;
    ABE_Diag_RecordNavigation();

    ABE_Log(ABE_LOG_INFO, "NAV", "Navigation completed successfully for URL: ");
    ABE_Log(ABE_LOG_INFO, "NAV", parsed_url.raw_url);
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_Reload(ABE_NavigationHistory* history) {
    if (!history || history->total_count == 0) return ABE_ERR_INVALID_PARAM;
    const ABE_NavigationEntry* cur = ABE_Navigation_GetCurrentEntry(history);
    if (!cur) return ABE_ERR_INVALID_PARAM;

    history->state = NAV_STATE_LOADING;
    history->load_progress = 50;
    history->state = NAV_STATE_COMPLETE;
    history->load_progress = 100;
    ABE_Diag_RecordNavigation();
    ABE_Log(ABE_LOG_INFO, "NAV", "Reload completed");
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_Stop(ABE_NavigationHistory* history) {
    if (!history) return ABE_ERR_INVALID_PARAM;
    if (history->state == NAV_STATE_CONNECTING || history->state == NAV_STATE_LOADING || history->state == NAV_STATE_PARSING) {
        history->state = NAV_STATE_STOPPED;
        ABE_Log(ABE_LOG_INFO, "NAV", "Navigation halted by user request");
    }
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_GoBack(ABE_NavigationHistory* history) {
    if (!history) return ABE_ERR_INVALID_PARAM;
    if (!ABE_Navigation_CanGoBack(history)) return ABE_ERR_NAVIGATION_FAILED;

    history->current_index--;
    history->state = NAV_STATE_COMPLETE;
    history->load_progress = 100;
    ABE_Diag_RecordNavigation();
    ABE_Log(ABE_LOG_INFO, "NAV", "Navigated BACK to history index");
    return ABE_SUCCESS;
}

ABE_Error ABE_Navigation_GoForward(ABE_NavigationHistory* history) {
    if (!history) return ABE_ERR_INVALID_PARAM;
    if (!ABE_Navigation_CanGoForward(history)) return ABE_ERR_NAVIGATION_FAILED;

    history->current_index++;
    history->state = NAV_STATE_COMPLETE;
    history->load_progress = 100;
    ABE_Diag_RecordNavigation();
    ABE_Log(ABE_LOG_INFO, "NAV", "Navigated FORWARD to history index");
    return ABE_SUCCESS;
}

bool ABE_Navigation_CanGoBack(const ABE_NavigationHistory* history) {
    if (!history || history->total_count == 0) return false;
    return (history->current_index > 0);
}

bool ABE_Navigation_CanGoForward(const ABE_NavigationHistory* history) {
    if (!history || history->total_count == 0) return false;
    return (history->current_index < history->total_count - 1);
}

const ABE_NavigationEntry* ABE_Navigation_GetCurrentEntry(const ABE_NavigationHistory* history) {
    if (!history || history->total_count == 0 || history->current_index >= history->total_count) return NULL;
    return &history->entries[history->current_index];
}
