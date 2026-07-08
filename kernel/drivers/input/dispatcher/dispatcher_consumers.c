#include "dispatcher_consumers.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static DispatcherConsumer g_consumers[DISPATCHER_MAX_CONSUMERS];
static uint32_t g_consumer_count = 0;
static uint32_t g_next_id = 1;

void dispatcher_consumers_init(void) {
    g_consumer_count = 0;
    g_next_id = 1;
    memset(g_consumers, 0, sizeof(g_consumers));
}

uint32_t dispatcher_consumers_get_count(void) {
    return g_consumer_count;
}

const DispatcherConsumer* dispatcher_consumers_get_by_index(uint32_t index) {
    if (index >= g_consumer_count) {
        return 0;
    }
    return &g_consumers[index];
}

uint32_t dispatcher_consumers_register(const char* name, DispatcherPriorityTier priority,
                                       uint32_t event_mask, DispatcherCallback callback, void* context) {
    if (!callback || !dispatcher_priority_is_valid(priority)) {
        return 0;
    }

    if (g_consumer_count >= DISPATCHER_MAX_CONSUMERS) {
        display_print("[DISPATCHER] ERROR: Consumer registry full!\n");
        return 0;
    }

    // Find insertion index to maintain sorted priority order (lowest tier enum value first)
    uint32_t insert_idx = g_consumer_count;
    for (uint32_t i = 0; i < g_consumer_count; i++) {
        if ((int)priority < (int)g_consumers[i].priority) {
            insert_idx = i;
            break;
        }
    }

    // Shift elements right if inserting in the middle
    if (insert_idx < g_consumer_count) {
        for (uint32_t j = g_consumer_count; j > insert_idx; j--) {
            g_consumers[j] = g_consumers[j - 1];
        }
    }

    uint32_t new_id = g_next_id++;
    DispatcherConsumer* c = &g_consumers[insert_idx];
    c->id = new_id;
    c->priority = priority;
    c->event_type_mask = event_mask;
    c->enabled = true;
    c->context = context;
    c->callback = callback;

    if (name) {
        strncpy(c->name, name, sizeof(c->name) - 1);
        c->name[sizeof(c->name) - 1] = '\0';
    } else {
        strcpy(c->name, "Unnamed");
    }

    g_consumer_count++;
    return new_id;
}

bool dispatcher_consumers_unregister(uint32_t consumer_id) {
    if (consumer_id == 0) return false;

    for (uint32_t i = 0; i < g_consumer_count; i++) {
        if (g_consumers[i].id == consumer_id) {
            // Shift remaining elements left
            for (uint32_t j = i; j < g_consumer_count - 1; j++) {
                g_consumers[j] = g_consumers[j + 1];
            }
            g_consumer_count--;
            memset(&g_consumers[g_consumer_count], 0, sizeof(DispatcherConsumer));
            return true;
        }
    }
    return false;
}

bool dispatcher_consumers_set_state(uint32_t consumer_id, bool enabled) {
    for (uint32_t i = 0; i < g_consumer_count; i++) {
        if (g_consumers[i].id == consumer_id) {
            g_consumers[i].enabled = enabled;
            return true;
        }
    }
    return false;
}
