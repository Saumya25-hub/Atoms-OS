#include "rtl8125_minimal.h"
#include <kernel/debug/lan_debug/lan_debug.h>
#include <kernel/core/memory/pmm/include/pmm.h>
#include <kernel/core/memory/vmm/include/vmm.h>
#include <kernel/core/pci/pci.h>
#include <kernel/core/timer/include/timer.h> // For sleep/delay if needed

// ----------------------------------------------------------------------------
// MMIO Access Helpers
// ----------------------------------------------------------------------------
static inline uint8_t rtl_read8(uint64_t mmio_base, uint32_t reg) {
    return *(volatile uint8_t*)(mmio_base + reg);
}

static inline uint16_t rtl_read16(uint64_t mmio_base, uint32_t reg) {
    return *(volatile uint16_t*)(mmio_base + reg);
}

static inline uint32_t rtl_read32(uint64_t mmio_base, uint32_t reg) {
    return *(volatile uint32_t*)(mmio_base + reg);
}

static inline void rtl_write8(uint64_t mmio_base, uint32_t reg, uint8_t val) {
    *(volatile uint8_t*)(mmio_base + reg) = val;
}

static inline void rtl_write16(uint64_t mmio_base, uint32_t reg, uint16_t val) {
    *(volatile uint16_t*)(mmio_base + reg) = val;
}

static inline void rtl_write32(uint64_t mmio_base, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(mmio_base + reg) = val;
}

// Ensure memory visibility to the hardware Root Complex
static inline void flush_cache_line(volatile void* addr) {
    __asm__ volatile ("clflush (%0)" :: "r"(addr) : "memory");
}

static inline void mfence() {
    __asm__ volatile ("mfence" ::: "memory");
}

