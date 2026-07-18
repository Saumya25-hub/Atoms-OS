#include "xhci.h"
#include "kernel/core/pci/pci.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"

XHCIDcbaa* g_xhci_dcbaa;
XHCIRing g_xhci_cmd_ring;
XHCIRing g_xhci_event_ring;
XHCIEventRingSegmentTableEntry* g_xhci_erst;
volatile uint32_t* g_xhci_db_regs;
volatile uint32_t* g_xhci_ir_regs;

// Delay helper
static void delay_cycles(uint64_t cycles) {
    for (volatile uint64_t i = 0; i < cycles; i++) {
        __asm__ volatile ("pause");
    }
}

void xhci_init(void) {
    display_print("[XHCI] Starting initialization...\n");
    
    // Find xHCI controller
    uint32_t count = pci_get_device_count();
    PCIDevice* xhci_dev = NULL;
    for (uint32_t i = 0; i < count; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (dev->base_class == 0x0C && dev->sub_class == 0x03) {
            uint32_t class_info = pci_read_config(dev->bus, dev->slot, dev->func, 0x08);
            uint8_t prog_if = (class_info >> 8) & 0xFF;
            if (prog_if == XHCI_PCI_PROGIF) {
                xhci_dev = dev;
                break;
            }
        }
    }
    
    if (!xhci_dev) {
        display_print("[XHCI] No xHCI controller found on PCI bus.\n");
        return;
    }
    
    display_print("[XHCI] Controller detected\n");
    display_print("[XHCI] PCI ");
    display_print_dec(xhci_dev->bus);
    display_print(":");
    display_print_dec(xhci_dev->slot);
    display_print("\n");
    
    // Read BAR0 (offset 0x10)
    uint32_t bar0 = pci_read_config(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x10);
    uint64_t mmio_base = bar0 & 0xFFFFFFF0; // Mask out type/prefetch bits
    
    // If it's a 64-bit BAR
    if ((bar0 & 0x6) == 0x4) {
        uint32_t bar1 = pci_read_config(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, 0x14);
        mmio_base |= ((uint64_t)bar1 << 32);
    }
    
    display_print("[XHCI] MMIO base = ");
    display_print_hex(mmio_base);
    display_print("\n");
    
    // Enable Bus Master and Memory Space
    uint32_t cmd = pci_read_config(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, PCI_COMMAND_OFFSET);
    pci_write_config_16(xhci_dev->bus, xhci_dev->slot, xhci_dev->func, PCI_COMMAND_OFFSET, (uint16_t)(cmd | PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY));
    
    // Map MMIO pages (assume 64KB region is enough for capabilities + operational regs)
    void* pml4 = vmm_get_active_pml4();
    for (uint64_t i = 0; i < 16; i++) {
        uint64_t phys = (mmio_base + i * 4096) & PAGE_PHYS_ADDRESS_MASK;
        vmm_map_page(pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    
    // Capability Registers
    volatile uint8_t* cap_regs = (volatile uint8_t*)mmio_base;
    uint8_t caplength = cap_regs[0];
    uint16_t hciversion = *(volatile uint16_t*)(cap_regs + 2);
    uint32_t hcsparams1 = *(volatile uint32_t*)(cap_regs + 4);
    uint32_t hcsparams2 = *(volatile uint32_t*)(cap_regs + 8);
    // uint32_t hcsparams3 = *(volatile uint32_t*)(cap_regs + 12);
    // uint32_t hccparams1 = *(volatile uint32_t*)(cap_regs + 16);
    uint32_t dboff = *(volatile uint32_t*)(cap_regs + 20);
    uint32_t rtsoff = *(volatile uint32_t*)(cap_regs + 24);
    
    g_xhci_db_regs = (volatile uint32_t*)(mmio_base + dboff);
    g_xhci_ir_regs = (volatile uint32_t*)(mmio_base + rtsoff + 0x20); // Interrupter 0 is at RTSOFF + 0x20
    
    display_print("[XHCI] Capability registers valid\n");
    display_print("[XHCI] Version = ");
    display_print_hex(hciversion);
    display_print("\n");
    
    uint32_t max_slots = hcsparams1 & 0xFF;
    uint32_t max_ports = (hcsparams1 >> 24) & 0xFF;
    
    display_print("[XHCI] Max Slots = ");
    display_print_dec(max_slots);
    display_print("\n");
    display_print("[XHCI] Max Ports = ");
    display_print_dec(max_ports);
    display_print("\n");
    
    // Operational Registers
    volatile uint32_t* op_regs = (volatile uint32_t*)(cap_regs + caplength);
    volatile uint32_t* usbcmd = op_regs + 0;
    volatile uint32_t* usbsts = op_regs + 1;
    volatile uint32_t* config = op_regs + 14; // CONFIG is at OPBASE + 0x38 (which is 14 * 4)
    
    // Stop controller if running
    *usbcmd &= ~1; // Clear Run/Stop
    
    // Wait for HCHalted (bit 0 in USBSTS)
    uint32_t wait_count = 0;
    while (!(*usbsts & 1)) {
        delay_cycles(1000);
        wait_count++;
        if (wait_count > 10000) break; // Timeout
    }
    
    // Perform Host Controller Reset
    *usbcmd |= (1 << 1); // Set HCRST
    
    // Wait for HCRST to clear
    wait_count = 0;
    while (*usbcmd & (1 << 1)) {
        delay_cycles(1000);
        wait_count++;
        if (wait_count > 10000) break; // Timeout
    }
    
    // Wait for Controller Not Ready (CNR, bit 11 in USBSTS) to clear
    wait_count = 0;
    while (*usbsts & (1 << 11)) {
        delay_cycles(1000);
        wait_count++;
        if (wait_count > 10000) break; // Timeout
    }
    
    display_print("[XHCI] Controller reset successful\n");
    
    // Set MaxSlotsEn in CONFIG register (bits 0-7)
    *config = (*config & ~0xFF) | max_slots;
    
    // Allocate DCBAA
    uint64_t dcbaa_phys;
    g_xhci_dcbaa = (XHCIDcbaa*)xhci_alloc_dma(sizeof(XHCIDcbaa), &dcbaa_phys, "DCBAA");
    
    // Setup Scratchpad Buffers if required
    uint32_t max_scratchpad = (hcsparams2 >> 21) & 0x1F;
    max_scratchpad |= ((hcsparams2 >> 27) & 0x1F) << 5;
    if (max_scratchpad > 0) {
        uint64_t scratch_array_phys;
        uint64_t* scratch_array = (uint64_t*)xhci_alloc_dma(max_scratchpad * sizeof(uint64_t), &scratch_array_phys, "ScratchArray");
        for (uint32_t s = 0; s < max_scratchpad; s++) {
            uint64_t sp_phys;
            xhci_alloc_dma(4096, &sp_phys, "ScratchPage");
            scratch_array[s] = sp_phys;
        }
        g_xhci_dcbaa->pointers[0] = scratch_array_phys;
    }
    
    // Write DCBAAP
    volatile uint64_t* dcbaap = (volatile uint64_t*)(op_regs + 12); // OPBASE + 0x30
    *dcbaap = dcbaa_phys;

    // Initialize Command Ring
    xhci_ring_init(&g_xhci_cmd_ring, 256); // 256 TRBs = 4096 bytes (1 page)
    volatile uint64_t* crcr = (volatile uint64_t*)(op_regs + 6); // OPBASE + 0x18
    *crcr = g_xhci_cmd_ring.phys_base | 1; // Set Ring Cycle State (RCS) bit to 1

    // Initialize Event Ring
    xhci_ring_init(&g_xhci_event_ring, 256);
    
    // Initialize ERST (Event Ring Segment Table)
    uint64_t erst_phys;
    g_xhci_erst = (XHCIEventRingSegmentTableEntry*)xhci_alloc_dma(sizeof(XHCIEventRingSegmentTableEntry), &erst_phys, "ERST");
    g_xhci_erst[0].ring_segment_base_address = g_xhci_event_ring.phys_base;
    g_xhci_erst[0].ring_segment_size = g_xhci_event_ring.size;
    g_xhci_erst[0].reserved1 = 0;
    g_xhci_erst[0].reserved2 = 0;

    // Interrupter 0 Initialization
    volatile uint32_t* iman = g_xhci_ir_regs + 0;
    volatile uint32_t* imod = g_xhci_ir_regs + 1;
    volatile uint32_t* erstsz = g_xhci_ir_regs + 2;
    volatile uint64_t* erstba = (volatile uint64_t*)(g_xhci_ir_regs + 4);
    volatile uint64_t* erdp = (volatile uint64_t*)(g_xhci_ir_regs + 6);

    *erstsz = 1; // 1 segment
    *erstba = erst_phys;
    *erdp = g_xhci_event_ring.phys_base;
    
    // Enable Interrupter (IE)
    *iman |= 2;
    
    // Start controller
    *usbcmd |= 1; // Set Run/Stop
    
    display_print("[XHCI] Controller ready and running\n");
}
