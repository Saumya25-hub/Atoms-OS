/* kernel/net/core/net_device.c - NETLIB Network Interface Registry & Manager */
#include <kernel/net/net_framework.h>
#include "kernel/core/lib/include/string.h"

#define MAX_NET_DEVICES 8

static net_device_t* g_registered_devices[MAX_NET_DEVICES];
static uint32_t g_device_count = 0;
static net_device_t* g_default_device = NULL;

void net_framework_init(void) {
    memset(g_registered_devices, 0, sizeof(g_registered_devices));
    g_device_count = 0;
    g_default_device = NULL;
}

bool net_device_register(net_device_t* dev) {
    if (!dev || g_device_count >= MAX_NET_DEVICES) return false;

    g_registered_devices[g_device_count++] = dev;
    if (!g_default_device) {
        g_default_device = dev;
    }
    return true;
}

net_device_t* net_device_get_default(void) {
    return g_default_device;
}
