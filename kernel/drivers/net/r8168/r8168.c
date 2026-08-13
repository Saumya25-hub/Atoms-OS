#include "r8168.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/net/net_framework.h"

#define R8168_TX_RING_SIZE 256
#define R8168_RX_RING_SIZE 256

static R8168Device g_r8168_dev = {0};
static net_device_t g_r8168_netdev = {0};

static bool r8168_netdev_xmit(struct net_device* dev, const void* frame, uint16_t length) {
    (void)dev;
    return r8168_transmit_raw(frame, length);
}

static bool r8168_netdev_poll(struct net_device* dev) {
    (void)dev;
    return r8168_poll_receive();
}

static void r8168_netdev_reclaim(struct net_device* dev) {
    (void)dev;
    r8168_tx_reclaim();
}

static struct r8168_tx_desc g_r8168_tx_ring[R8168_TX_RING_SIZE] __attribute__((aligned(256)));
static uint8_t g_r8168_tx_buffers[R8168_TX_RING_SIZE][1536] __attribute__((aligned(16)));

static struct r8168_rx_desc g_r8168_rx_ring[R8168_RX_RING_SIZE] __attribute__((aligned(256)));
static uint8_t g_r8168_rx_buffers[R8168_RX_RING_SIZE][1536] __attribute__((aligned(16)));

// RX diagnostic counters
volatile uint64_t g_rx_frames      = 0;
volatile uint64_t g_rx_arp_frames  = 0;
volatile uint64_t g_rx_ipv4_frames = 0;
volatile uint64_t g_rx_drop_count  = 0;

// TX diagnostic counters (producer/consumer model & forensic pipeline tracing)
volatile uint64_t g_tx_try_count       = 0;  // Every transmit attempt
volatile uint64_t g_tx_ok_count        = 0;  // Descriptors confirmed sent (hardware OWN cleared)
volatile uint64_t g_tx_drop_count      = 0;  // Ring full drop count
volatile uint32_t g_tx_head_snapshot   = 0; // Producer index
volatile uint32_t g_tx_tail_snapshot   = 0; // Consumer index

volatile uint64_t g_r8168_xmit_calls   = 0; // Driver xmit entry count
volatile uint64_t g_tx_desc_used       = 0; // Descriptor assigned OWN=1 count
volatile uint64_t g_tx_doorbell_writes = 0; // Hardware Doorbell ring count
volatile uint64_t g_tx_reclaim_count   = 0; // Reclaim scan execution count

extern void ethernet_process_frame(const uint8_t* frame, uint16_t length);
extern void netif_init(void);
extern void netif_set_config(uint32_t ip, uint32_t mask, uint32_t gw, uint32_t dns, uint32_t dhcp_server, uint32_t lease_time, uint32_t t1, uint32_t t2);

