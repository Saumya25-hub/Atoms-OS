/* kernel/net/drivers/realtek/rtl8168.c - Isolated Certified RTL8168 Gigabit Driver */
#include <kernel/net/drivers/realtek/rtl8168.h>
#include <kernel/net/net_framework.h>
#include <kernel/core/memory/vmm/include/vmm.h>
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static net_device_t g_rtl8168_net_dev;
static rtl8168_private_t g_rtl8168_priv;

static struct rtl8168_tx_desc g_rtl8168_tx_ring[RTL8168_RING_SIZE] __attribute__((aligned(256)));
static struct rtl8168_rx_desc g_rtl8168_rx_ring[RTL8168_RING_SIZE] __attribute__((aligned(256)));
static uint8_t g_rtl8168_tx_buffers[RTL8168_RING_SIZE][1536] __attribute__((aligned(64)));
static uint8_t g_rtl8168_rx_buffers[RTL8168_RING_SIZE][1536] __attribute__((aligned(64)));

extern void ethernet_process_frame(const uint8_t* frame, uint16_t length);

static inline void rtl8168_write8(rtl8168_private_t* priv, uint32_t reg, uint8_t val) {
    if (priv->is_mmio) {
        *(volatile uint8_t*)(uintptr_t)(priv->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"((uint16_t)(priv->io_base + reg)));
    }
}

static inline uint8_t rtl8168_read8(rtl8168_private_t* priv, uint32_t reg) {
    if (priv->is_mmio) {
        return *(volatile uint8_t*)(uintptr_t)(priv->mmio_base + reg);
    } else {
        uint8_t ret;
        __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"((uint16_t)(priv->io_base + reg)));
        return ret;
    }
}

static inline uint16_t rtl8168_read16(rtl8168_private_t* priv, uint32_t reg) {
    if (priv->is_mmio) {
        return *(volatile uint16_t*)(uintptr_t)(priv->mmio_base + reg);
    } else {
        uint16_t ret;
        __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"((uint16_t)(priv->io_base + reg)));
        return ret;
    }
}

static inline void rtl8168_write16(rtl8168_private_t* priv, uint32_t reg, uint16_t val) {
    if (priv->is_mmio) {
        *(volatile uint16_t*)(uintptr_t)(priv->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"((uint16_t)(priv->io_base + reg)));
    }
}

static inline void rtl8168_write32(rtl8168_private_t* priv, uint32_t reg, uint32_t val) {
    if (priv->is_mmio) {
        *(volatile uint32_t*)(uintptr_t)(priv->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"((uint16_t)(priv->io_base + reg)));
    }
}

void rtl8168_reclaim_tx(net_device_t* dev) {
    rtl8168_private_t* priv = (rtl8168_private_t*)dev->driver_private;
    if (!priv) return;

    uint16_t isr = rtl8168_read16(priv, RTL8168_REG_ISR);
    if (isr) rtl8168_write16(priv, RTL8168_REG_ISR, isr);

    uint32_t tail = priv->tx_tail;
    uint32_t head = priv->tx_head;

    while (tail != head) {
        struct rtl8168_tx_desc* desc = &g_rtl8168_tx_ring[tail];
        __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
        __asm__ __volatile__("mfence" ::: "memory");

        if (desc->opts1 & (1U << 31)) break; // OWN bit still 1, HW transmitting

        dev->stats.tx_packets++;
        tail = (tail + 1) % RTL8168_RING_SIZE;
    }

    priv->tx_tail = tail;
}

bool rtl8168_xmit(net_device_t* dev, const void* frame, uint16_t length) {
    if (!dev || !frame || length == 0 || length > 1518) return false;
    rtl8168_private_t* priv = (rtl8168_private_t*)dev->driver_private;
    if (!priv) return false;

    rtl8168_reclaim_tx(dev);

    uint32_t next_head = (priv->tx_head + 1) % RTL8168_RING_SIZE;
    if (next_head == priv->tx_tail) {
        dev->stats.tx_dropped++;
        return false; // Ring full
    }

    uint32_t idx = priv->tx_head;
    struct rtl8168_tx_desc* desc = &g_rtl8168_tx_ring[idx];

    memcpy(g_rtl8168_tx_buffers[idx], frame, length);
    for (uint32_t off = 0; off < length; off += 64) {
        __asm__ __volatile__("clflush (%0)" :: "r"(&g_rtl8168_tx_buffers[idx][off]) : "memory");
    }
    __asm__ __volatile__("mfence" ::: "memory");

    uint32_t opts1 = (1U << 31) | (1U << 29) | (1U << 28) | ((uint32_t)length & 0x3FFF);
    if (idx == RTL8168_RING_SIZE - 1) opts1 |= (1U << 30); // EOR

    void* pml4 = vmm_get_active_pml4();
    uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_tx_buffers[idx]);
    if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_rtl8168_tx_buffers[idx];

    desc->opts2     = 0;
    desc->addr_low  = (uint32_t)buf_pa;
    desc->addr_high = (uint32_t)(buf_pa >> 32);

    __asm__ __volatile__("mfence" ::: "memory");
    desc->opts1 = opts1;

    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    priv->tx_head = next_head;

    uint64_t tx_ring_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_tx_ring);
    if (!tx_ring_pa) tx_ring_pa = (uint64_t)(uintptr_t)g_rtl8168_tx_ring;
    rtl8168_write32(priv, RTL8168_REG_TX_DESC_LOW, (uint32_t)tx_ring_pa);
    rtl8168_write8(priv, RTL8168_REG_TX_POLL, 0x40); // NPQ kick

    return true;
}

bool rtl8168_poll_rx(net_device_t* dev) {
    if (!dev) return false;
    rtl8168_private_t* priv = (rtl8168_private_t*)dev->driver_private;
    if (!priv) return false;

    rtl8168_reclaim_tx(dev);

    uint32_t idx = priv->rx_head;
    struct rtl8168_rx_desc* desc = &g_rtl8168_rx_ring[idx];

    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    if (desc->opts1 & (1U << 31)) return false; // OWN=1, no packet

    uint16_t pkt_len = (desc->opts1 & 0x3FFF);
    if (pkt_len >= 14 && pkt_len <= 1518) {
        dev->stats.rx_packets++;
        ethernet_process_frame(g_rtl8168_rx_buffers[idx], pkt_len);
    } else {
        dev->stats.rx_dropped++;
    }

    void* pml4 = vmm_get_active_pml4();
    uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_rx_buffers[idx]);
    if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_rtl8168_rx_buffers[idx];

    uint32_t opts1 = (1U << 31) | 1536;
    if (idx == RTL8168_RING_SIZE - 1) opts1 |= (1U << 30);

    desc->opts2     = 0;
    desc->addr_low  = (uint32_t)buf_pa;
    desc->addr_high = (uint32_t)(buf_pa >> 32);

    __asm__ __volatile__("mfence" ::: "memory");
    desc->opts1 = opts1;

    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    priv->rx_head = (idx + 1) % RTL8168_RING_SIZE;
    return true;
}