// ----------------------------------------------------------------------------
// Bare-Metal Minimal DMA Bring-Up
// ----------------------------------------------------------------------------
void rtl8125_minimal_test(PCIDevice* dev) {
    kprintf("\n=======================================================\n");
    kprintf(" [RTL8125 MINIMAL] BARE-METAL TX DMA FORENSIC TEST\n");
    kprintf("=======================================================\n");

    // 1. PCI Init (Ensure Bus Master & MMIO)
    uint16_t pci_cmd = pci_read_config_16(dev->bus, dev->slot, dev->func, 0x04);
    pci_cmd |= 0x07; // IO | MEM | BUS_MASTER
    pci_write_config_16(dev->bus, dev->slot, dev->func, 0x04, pci_cmd);
    kprintf("[RTL8125] PCI Command set to: 0x%x (Bus Master ENABLED)\n", pci_cmd);

    // 2. Memory Map (Read BAR2 typically for MMIO)
    // Actually, let's just use the known good MMIO base from the PCI device struct if available,
    // or read it directly from BAR2 config space (0x18 offset is BAR2, 0x10 is BAR0).
    // Let's use dev->bars[2].base_address.
    uint64_t mmio_base = dev->bars[2].base_address;
    // Map MMIO in VMM (Assuming identity mapped in this OS design for device space)
    kprintf("[RTL8125] MMIO Base: 0x%llx\n", mmio_base);

    // 3. Hardware Reset
    kprintf("[RTL8125] Asserting Hardware Reset...\n");
    rtl_write8(mmio_base, RTL8125_REG_CHIP_CMD, RTL8125_CMD_RESET);
    for (int i = 0; i < 1000; i++) {
        if ((rtl_read8(mmio_base, RTL8125_REG_CHIP_CMD) & RTL8125_CMD_RESET) == 0) {
            break;
        }
    }
    kprintf("[RTL8125] Hardware Reset COMPLETE.\n");

    // 4. MAC Unlock
    rtl_write8(mmio_base, RTL8125_REG_CFG9346, RTL8125_CFG9346_UNLOCK);

    // 5. Descriptor Ring Allocation (Allocate 1 physical page)
    uint64_t ring_pa = (uint64_t)pmm_alloc_page();
    volatile rtl8125_min_desc_t* tx_ring = (volatile rtl8125_min_desc_t*)ring_pa;
    
    // Zero out the ring
    for (int i = 0; i < 256; i++) {
        ((volatile uint8_t*)tx_ring)[i] = 0;
    }
    kprintf("[RTL8125] TX Ring Physical Address: 0x%llx\n", ring_pa);

    // 6. Base PA Config
    rtl_write32(mmio_base, RTL8125_REG_TX_DESC_LOW, (uint32_t)(ring_pa & 0xFFFFFFFF));
    rtl_write32(mmio_base, RTL8125_REG_TX_DESC_HIGH, (uint32_t)(ring_pa >> 32));
    
    uint32_t readback_low = rtl_read32(mmio_base, RTL8125_REG_TX_DESC_LOW);
    kprintf("[RTL8125] RING_PA / 0x20 MATCH: %s (Reg=0x%x)\n", (readback_low == ring_pa) ? "YES" : "NO", readback_low);

    // 7. Ring Size & DMA Config (Linux default)
    rtl_write32(mmio_base, RTL8125_REG_TX_CONFIG, RTL8125_TX_CONFIG_DEFAULT);

    // 8. Descriptor Payload setup
    uint64_t packet_pa = (uint64_t)pmm_alloc_page();
    volatile uint8_t* packet = (volatile uint8_t*)packet_pa;
    // Fill dummy ethernet packet (68 bytes: Broadcast ARP or arbitrary payload)
    for (int i = 0; i < 68; i++) packet[i] = 0xFF;
    
    // Populate Descriptor 0
    tx_ring[0].addr = packet_pa;
    tx_ring[0].opts2 = 0;
    // OWN=1, FS=1, LS=1, EOR=0, Len=68
    tx_ring[0].opts1 = RTL8125_DESC_OWN | RTL8125_DESC_FS | RTL8125_DESC_LS | 68;
    
    kprintf("[RTL8125] OPTS0 BEFORE KICK: 0x%08x\n", tx_ring[0].opts1);

    // 9. Cache Flush & Memory Fence (CRITICAL for Intel 14th Gen)
    flush_cache_line((void*)&tx_ring[0]);
    flush_cache_line((void*)packet);
    mfence();

    // 10. Engine Enable (Turn on TX and RX)
    rtl_write8(mmio_base, RTL8125_REG_CHIP_CMD, RTL8125_CMD_TX_EN | RTL8125_CMD_RX_EN);

    // MAC Lock
    rtl_write8(mmio_base, RTL8125_REG_CFG9346, RTL8125_CFG9346_LOCK);

    // 11. TxNoClose Doorbell (The Crux for RTL8125 Rev 05)
    uint16_t tail_index = 1; // We placed 1 packet at index 0, so tail advances to 1
    kprintf("[RTL8125] DOORBELL KICK (Reg: 0x2800, Val: %d)\n", tail_index);
    rtl_write16(mmio_base, RTL8125_SW_TAIL_PTR0, tail_index);
    
    // Additional mfence just in case
    mfence();

    // 12. OWN Bit Polling (Verification)
    kprintf("[RTL8125] Waiting for OWN bit to clear...\n");
    int timeout = 100000;
    while (timeout > 0) {
        if ((tx_ring[0].opts1 & RTL8125_DESC_OWN) == 0) {
            break;
        }
        timeout--;
    }

    kprintf("[RTL8125] OPTS0 AFTER KICK: 0x%08x\n", tx_ring[0].opts1);
    
    if ((tx_ring[0].opts1 & RTL8125_DESC_OWN) == 0) {
        kprintf("[RTL8125] CERTIFICATION SUCCESS: OWN BIT CLEARED!\n");
        kprintf("[RTL8125] Hardware Transmit DMA is Fully Operational.\n");
    } else {
        kprintf("[RTL8125] CERTIFICATION FAILED: OWN BIT STUCK.\n");
    }

    kprintf("=======================================================\n");

    // Halt network operations so the legacy driver doesn't trample this state
    kprintf("[RTL8125] Halting further PCI processing to freeze telemetry.\n");
    while(1) { __asm__ volatile("hlt"); }
}
