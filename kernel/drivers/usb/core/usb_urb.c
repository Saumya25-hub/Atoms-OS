#include "kernel/drivers/usb/include/usb_spec.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

typedef struct USBURB {
    uint8_t slot_id;
    uint8_t endpoint_id;
    uint32_t transfer_type;
    void* buffer;
    uint32_t buffer_length;
    uint32_t actual_length;
    int32_t status;
    void (*complete_callback)(struct USBURB* urb);
} USBURB;

#define MAX_URBS 64
static USBURB g_urb_pool[MAX_URBS];
static bool g_urb_allocated[MAX_URBS];

void usb_core_urb_init(void) {
    display_print("[USB URB ENGINE] Initializing Asynchronous URB Pipeline\n");
    memset(g_urb_pool, 0, sizeof(g_urb_pool));
    memset(g_urb_allocated, 0, sizeof(g_urb_allocated));
}

USBURB* usb_core_alloc_urb(void) {
    for (int i = 0; i < MAX_URBS; i++) {
        if (!g_urb_allocated[i]) {
            g_urb_allocated[i] = true;
            memset(&g_urb_pool[i], 0, sizeof(USBURB));
            return &g_urb_pool[i];
        }
    }
    return NULL;
}

void usb_core_free_urb(USBURB* urb) {
    if (!urb) return;
    size_t idx = urb - g_urb_pool;
    if (idx < MAX_URBS) {
        g_urb_allocated[idx] = false;
    }
}