bool rtl8168_driver_probe(PCIDevice* pdev) {
    if (!pdev) return false;
    memset(&g_rtl8168_priv, 0, sizeof(g_rtl8168_priv));
    memset(&g_rtl8168_net_dev, 0, sizeof(g_rtl8168_net_dev));

    g_rtl8168_priv.pci_device = pdev;
    g_rtl8168_priv.net_dev    = &g_rtl8168_net_dev;

    pci_enable_bus_mastering(pdev);

    for (int i = 0; i < 6; i++) {
        if (pdev->bars[i].type == PCI_BAR_TYPE_MMIO32 || pdev->bars[i].type == PCI_BAR_TYPE_MMIO64) {
            g_rtl8168_priv.mmio_base = (uint32_t)pdev->bars[i].base_address;
            g_rtl8168_priv.is_mmio   = true;
            break;
        } else if (pdev->bars[i].type == PCI_BAR_TYPE_IO && g_rtl8168_priv.io_base == 0) {
            g_rtl8168_priv.io_base = (uint16_t)pdev->bars[i].base_address;
        }
    }

    // Reset Chip
    rtl8168_write8(&g_rtl8168_priv, RTL8168_REG_CHIP_CMD, RTL8168_CMD_RESET);
    while (rtl8168_read8(&g_rtl8168_priv, RTL8168_REG_CHIP_CMD) & RTL8168_CMD_RESET);

    // Read MAC
    for (int i = 0; i < 6; i++) {
        g_rtl8168_priv.mac_addr[i] = rtl8168_read8(&g_rtl8168_priv, RTL8168_REG_MAC0 + i);
        g_rtl8168_net_dev.mac_addr[i] = g_rtl8168_priv.mac_addr[i];
    }

    // Program Registers
    rtl8168_write8(&g_rtl8168_priv, RTL8168_REG_9346CR, RTL8168_9346_UNLOCK);
    rtl8168_write16(&g_rtl8168_priv, RTL8168_REG_CPLUS_CMD, 0x0220);
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_TX_CONFIG, 0x03000700);
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_RX_CONFIG, 0x0000E70F);

    void* pml4 = vmm_get_active_pml4();
    memset(g_rtl8168_tx_ring, 0, sizeof(g_rtl8168_tx_ring));
    for (int i = 0; i < RTL8168_RING_SIZE; i++) {
        uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_tx_buffers[i]);
        if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_rtl8168_tx_buffers[i];
        g_rtl8168_tx_ring[i].addr_low  = (uint32_t)buf_pa;
        g_rtl8168_tx_ring[i].addr_high = (uint32_t)(buf_pa >> 32);
    }
    g_rtl8168_tx_ring[RTL8168_RING_SIZE - 1].opts1 = (1 << 30);

    memset(g_rtl8168_rx_ring, 0, sizeof(g_rtl8168_rx_ring));
    for (int i = 0; i < RTL8168_RING_SIZE; i++) {
        uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_rx_buffers[i]);
        if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_rtl8168_rx_buffers[i];
        g_rtl8168_rx_ring[i].opts1 = (1U << 31) | 1536;
        g_rtl8168_rx_ring[i].addr_low  = (uint32_t)buf_pa;
        g_rtl8168_rx_ring[i].addr_high = (uint32_t)(buf_pa >> 32);
    }
    g_rtl8168_rx_ring[RTL8168_RING_SIZE - 1].opts1 |= (1U << 30);

    rtl8168_write16(&g_rtl8168_priv, RTL8168_REG_RMS, 1536);
    rtl8168_write8(&g_rtl8168_priv, RTL8168_REG_CHIP_CMD, RTL8168_CMD_RX_ENABLE | RTL8168_CMD_TX_ENABLE);

    uint64_t tx_ring_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_tx_ring);
    if (!tx_ring_pa) tx_ring_pa = (uint64_t)(uintptr_t)g_rtl8168_tx_ring;
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_TX_DESC_LOW, (uint32_t)tx_ring_pa);
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_TX_DESC_HIGH, (uint32_t)(tx_ring_pa >> 32));

    uint64_t rx_ring_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_rtl8168_rx_ring);
    if (!rx_ring_pa) rx_ring_pa = (uint64_t)(uintptr_t)g_rtl8168_rx_ring;
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_RX_DESC_LOW, (uint32_t)rx_ring_pa);
    rtl8168_write32(&g_rtl8168_priv, RTL8168_REG_RX_DESC_HIGH, (uint32_t)(rx_ring_pa >> 32));

    rtl8168_write16(&g_rtl8168_priv, RTL8168_REG_IMR, 0xFFFF);
    rtl8168_write16(&g_rtl8168_priv, RTL8168_REG_ISR, 0xFFFF);
    rtl8168_write8(&g_rtl8168_priv, RTL8168_REG_9346CR, RTL8168_9346_LOCK);

    // Bind NETLIB device ops
    strcpy(g_rtl8168_net_dev.name, "eth0");
    g_rtl8168_net_dev.pci_dev    = pdev;
    g_rtl8168_net_dev.vendor_id  = pdev->vendor_id;
    g_rtl8168_net_dev.device_id  = pdev->device_id;
    g_rtl8168_net_dev.revision_id= pdev->revision_id;
    g_rtl8168_net_dev.mmio_base  = g_rtl8168_priv.mmio_base;
    g_rtl8168_net_dev.io_base    = g_rtl8168_priv.io_base;
    g_rtl8168_net_dev.is_mmio    = g_rtl8168_priv.is_mmio;
    g_rtl8168_net_dev.driver_private = &g_rtl8168_priv;

    g_rtl8168_net_dev.ops.xmit       = rtl8168_xmit;
    g_rtl8168_net_dev.ops.poll_rx    = rtl8168_poll_rx;
    g_rtl8168_net_dev.ops.reclaim_tx = rtl8168_reclaim_tx;
    g_rtl8168_net_dev.link_up        = true;

    net_device_register(&g_rtl8168_net_dev);
    display_print("[RTL8168 DRIVER] Certified 1GbE Driver Active & Bound to eth0!\n");
    return true;
}