static inline void r8168_write8(R8168Device* dev, uint32_t reg, uint8_t val) {
    if (dev->is_mmio) {
        *(volatile uint8_t*)(uintptr_t)(dev->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"((uint16_t)(dev->io_base + reg)));
    }
}

static inline uint8_t r8168_read8(R8168Device* dev, uint32_t reg) {
    if (dev->is_mmio) {
        return *(volatile uint8_t*)(uintptr_t)(dev->mmio_base + reg);
    } else {
        uint8_t ret;
        __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"((uint16_t)(dev->io_base + reg)));
        return ret;
    }
}

static inline uint16_t r8168_read16(R8168Device* dev, uint32_t reg) {
    if (dev->is_mmio) {
        return *(volatile uint16_t*)(uintptr_t)(dev->mmio_base + reg);
    } else {
        uint16_t ret;
        __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"((uint16_t)(dev->io_base + reg)));
        return ret;
    }
}

static inline void r8168_write16(R8168Device* dev, uint32_t reg, uint16_t val) {
    if (dev->is_mmio) {
        *(volatile uint16_t*)(uintptr_t)(dev->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"((uint16_t)(dev->io_base + reg)));
    }
}

static inline void r8168_write32(R8168Device* dev, uint32_t reg, uint32_t val) {
    if (dev->is_mmio) {
        *(volatile uint32_t*)(uintptr_t)(dev->mmio_base + reg) = val;
    } else {
        __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"((uint16_t)(dev->io_base + reg)));
    }
}

static inline uint32_t r8168_read32(R8168Device* dev, uint32_t reg) {
    if (dev->is_mmio) {
        return *(volatile uint32_t*)(uintptr_t)(dev->mmio_base + reg);
    } else {
        uint32_t ret;
        __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"((uint16_t)(dev->io_base + reg)));
        return ret;
    }
}

static uint16_t r8168_intr_mask_reg(void) {
    return g_r8168_dev.is_rtl8125 ? R8168_REG_IMR_8125 : R8168_REG_IMR;
}

static uint16_t r8168_intr_status_reg(void) {
    return g_r8168_dev.is_rtl8125 ? R8168_REG_ISR_8125 : R8168_REG_ISR;
}

static void r8168_tx_doorbell(void) {
    g_tx_doorbell_writes++;
    if (g_r8168_dev.is_rtl8125) {
        // [ATOMS OS RTL8125 FIX]
        // RTL8125 Rev 05 ignores legacy 0x38 (now IMR).
        // It requires a 16-bit write to TxPoll_8125 (0x90) with value BIT(0) (0x0001)
        r8168_write16(&g_r8168_dev, 0x90, 0x0001);
    } else {
        r8168_write8(&g_r8168_dev, R8168_REG_TX_POLL, 0x40);
    }
}

static uint16_t r8168_mac_ocp_read(uint32_t reg) {
    r8168_write32(&g_r8168_dev, R8168_REG_OCPDR, reg << 15);
    return (uint16_t)(r8168_read32(&g_r8168_dev, R8168_REG_OCPDR) & 0xFFFF);
}

static void r8168_mac_ocp_write(uint32_t reg, uint16_t value) {
    r8168_write32(&g_r8168_dev, R8168_REG_OCPDR, 0x80000000U | (reg << 15) | value);
}

static void r8168_mac_ocp_modify(uint32_t reg, uint16_t clear_mask, uint16_t set_bits) {
    uint16_t value = r8168_mac_ocp_read(reg);
    r8168_mac_ocp_write(reg, (uint16_t)((value & ~clear_mask) | set_bits));
}

static void r8168_init_rtl8125_gates(void) {
    if (!g_r8168_dev.is_rtl8125) return;

    r8168_write8(&g_r8168_dev, R8168_REG_INT_CFG0_8125, 0x00);
    for (uint32_t reg = 0x0A00; reg < 0x0A80; reg += 4) {
        r8168_write32(&g_r8168_dev, reg, 0x00000000);
    }
    r8168_write16(&g_r8168_dev, R8168_REG_INT_CFG1_8125, 0x0000);
    r8168_write32(&g_r8168_dev, R8168_REG_RSS_CTRL_8125, 0x00000000);
    r8168_write16(&g_r8168_dev, R8168_REG_QNUM_CTRL_8125, 0x0000);

    /* Linux r8169 keeps the standard 16-byte descriptor format on RTL8125. */
    r8168_mac_ocp_modify(0xEB58, 0x0001, 0x0000);
    r8168_mac_ocp_modify(0xEB54, 0x0000, 0x0001);
    for (volatile int i = 0; i < 1000; i++) __asm__ __volatile__("nop");
    r8168_mac_ocp_modify(0xEB54, 0x0001, 0x0000);
    r8168_write16(&g_r8168_dev, 0x1880, r8168_read16(&g_r8168_dev, 0x1880) & (uint16_t)~0x0030);
}

R8168Device* r8168_get_device(void) {
    return &g_r8168_dev;
}

void r8168_init(void) {
    g_r8168_dev.state = R8168_STATE_UNINITIALIZED;
    g_r8168_dev.pci_device = NULL;

    display_print("\n=== REALTEK RTL8111/R8168 HARDWARE INIT ===\n\n");

    uint32_t dev_count = pci_get_device_count();
    PCIDevice* matched_pci = NULL;

    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice* pdev = pci_get_device(i);
        if (pdev && (pdev->vendor_id == 0x10EC || pdev->base_class == 0x02)) {
            matched_pci = pdev;
            break;
        }
    }

    if (!matched_pci) {
        display_print("[R8168] No Realtek 0x10EC hardware detected on PCI bus.\n\n");
        g_r8168_dev.state = R8168_STATE_FAILED;
        return;
    }

    g_r8168_dev.pci_device = matched_pci;
    g_r8168_dev.is_rtl8125 = (matched_pci->vendor_id == 0x10EC && matched_pci->device_id == 0x8125);
    g_r8168_dev.state = R8168_STATE_PCI_FOUND;

    pci_enable_bus_mastering(matched_pci);
    pci_enable_memory_space(matched_pci);
    pci_enable_io_space(matched_pci);

    uint64_t bar0_addr = matched_pci->bars[0].base_address;
    PCIBarType bar0_type = matched_pci->bars[0].type;
    uint64_t bar2_addr = matched_pci->bars[2].base_address;
    PCIBarType bar2_type = matched_pci->bars[2].type;

    if (bar0_type == PCI_BAR_TYPE_IO && bar0_addr != 0) {
        g_r8168_dev.is_mmio = false;
        g_r8168_dev.io_base = (uint16_t)bar0_addr;
        display_print("[R8168] Operating Mode: IO Port Base 0x");
        display_print_hex(g_r8168_dev.io_base); display_print("\n");
    } else if (bar2_type == PCI_BAR_TYPE_IO && bar2_addr != 0) {
        g_r8168_dev.is_mmio = false;
        g_r8168_dev.io_base = (uint16_t)bar2_addr;
        display_print("[R8168] Operating Mode: BAR2 IO Port Base 0x");
        display_print_hex(g_r8168_dev.io_base); display_print("\n");
    } else if (bar0_addr != 0) {
        g_r8168_dev.is_mmio = true;
        g_r8168_dev.mmio_base = (uint32_t)bar0_addr;
        display_print("[R8168] Operating Mode: MMIO Base 0x");
        display_print_hex(g_r8168_dev.mmio_base); display_print("\n");
    } else {
        g_r8168_dev.is_mmio = true;
        g_r8168_dev.mmio_base = (uint32_t)bar2_addr;
        display_print("[R8168] Operating Mode: BAR2 MMIO Base 0x");
        display_print_hex(g_r8168_dev.mmio_base); display_print("\n");
    }

    // Read MAC Address
    for (int i = 0; i < 6; i++) {
        g_r8168_dev.mac_addr[i] = r8168_read8(&g_r8168_dev, R8168_REG_MAC0 + i);
    }

    display_print("[R8168] MAC Address: ");
    for (int i = 0; i < 6; i++) {
        display_print_hex(g_r8168_dev.mac_addr[i]);
        if (i < 5) display_print(":");
    }
    display_print("\n");

    // Unlock Configuration Registers
    r8168_write8(&g_r8168_dev, R8168_REG_9346CR, R8168_9346_UNLOCK);

    // Software Reset
    r8168_write8(&g_r8168_dev, R8168_REG_CHIP_CMD, R8168_CMD_RESET);
    for (volatile int timeout = 0; timeout < 100000; timeout++) {
        if ((r8168_read8(&g_r8168_dev, R8168_REG_CHIP_CMD) & R8168_CMD_RESET) == 0) break;
    }

    r8168_write8(&g_r8168_dev, R8168_REG_9346CR, R8168_9346_UNLOCK);
    r8168_init_rtl8125_gates();

    // Mark TX Descriptor Ring memory as Uncacheable (DMA-safe)
    void* pml4 = vmm_get_active_pml4();
    uint64_t tx_ring_va = (uint64_t)(uintptr_t)g_r8168_tx_ring;
    uint64_t tx_buf_va  = (uint64_t)(uintptr_t)g_r8168_tx_buffers;
    // Remap descriptor ring pages as UC using true physical addresses
    for (uint64_t off = 0; off < sizeof(g_r8168_tx_ring); off += 0x1000) {
        uint64_t virt = tx_ring_va + off;
        uint64_t phys = vmm_get_physical_address(pml4, virt);
        if (!phys) phys = virt;
        vmm_map_page(pml4, phys, virt, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    // Remap TX buffer pages as UC using true physical addresses
    for (uint64_t off = 0; off < sizeof(g_r8168_tx_buffers); off += 0x1000) {
        uint64_t virt = tx_buf_va + off;
        uint64_t phys = vmm_get_physical_address(pml4, virt);
        if (!phys) phys = virt;
        vmm_map_page(pml4, phys, virt, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    // Flush TLB after remapping
    __asm__ __volatile__("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    // Initialize TX Descriptor Ring
    memset(g_r8168_tx_ring, 0, sizeof(g_r8168_tx_ring));
    for (int i = 0; i < R8168_TX_RING_SIZE; i++) {
        uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_r8168_tx_buffers[i]);
        if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_r8168_tx_buffers[i];
        g_r8168_tx_ring[i].opts1 = 0;
        g_r8168_tx_ring[i].opts2 = 0;
        g_r8168_tx_ring[i].addr_low  = (uint32_t)buf_pa;
        g_r8168_tx_ring[i].addr_high = (uint32_t)(buf_pa >> 32);
    }
    // Mark last descriptor with End of Ring (EOR) bit
    g_r8168_tx_ring[R8168_TX_RING_SIZE - 1].opts1 = (1 << 30);

    g_r8168_dev.tx_head = 0;

    // Configure TX & C+ Command Registers FIRST before setting ring addresses
    r8168_write16(&g_r8168_dev, R8168_REG_CPLUS_CMD, g_r8168_dev.is_rtl8125 ? 0x0020 : 0x0220);
    r8168_write32(&g_r8168_dev, R8168_REG_TX_CONFIG, 0x03000700); // 1024B DMA burst

    // Configure RX Register (Accept Broadcast, Multicast, Match MAC, All Packets, 1024B DMA Burst)
    r8168_write32(&g_r8168_dev, R8168_REG_RX_CONFIG, 0x0000E70F);

    // Initialize RX Descriptor Ring (Hand descriptors to NIC hardware with OWN=1)
    memset(g_r8168_rx_ring, 0, sizeof(g_r8168_rx_ring));
    for (int i = 0; i < R8168_RX_RING_SIZE; i++) {
        uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_r8168_rx_buffers[i]);
        if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_r8168_rx_buffers[i];
        g_r8168_rx_ring[i].opts1 = (1U << 31) | 1536; // OWN bit set, 1536B buffer capacity
        g_r8168_rx_ring[i].opts2 = 0;
        g_r8168_rx_ring[i].addr_low  = (uint32_t)buf_pa;
        g_r8168_rx_ring[i].addr_high = (uint32_t)(buf_pa >> 32);
    }
    g_r8168_rx_ring[R8168_RX_RING_SIZE - 1].opts1 |= (1U << 30); // End of Ring (EOR)

    g_r8168_dev.rx_head = 0;

    // Set Max Rx Packet Size (RMS = 1536 bytes) so hardware does not drop incoming frames
    r8168_write16(&g_r8168_dev, R8168_REG_RMS, 1536);

    // Enable RX and TX Engine FIRST before writing ring base addresses
    r8168_write8(&g_r8168_dev, R8168_REG_CHIP_CMD, R8168_CMD_RX_ENABLE | R8168_CMD_TX_ENABLE);

    // Assign TRUE Physical TX Descriptor Address to NIC Hardware (High 32-bit before Low 32-bit as per Realtek spec)
    uint64_t rx_ring_va = (uint64_t)(uintptr_t)g_r8168_rx_ring;

    uint64_t tx_ring_pa = vmm_get_physical_address(pml4, tx_ring_va);
    if (!tx_ring_pa) tx_ring_pa = tx_ring_va;
    r8168_write32(&g_r8168_dev, R8168_REG_TX_DESC_HIGH, (uint32_t)(tx_ring_pa >> 32));
    r8168_write32(&g_r8168_dev, R8168_REG_TX_DESC_LOW, (uint32_t)tx_ring_pa);

    // Assign TRUE Physical RX Descriptor Address to NIC Hardware (High 32-bit before Low 32-bit)
    uint64_t rx_ring_pa = vmm_get_physical_address(pml4, rx_ring_va);
    if (!rx_ring_pa) rx_ring_pa = rx_ring_va;
    r8168_write32(&g_r8168_dev, R8168_REG_RX_DESC_HIGH, (uint32_t)(rx_ring_pa >> 32));
    r8168_write32(&g_r8168_dev, R8168_REG_RX_DESC_LOW, (uint32_t)rx_ring_pa);

    // Unmask Interrupts (IMR = 0xFFFF) so hardware DMA status writeback engine is active
    r8168_write16(&g_r8168_dev, r8168_intr_mask_reg(), 0xFFFF);
    r8168_write16(&g_r8168_dev, r8168_intr_status_reg(), 0xFFFF);

    // Lock Configuration Registers (0x00) to enter normal operational DMA mode
    r8168_write8(&g_r8168_dev, R8168_REG_9346CR, R8168_9346_LOCK);

    g_r8168_dev.state = R8168_STATE_READY;

    // Register into NETLIB framework so ethernet_send and debuglan_send_raw execute r8168_transmit_raw
    memset(&g_r8168_netdev, 0, sizeof(net_device_t));
    strcpy(g_r8168_netdev.name, "eth0");
    memcpy(g_r8168_netdev.mac_addr, g_r8168_dev.mac_addr, 6);
    g_r8168_netdev.pci_dev = matched_pci;
    g_r8168_netdev.vendor_id = matched_pci->vendor_id;
    g_r8168_netdev.device_id = matched_pci->device_id;
    g_r8168_netdev.mmio_base = g_r8168_dev.mmio_base;
    g_r8168_netdev.io_base = g_r8168_dev.io_base;
    g_r8168_netdev.is_mmio = g_r8168_dev.is_mmio;
    g_r8168_netdev.link_up = true;
    g_r8168_netdev.ops.xmit = r8168_netdev_xmit;
    g_r8168_netdev.ops.poll_rx = r8168_netdev_poll;
    g_r8168_netdev.ops.reclaim_tx = r8168_netdev_reclaim;

    extern void net_framework_init(void);
    extern bool net_device_register(net_device_t* dev);
    net_framework_init();
    net_device_register(&g_r8168_netdev);

    // Initialize Network Interface Abstraction (`netif`)
    netif_init();
    netif_set_config(
        (192) | (168 << 8) | (2 << 16) | (100U << 24), // 192.168.2.100
        (255) | (255 << 8) | (255 << 16) | (0U << 24), // 255.255.255.0
        (192) | (168 << 8) | (2 << 16) | (1U << 24),   // 192.168.2.1
        (192) | (168 << 8) | (2 << 16) | (1U << 24),   // 192.168.2.1
        (192) | (168 << 8) | (2 << 16) | (1U << 24),   // 192.168.2.1
        3600, 1800, 3150
    );

    // Trigger initial ARP probe to activate TX DMA engine immediately
    uint32_t gw_ip = (192) | (168 << 8) | (2 << 16) | (1U << 24);
    extern bool arp_request(uint32_t target_ip);
    arp_request(gw_ip);

    display_print("[R8168] Realtek Hardware Driver Initialized & READY! (RX+TX Active)\n\n");
}

// r8168_tx_reclaim — Linux-equivalent TX completion/consumer scan
// Walks tx_tail forward past any descriptors hardware has cleared OWN=0 on.
// This is the MISSING piece: without this, tx_head catches tx_tail after one
// full ring lap and blocks forever.
void r8168_tx_reclaim(void) {
    // Clear ISR status flags (write-1-to-clear) so NIC hardware DMA engine advances
    uint16_t isr = r8168_read16(&g_r8168_dev, r8168_intr_status_reg());
    if (isr) {
        r8168_write16(&g_r8168_dev, r8168_intr_status_reg(), isr);
    }

    uint32_t tail = g_r8168_dev.tx_tail;
    uint32_t head = g_r8168_dev.tx_head;

    while (tail != head) {
        struct r8168_tx_desc* desc = &g_r8168_tx_ring[tail];

        // Flush cache line to get hardware's latest write
        __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
        __asm__ __volatile__("mfence" ::: "memory");

        // OWN bit still set = hardware has NOT transmitted this yet, stop scanning
        if (desc->opts1 & (1U << 31)) break;

        // Hardware cleared OWN=0 = descriptor transmitted successfully
        g_tx_ok_count++;
        tail = (tail + 1) % R8168_TX_RING_SIZE;
    }

    g_r8168_dev.tx_tail = tail;
    g_tx_tail_snapshot = tail;
    g_tx_head_snapshot = g_r8168_dev.tx_head;
    g_tx_reclaim_count++;
}

bool r8168_transmit_raw(const void* frame, uint16_t length) {
    g_r8168_xmit_calls++;
    if (!frame || length == 0 || length > 1518) return false;
    if (g_r8168_dev.state < R8168_STATE_READY) return false;

    // First: reclaim any completed TX descriptors
    r8168_tx_reclaim();

    g_tx_try_count++;

    // Check if ring is full (producer == consumer means all slots used)
    uint32_t next_head = (g_r8168_dev.tx_head + 1) % R8168_TX_RING_SIZE;
    if (next_head == g_r8168_dev.tx_tail) {
        // Ring full — hardware has not completed any descriptors yet
        g_tx_drop_count++;
        return false;
    }

    uint32_t idx = g_r8168_dev.tx_head;
    struct r8168_tx_desc* desc = &g_r8168_tx_ring[idx];

    // Copy frame into TX buffer
    memcpy(g_r8168_tx_buffers[idx], frame, length);

    // Flush frame payload cache lines to physical DRAM so Realtek PCIe DMA reads fresh bytes
    for (uint32_t off = 0; off < length; off += 64) {
        __asm__ __volatile__("clflush (%0)" :: "r"(&g_r8168_tx_buffers[idx][off]) : "memory");
    }
    __asm__ __volatile__("mfence" ::: "memory");

    // Build descriptor opts1: OWN=1, FS=1, LS=1, length
    uint32_t opts1 = (1U << 31) | (1U << 29) | (1U << 28) | ((uint32_t)length & 0x3FFF);
    if (idx == R8168_TX_RING_SIZE - 1) {
        opts1 |= (1U << 30); // EOR bit on last descriptor
    }

    // Translate TX buffer virtual address to true physical RAM address for PCIe DMA
    void* pml4 = vmm_get_active_pml4();
    uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_r8168_tx_buffers[idx]);
    if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_r8168_tx_buffers[idx];

    // Write descriptor fields BEFORE opts1 (OWN bit last)
    desc->opts2     = 0;
    desc->addr_low  = (uint32_t)buf_pa;
    desc->addr_high = (uint32_t)(buf_pa >> 32);

    // Ensure addr fields reach DRAM before OWN bit write
    __asm__ __volatile__("mfence" ::: "memory");

    // Assign OWN bit — hands descriptor to hardware
    desc->opts1 = opts1;
    g_tx_desc_used++;

    // Flush descriptor cache line to DRAM immediately (UC mapping should make this instant,
    // but clflush guarantees it reaches PCIe before TX_POLL MMIO write)
    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    // Advance producer index
    g_r8168_dev.tx_head = next_head;
    g_tx_head_snapshot = next_head;

    // Kick TX DMA engine.
    r8168_tx_doorbell();

    return true;
}

uint32_t r8168_get_tx0_opts1(void) {
    __asm__ __volatile__("clflush (%0)" :: "r"(&g_r8168_tx_ring[0]) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");
    return g_r8168_tx_ring[0].opts1;
}

uint8_t r8168_get_chip_cmd(void) {
    return r8168_read8(&g_r8168_dev, R8168_REG_CHIP_CMD);
}

uint16_t r8168_get_isr(void) {
    return r8168_read16(&g_r8168_dev, r8168_intr_status_reg());
}

bool r8168_poll_receive(void) {
    if (g_r8168_dev.state < R8168_STATE_READY) return false;

    // Periodically reclaim completed TX descriptors during RX poll loop
    r8168_tx_reclaim();

    uint32_t idx = g_r8168_dev.rx_head;
    struct r8168_rx_desc* desc = &g_r8168_rx_ring[idx];

    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    // OWN bit 1 = Owned by NIC hardware (no frame received yet)
    // OWN bit 0 = Owned by CPU host driver (frame received by hardware DMA!)
    if (desc->opts1 & (1U << 31)) {
        return false;
    }

    uint16_t pkt_len = (desc->opts1 & 0x3FFF);
    if (pkt_len >= 14 && pkt_len <= 1518) {
        g_rx_frames++;
        uint16_t ethertype = ((uint16_t)g_r8168_rx_buffers[idx][12] << 8) | g_r8168_rx_buffers[idx][13];
        if (ethertype == 0x0806) {
            g_rx_arp_frames++;
        } else if (ethertype == 0x0800) {
            g_rx_ipv4_frames++;
        }

        ethernet_process_frame(g_r8168_rx_buffers[idx], pkt_len);
    } else {
        g_rx_drop_count++;
    }

    // Translate RX buffer virtual address to true physical RAM address for PCIe DMA
    void* pml4 = vmm_get_active_pml4();
    uint64_t buf_pa = vmm_get_physical_address(pml4, (uint64_t)(uintptr_t)g_r8168_rx_buffers[idx]);
    if (!buf_pa) buf_pa = (uint64_t)(uintptr_t)g_r8168_rx_buffers[idx];

    // Re-arm descriptor for next hardware RX DMA
    uint32_t opts1 = (1U << 31) | 1536;
    if (idx == R8168_RX_RING_SIZE - 1) {
        opts1 |= (1U << 30); // EOR bit
    }

    desc->opts2     = 0;
    desc->addr_low  = (uint32_t)buf_pa;
    desc->addr_high = (uint32_t)(buf_pa >> 32);

    __asm__ __volatile__("mfence" ::: "memory");
    desc->opts1 = opts1; // Assign OWN bit last

    __asm__ __volatile__("clflush (%0)" :: "r"(desc) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    g_r8168_dev.rx_head = (idx + 1) % R8168_RX_RING_SIZE;
    return true;
}

void r8168_run_forensics(R8168ForensicReport *rep) {
    if (!rep) return;
    memset(rep, 0, sizeof(R8168ForensicReport));

    PCIDevice *pdev = g_r8168_dev.pci_device;
    void *pml4 = vmm_get_active_pml4();

    // 1. PCI CONFIG SPACE & IDENTIFICATION
    if (pdev) {
        rep->vendor_id           = pdev->vendor_id;
        rep->device_id           = pdev->device_id;
        rep->revision_id         = pci_read_config_8(pdev->bus, pdev->slot, pdev->func, 0x08);
        rep->subsystem_vendor_id = pci_read_config_16(pdev->bus, pdev->slot, pdev->func, 0x2C);
        rep->subsystem_id        = pci_read_config_16(pdev->bus, pdev->slot, pdev->func, 0x2E);

        rep->pci_command        = pci_read_config_16(pdev->bus, pdev->slot, pdev->func, 0x04);
        rep->pci_status         = pci_read_config_16(pdev->bus, pdev->slot, pdev->func, 0x06);
        rep->pci_bar0           = pci_read_config_32(pdev->bus, pdev->slot, pdev->func, 0x10);
        rep->pci_bar1           = pci_read_config_32(pdev->bus, pdev->slot, pdev->func, 0x14);
        rep->pci_interrupt_line = pci_read_config_8(pdev->bus, pdev->slot, pdev->func, 0x3C);

        rep->pci_io_enable     = (rep->pci_command & (1 << 0)) != 0;
        rep->pci_memory_enable = (rep->pci_command & (1 << 1)) != 0;
        rep->pci_bus_master    = (rep->pci_command & (1 << 2)) != 0;
    }

    // 2. REALTEK REGISTERS
    rep->chip_cmd   = r8168_read8(&g_r8168_dev, R8168_REG_CHIP_CMD);
    rep->tx_config  = r8168_read32(&g_r8168_dev, R8168_REG_TX_CONFIG);
    rep->rx_config  = r8168_read32(&g_r8168_dev, R8168_REG_RX_CONFIG);
    rep->cplus_cmd  = r8168_read16(&g_r8168_dev, R8168_REG_CPLUS_CMD);
    rep->tx_poll    = g_r8168_dev.is_rtl8125 ? (uint8_t)r8168_read16(&g_r8168_dev, 0x90)
                                              : r8168_read8(&g_r8168_dev, R8168_REG_TX_POLL);
    rep->imr        = r8168_read16(&g_r8168_dev, r8168_intr_mask_reg());
    rep->isr        = r8168_read16(&g_r8168_dev, r8168_intr_status_reg());

    // 3. DMA BASE REGISTERS
    rep->hw_tx_desc_low  = r8168_read32(&g_r8168_dev, R8168_REG_TX_DESC_LOW);
    rep->hw_tx_desc_high = r8168_read32(&g_r8168_dev, R8168_REG_TX_DESC_HIGH);
    rep->hw_rx_desc_low  = r8168_read32(&g_r8168_dev, R8168_REG_RX_DESC_LOW);
    rep->hw_rx_desc_high = r8168_read32(&g_r8168_dev, R8168_REG_RX_DESC_HIGH);

    // 4. DESCRIPTOR FORENSICS
    __asm__ __volatile__("clflush (%0)" :: "r"(&g_r8168_tx_ring[0]) : "memory");
    __asm__ __volatile__("clflush (%0)" :: "r"(&g_r8168_tx_ring[1]) : "memory");
    __asm__ __volatile__("clflush (%0)" :: "r"(&g_r8168_tx_ring[255]) : "memory");
    __asm__ __volatile__("mfence" ::: "memory");

    rep->desc0_opts1     = g_r8168_tx_ring[0].opts1;
    rep->desc0_opts2     = g_r8168_tx_ring[0].opts2;
    rep->desc0_addr_low  = g_r8168_tx_ring[0].addr_low;
    rep->desc0_addr_high = g_r8168_tx_ring[0].addr_high;

    rep->desc1_opts1     = g_r8168_tx_ring[1].opts1;
    rep->desc1_addr_low  = g_r8168_tx_ring[1].addr_low;

    rep->desc255_opts1    = g_r8168_tx_ring[255].opts1;
    rep->desc255_addr_low = g_r8168_tx_ring[255].addr_low;

    // 5. ADDRESS VERIFICATION
    rep->tx_ring_va = (uint64_t)(uintptr_t)g_r8168_tx_ring;
    rep->tx_ring_pa = vmm_get_physical_address(pml4, rep->tx_ring_va);

    rep->rx_ring_va = (uint64_t)(uintptr_t)g_r8168_rx_ring;
    rep->rx_ring_pa = vmm_get_physical_address(pml4, rep->rx_ring_va);

    rep->tx_buf0_va = (uint64_t)(uintptr_t)g_r8168_tx_buffers[0];
    rep->tx_buf0_pa = vmm_get_physical_address(pml4, rep->tx_buf0_va);

    rep->rx_buf0_va = (uint64_t)(uintptr_t)g_r8168_rx_buffers[0];
    rep->rx_buf0_pa = vmm_get_physical_address(pml4, rep->rx_buf0_va);

    // 6. DMA SANITY CHECKS
    rep->bus_master_ok        = rep->pci_bus_master;
    rep->tx_ring_base_matches = (rep->hw_tx_desc_low == (uint32_t)rep->tx_ring_pa);
    rep->desc0_addr_matches   = (rep->desc0_addr_low == (uint32_t)rep->tx_buf0_pa);
    rep->own_cleared_by_hw    = ((rep->desc0_opts1 & (1U << 31)) == 0);
}
