/* kernel/net/core/net_packet.c - Zero-Allocation Static Packet Pool Allocator */
#include <kernel/net/net_framework.h>
#include <kernel/core/memory/vmm/include/vmm.h>
#include "kernel/core/lib/include/string.h"

static net_packet_t g_packet_pool[NET_PACKET_POOL_SIZE] __attribute__((aligned(64)));
static bool g_pool_initialized = false;

void net_packet_pool_init(void) {
    if (g_pool_initialized) return;

    void* pml4 = vmm_get_active_pml4();
    memset(g_packet_pool, 0, sizeof(g_packet_pool));

    for (int i = 0; i < NET_PACKET_POOL_SIZE; i++) {
        uint64_t virt = (uint64_t)(uintptr_t)&g_packet_pool[i].data[0];
        uint64_t phys = vmm_get_physical_address(pml4, virt);
        if (!phys) phys = virt;
        g_packet_pool[i].phys_addr = phys;
        g_packet_pool[i].in_use    = false;
        g_packet_pool[i].length    = 0;
        g_packet_pool[i].offset    = 0;
        g_packet_pool[i].next      = (i < NET_PACKET_POOL_SIZE - 1) ? &g_packet_pool[i + 1] : NULL;
    }

    g_pool_initialized = true;
}

net_packet_t* net_packet_allocate(void) {
    if (!g_pool_initialized) net_packet_pool_init();

    for (int i = 0; i < NET_PACKET_POOL_SIZE; i++) {
        if (!g_packet_pool[i].in_use) {
            g_packet_pool[i].in_use = true;
            g_packet_pool[i].length = 0;
            g_packet_pool[i].offset = 0;
            return &g_packet_pool[i];
        }
    }
    return NULL; // Pool exhausted
}

void net_packet_free(net_packet_t* pkt) {
    if (!pkt) return;
    pkt->length = 0;
    pkt->offset = 0;
    pkt->in_use = false;
}
