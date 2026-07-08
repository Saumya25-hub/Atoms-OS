#include "pointer_consumers.h"
#include "kernel/drivers/display/display.h"

typedef struct {
    char name[32];
    PointerUpdateCallback callback;
    void* user_data;
    bool active;
} PointerConsumerEntry;

static PointerConsumerEntry g_pointer_consumers[POINTER_MAX_CONSUMERS];
static uint32_t g_pointer_consumer_count = 0;

void pointer_consumers_init(void) {
    for (int i = 0; i < POINTER_MAX_CONSUMERS; i++) {
        g_pointer_consumers[i].active = false;
    }
    g_pointer_consumer_count = 0;
    display_print("[POINTER CONSUMERS] Publish-Subscribe Dispatcher Initialized.\n");
}

bool pointer_consumers_register(const char* name, PointerUpdateCallback callback, void* user_data) {
    if (!callback || g_pointer_consumer_count >= POINTER_MAX_CONSUMERS) {
        return false;
    }

    PointerConsumerEntry* entry = &g_pointer_consumers[g_pointer_consumer_count++];
    int i = 0;
    while (name && name[i] && i < 31) {
        entry->name[i] = name[i];
        i++;
    }
    entry->name[i] = '\0';
    entry->callback = callback;
    entry->user_data = user_data;
    entry->active = true;

    display_print("[POINTER CONSUMERS] Registered Listener: ");
    display_print(entry->name);
    display_print("\n");
    return true;
}

void pointer_consumers_notify(const PointerState* state) {
    if (!state) return;

    for (uint32_t i = 0; i < g_pointer_consumer_count; i++) {
        if (g_pointer_consumers[i].active && g_pointer_consumers[i].callback) {
            g_pointer_consumers[i].callback(state, g_pointer_consumers[i].user_data);
        }
    }
}
