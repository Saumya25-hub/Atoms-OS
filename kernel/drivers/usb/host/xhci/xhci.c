#include "xhci.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/core/pci/pci.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/usb/core/usb_core.h"
#include "kernel/core/interrupt/include/isr.h"

XHCIDcbaa* g_xhci_dcbaa;
XHCIRing g_xhci_cmd_ring;
XHCIRing g_xhci_event_ring;
XHCIEventRingSegmentTableEntry* g_xhci_erst;
volatile uint32_t* g_xhci_db_regs;
volatile uint32_t* g_xhci_ir_regs;
volatile uint32_t* g_xhci_op_regs = NULL;
PCIDevice* g_xhci_pci_dev = NULL;

volatile xhci_intr_delivery_mode_t g_xhci_intr_delivery_mode = XHCI_INTR_MODE_NONE;
volatile uint64_t g_xhci_interrupts = 0;
volatile uint64_t g_bsme_stall_recoveries = 0;
volatile uint32_t g_xhci_event_processing = 0;

bool xhci_is_processing_events(void) {
    return (g_xhci_event_processing != 0);
}

volatile bool g_xhci_ep0_transfer_complete[256] = {0};
volatile uint32_t g_xhci_ep0_completion_code[256] = {0};
volatile uint32_t g_xhci_ep0_transfer_length[256] = {0};

// Delay helper
static inline uint64_t xhci_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void xhci_delay_ms(uint32_t ms) {
    uint64_t total_cycles = (uint64_t)ms * 3500000ULL;
    uint64_t start = xhci_rdtsc();
    while ((xhci_rdtsc() - start) < total_cycles) {
        __asm__ volatile ("pause");
    }
}

void delay_cycles(uint64_t cycles) {
    for (volatile uint64_t i = 0; i < cycles; i++) {
        __asm__ volatile ("pause");
    }
}

uint32_t g_xhci_context_size = 32;

void xhci_bios_handoff(uint64_t mmio_base, uint32_t hccparams1) {
    uint32_t eecp = (hccparams1 >> 16) & 0xFFFF;
    if (!eecp) return;

    uint32_t cur_offset = eecp << 2;
    int guard = 0;

    while (cur_offset && guard++ < 32) {
        volatile uint32_t* ext_cap = (volatile uint32_t*)(mmio_base + cur_offset);
        uint32_t val = ext_cap[0];
        uint8_t cap_id = val & 0xFF;
        uint8_t next_eecp = (val >> 8) & 0xFF;

        if (cap_id == XHCI_EXT_CAP_LEGSUP) {
            display_print("[XHCI] USBLEGSUP Found: Claiming OS Ownership...\n");
            
            // Set OS Owned Semaphore (Bit 24)
            ext_cap[0] |= XHCI_OS_OWNED_SEMAPHORE;

            // Wait for BIOS Owned Semaphore (Bit 16) to clear with non-blocking timeout
            uint32_t timeout = 50000;
            while ((ext_cap[0] & XHCI_BIOS_OWNED_SEMAPHORE) && timeout--) {
                delay_cycles(100);
            }

            if (ext_cap[0] & XHCI_BIOS_OWNED_SEMAPHORE) {
                display_print("[XHCI] USBLEGSUP: BIOS Handoff Timeout (Forcing Controller Claim)\n");
            } else {
                display_print("[XHCI] USBLEGSUP: OS Ownership Claimed 100% PASS\n");
            }

            // Clear SMI Control/Status bits to prevent SMM SMI traps
            ext_cap[1] &= ~0xE0000000;
            break;
        }

        if (!next_eecp) break;
        cur_offset += (next_eecp << 2);
    }
}

static bool xhci_enable_msix_or_msi(PCIDevice* dev, uint64_t mmio_base) {
    if (!dev) return false;
    uint16_t pci_status = pci_read_config_16(dev->bus, dev->slot, dev->func, 0x06);
    if (!(pci_status & (1 << 4))) { // Capabilities list bit
        display_print("[XHCI INTR] No PCI Capabilities list present\n");
        g_xhci_intr_delivery_mode = XHCI_INTR_MODE_POLLED;
        return false;
    }

    uint8_t cap_ptr = pci_read_config_8(dev->bus, dev->slot, dev->func, 0x34) & ~3;
    uint8_t msix_offset = 0;
    uint8_t msi_offset = 0;

    while (cap_ptr >= 0x40 && cap_ptr <= 0xFC) {
        uint8_t cap_id = pci_read_config_8(dev->bus, dev->slot, dev->func, cap_ptr);
        if (cap_id == 0x11) {
            msix_offset = cap_ptr;
        } else if (cap_id == 0x05) {
            msi_offset = cap_ptr;
        }
        cap_ptr = pci_read_config_8(dev->bus, dev->slot, dev->func, cap_ptr + 1) & ~3;
    }

    // 1. Prefer MSI-X
    if (msix_offset != 0) {
        uint16_t msg_ctrl = pci_read_config_16(dev->bus, dev->slot, dev->func, msix_offset + 2);
        uint32_t table_info = pci_read_config_32(dev->bus, dev->slot, dev->func, msix_offset + 4);
        uint8_t bir = table_info & 0x07;
        uint32_t table_offset = table_info & ~0x07;

        if (bir == 0 && table_offset < (512 * 4096)) {
            volatile uint32_t* msix_table = (volatile uint32_t*)(mmio_base + table_offset);
            // Vector 0 Table Entry (16 bytes): Local APIC Vector 0x40, Fixed, Edge
            msix_table[0] = 0xFEE00000; // Local APIC Base Address (BSP APIC ID 0)
            msix_table[1] = 0x00000000; // Upper Address
            msix_table[2] = 0x00000040; // Vector 0x40
            msix_table[3] = 0x00000000; // Unmask Vector 0 (bit 0 = 0)

            // Enable MSI-X in Message Control
            msg_ctrl |= (1 << 15);  // MSI-X Enable
            msg_ctrl &= ~(1 << 14); // Function Mask clear
            pci_write_config_16(dev->bus, dev->slot, dev->func, msix_offset + 2, msg_ctrl);

            g_xhci_intr_delivery_mode = XHCI_INTR_MODE_MSIX;
            display_print("[XHCI INTR] Hardware MSI-X Enabled (Vector 0x40 -> Local APIC ID 0)\n");
            return true;
        }
    }

    // 2. Fallback to standard MSI
    if (msi_offset != 0) {
        uint16_t msg_ctrl = pci_read_config_16(dev->bus, dev->slot, dev->func, msi_offset + 2);
        bool is_64bit = (msg_ctrl & (1 << 7)) != 0;

        pci_write_config_32(dev->bus, dev->slot, dev->func, msi_offset + 4, 0xFEE00000);
        if (is_64bit) {
            pci_write_config_32(dev->bus, dev->slot, dev->func, msi_offset + 8, 0x00000000);
            pci_write_config_16(dev->bus, dev->slot, dev->func, msi_offset + 12, 0x0040);
        } else {
            pci_write_config_16(dev->bus, dev->slot, dev->func, msi_offset + 8, 0x0040);
        }

        msg_ctrl |= (1 << 0); // MSI Enable
        pci_write_config_16(dev->bus, dev->slot, dev->func, msi_offset + 2, msg_ctrl);

        g_xhci_intr_delivery_mode = XHCI_INTR_MODE_MSI;
        display_print("[XHCI INTR] Hardware MSI Enabled (Vector 0x40 -> Local APIC ID 0)\n");
        return true;
    }

    g_xhci_intr_delivery_mode = XHCI_INTR_MODE_POLLED;
    display_print("[XHCI INTR] Hardware MSI/MSI-X not available; running in Polled Mode\n");
    return false;
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
        extern void usb_forensic_update_controller_dashboard(uint8_t bus, uint8_t slot, uint8_t func, uint32_t usbcmd, uint32_t usbsts, uint32_t dnctrl, uint32_t config, bool running);
        usb_forensic_update_controller_dashboard(0, 0, 0, 0, 0, 0, 0, false);
        return;
    }
    
    display_print("USB_DIAG_1 = xHCI controller detected\n");
    display_print("[XHCI] Controller detected\n");
    g_usb_diag.xhci_started = true;
    extern void usb_forensic_mark_stage(int stage, bool success);
    usb_forensic_mark_stage(1, true); // USB_STAGE_XHCI_CONTROLLER_STARTED
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
    pci_enable_memory_space(xhci_dev);
    pci_enable_bus_mastering(xhci_dev);
    
    // Map MMIO pages (2MB region for capabilities + operational + doorbells + runtime regs)
    extern void* vmm_get_kernel_pml4(void);
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys = (mmio_base + i * 4096) & PAGE_PHYS_ADDRESS_MASK;
        vmm_map_page(pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }

    // Ensure Local APIC register page 0xFEE00000 is mapped for ISR EOI signaling
    vmm_map_page(pml4, 0xFEE00000ULL, 0xFEE00000ULL, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    if (k_pml4 && k_pml4 != pml4) {
        vmm_map_page(k_pml4, 0xFEE00000ULL, 0xFEE00000ULL, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    
    extern bool vmm_is_mapped(void* pml4, uint64_t virt_addr);
    if (!vmm_is_mapped(pml4, mmio_base)) {
        display_print("[XHCI] ERROR: MMIO base is NOT mapped in active PML4!\n");
    } else {
        display_print("[XHCI] MMIO base mapping verified PASS!\n");
    }

    // Capability Registers (Aligned 32-bit reads)
    volatile uint32_t* cap_regs32 = (volatile uint32_t*)mmio_base;
    uint32_t cap_dw0 = cap_regs32[0];
    uint8_t caplength = cap_dw0 & 0xFF;
    uint16_t hciversion = (cap_dw0 >> 16) & 0xFFFF;
    
    uint32_t hcsparams1 = cap_regs32[1];
    uint32_t hcsparams2 = cap_regs32[2];
    uint32_t hcsparams3 = cap_regs32[3];
    uint32_t hccparams1 = cap_regs32[4];
    uint32_t dboff = cap_regs32[5] & ~3; // Ensure doorbell offset is aligned/masked
    uint32_t rtsoff = cap_regs32[6] & ~3;
    
    // Context Size Check
    if (hccparams1 & (1 << 2)) { // CSZ bit
        g_xhci_context_size = 64;
    } else {
        g_xhci_context_size = 32;
    }
    
    g_xhci_db_regs = (volatile uint32_t*)(mmio_base + dboff);
    g_xhci_ir_regs = (volatile uint32_t*)(mmio_base + rtsoff + 0x20); // Interrupter 0 is at RTSOFF + 0x20
    
    display_print("[XHCI] Capability registers valid\n");
    display_print("[XHCI] Version = ");
    display_print_hex(hciversion);
    display_print(" CSZ = ");
    display_print_dec(g_xhci_context_size);
    display_print("\n");

    diag_set_step("XHCI BIOS HANDOFF");
    // Perform BIOS-to-OS Ownership Handoff before Controller Reset
    xhci_bios_handoff(mmio_base, hccparams1);

    uint32_t max_slots = hcsparams1 & 0xFF;
    uint32_t max_ports = (hcsparams1 >> 24) & 0xFF;
    
    display_print("[XHCI] Max Slots = ");
    display_print_dec(max_slots);
    display_print(" Max Ports = ");
    display_print_dec(max_ports);
    display_print("\n");
    
    // Operational Registers
    volatile uint8_t* cap_regs8 = (volatile uint8_t*)mmio_base;
    volatile uint32_t* op_regs = (volatile uint32_t*)(cap_regs8 + caplength);
    volatile uint32_t* usbcmd = op_regs + 0;
    volatile uint32_t* usbsts = op_regs + 1;
    volatile uint32_t* config = op_regs + 14; // CONFIG is at OPBASE + 0x38 (which is 14 * 4)
    g_xhci_op_regs = op_regs;
    g_xhci_pci_dev = xhci_dev;
    
    diag_set_step("XHCI CONTROLLER RESET");
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
    
    // Wait for HCRST to become 0
    wait_count = 0;
    while (*usbcmd & (1 << 1)) {
        delay_cycles(1000);
        wait_count++;
        if (wait_count > 10000) {
            display_print("[XHCI] Host Controller Reset Timeout!\n");
            break;
        }
    }
    
    // Wait for CNR (Controller Not Ready) in USBSTS to become 0
    wait_count = 0;
    while (*usbsts & (1 << 11)) {
        delay_cycles(1000);
        wait_count++;
        if (wait_count > 10000) {
            display_print("[XHCI] Controller Not Ready Timeout!\n");
            break;
        }
    }
    
    display_print("[XHCI] Controller reset complete\n");

    diag_set_step("XHCI PROGRAM DCBAA & RINGS");
    // Program Max Device Slots Enabled in CONFIG register
    *config = max_slots;
    
    // Allocate DCBAA (Device Context Base Address Array)
    uint64_t dcbaa_phys;
    g_xhci_dcbaa = (XHCIDcbaa*)xhci_alloc_dma(sizeof(XHCIDcbaa), &dcbaa_phys, "DCBAA");
    memset(g_xhci_dcbaa, 0, sizeof(XHCIDcbaa));
    
    // Scratchpad Buffer Allocation (if required by controller)
    uint32_t max_scratchpad_buffers = ((hcsparams2 >> 27) & 0x1F) | (((hcsparams2 >> 21) & 0x1F) << 5);
    if (max_scratchpad_buffers > 0) {
        display_print("[XHCI] Allocating ");
        display_print_dec(max_scratchpad_buffers);
        display_print(" Scratchpad Buffers\n");
        
        uint64_t scratch_array_phys;
        uint64_t* scratch_array = (uint64_t*)xhci_alloc_dma(max_scratchpad_buffers * sizeof(uint64_t), &scratch_array_phys, "ScratchArray");
        
        for (uint32_t i = 0; i < max_scratchpad_buffers; i++) {
            uint64_t buf_phys;
            xhci_alloc_dma(4096, &buf_phys, "ScratchBuf");
            scratch_array[i] = buf_phys;
        }
        g_xhci_dcbaa->pointers[0] = scratch_array_phys;
    }

    // Program DCBAAP (Device Context Base Address Array Pointer) in Operational Registers
    volatile uint64_t* dcbaap = (volatile uint64_t*)(op_regs + 12); // OPBASE + 0x30
    *dcbaap = dcbaa_phys;
    
    // Initialize Command Ring (256 TRBs = 4KB = 1 page)
    xhci_ring_init(&g_xhci_cmd_ring, 256);
    volatile uint64_t* crcr = (volatile uint64_t*)(op_regs + 6); // OPBASE + 0x18
    *crcr = g_xhci_cmd_ring.phys_base | 1; // Set Ring Cycle State (RCS) bit to 1

    // Initialize Event Ring (1024 TRBs = 16KB). Clear slot 1023 because Event Rings do not use Link TRBs (xHCI spec 4.9.4)
    xhci_ring_init(&g_xhci_event_ring, 1024);
    memset(&g_xhci_event_ring.trbs[1023], 0, sizeof(XHCITrb));
    
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
    *imod = 1000; // 250us Interrupt Moderation (Caps to 4 kHz to prevent interrupt storm)
    
    // Enable Interrupter (IE)
    *iman |= XHCI_IMAN_IE;

    // Register xHCI ISR at Local APIC Vector 0x40
    isr_register_handler(0x40, (void*)xhci_isr);

    // Enable Hardware MSI-X / MSI Interrupt Routing
    xhci_enable_msix_or_msi(xhci_dev, mmio_base);
    
    diag_set_step("XHCI RUNNING & SCANNING PORTS");
    // Start controller with Run/Stop (RS) and Interrupter Enable (INTE)
    *usbcmd |= (XHCI_CMD_RS | XHCI_CMD_INTE);
    
    display_print("[XHCI] Controller ready and running (INTE=1)\n");
    
    // Phase 2A: Root Port Power & Physical Debounce
    volatile uint32_t* portsc_base = op_regs + 256; // 0x400 / 4

    // 1. Ensure Port Power (PP, bit 9) is active across all root hub ports
    for (uint32_t p = 1; p <= max_ports; p++) {
        volatile uint32_t* portsc = portsc_base + (p - 1) * 4;
        uint32_t val = *portsc;
        if ((val & (1 << 9)) == 0) {
            *portsc = (val & 0x0E00C3E0) | (1 << 9);
        }
    }

    // 2. Bare-metal VBUS Power & Debounce Stabilization Delay (200ms)
    // Physical flash drives take 100-300ms to stabilize VBUS and pull D+/D- lines
    display_print("[XHCI] Waiting 200ms for root hub VBUS & physical USB device debounce...\n");
    xhci_delay_ms(200);

    // 3. Multi-Pass Root Port Detection
    bool port_handled[256] = {false};
    for (int pass = 1; pass <= 3; pass++) {
        for (uint32_t p = 1; p <= max_ports; p++) {
            if (port_handled[p]) continue;
            volatile uint32_t* portsc = portsc_base + (p - 1) * 4;
            uint32_t val = *portsc;
            
            // Check CCS (Current Connect Status, bit 0)
            if (val & 1) {
                port_handled[p] = true;
                display_print("[XHCI PORT] Physical device attached on Port=");
                display_print_dec(p);
                display_print(" (Pass ");
                display_print_dec(pass);
                display_print(")\n");
                
                diag_set_step("XHCI PORT RESET");
                // If port is not already enabled (PED bit 1 == 0), issue Port Reset (PR bit 4)
                if ((*portsc & (1 << 1)) == 0) {
                    *portsc = (*portsc & 0x0E00C3E0) | (1 << 4);
                    
                    // Wait up to 100ms for Port Enabled (PED bit 1) or Port Reset Change (PRC bit 21)
                    uint32_t reset_timeout_ms = 0;
                    while (((*portsc & (1 << 21)) == 0) && ((*portsc & (1 << 1)) == 0)) {
                        xhci_delay_ms(2);
                        reset_timeout_ms += 2;
                        if (reset_timeout_ms >= 100) break;
                    }
                }
                
                // Clear PRC if set
                if (*portsc & (1 << 21)) {
                    *portsc = (*portsc & 0x0E00C3E0) | (1 << 21);
                }
                
                // Give physical device 25ms Reset Recovery time (USB 2.0 Spec 9.2.6.2 TRSTRCY)
                xhci_delay_ms(25);
                
                if (*portsc & (1 << 1)) {
                    // CRITICAL SPEC FIX: Port Speed MUST be sampled AFTER reset and enabled!
                    uint32_t final_portsc = *portsc;
                    uint32_t speed = (final_portsc >> 10) & 0x0F;
                    if (speed == 0) {
                        // Fallback: If PHY reports 0 but port is enabled, default to High-Speed (3)
                        speed = 3;
                    }
                    display_print("[XHCI PORT] Port Enabled & Ready. Final Speed=");
                    display_print_dec(speed);
                    display_print("\n");
                    
                    diag_set_step("USB DEVICE ENUMERATION");
                    extern void usb_device_connected(uint8_t port, uint8_t speed);
                    usb_device_connected(p, speed);
                } else {
                    display_print("[XHCI PORT] Port Reset Timeout / Not Enabled\n");
                }
            }
        }
        if (pass < 3) {
            xhci_delay_ms(100); // 100ms between retry passes
        }
    }
}

// Global variables to track command completion
volatile uint32_t g_xhci_last_cmd_completion_code = 0;
volatile uint32_t g_xhci_last_cmd_slot_id = 0;
volatile bool g_xhci_cmd_complete = false;

// Global array for transfer event completion tracking
volatile bool g_xhci_transfer_complete[256]; // indexed by slot ID
volatile uint32_t g_xhci_transfer_length[256];

void xhci_process_events(void) {
    if (!g_xhci_ir_regs) return;

    // Single-threaded atomic lock guarding against reentrant event ring consumption
    if (__atomic_test_and_set(&g_xhci_event_processing, __ATOMIC_ACQUIRE)) {
        return;
    }

    volatile uint64_t* erdp = (volatile uint64_t*)(g_xhci_ir_regs + 6);
    XHCIRing* ring = &g_xhci_event_ring;
    
    while (true) {
        XHCITrb* trb = &ring->trbs[ring->dequeue];
        uint32_t cycle = trb->control & 1;
        
        if (cycle != ring->cycle) {
            break; // No more events
        }

        // 1. Copy event TRB to local stack structure before advancing
        XHCITrb event = *trb;

        // 2. Advance dequeue pointer and update cycle state IMMEDIATELY
        ring->dequeue++;
        if (ring->dequeue == ring->size) { // Event rings do NOT have Link TRBs, they wrap exactly at size
            ring->dequeue = 0;
            ring->cycle ^= 1;
        }

        // 3. Immediately commit ERDP acknowledgment to xHCI hardware controller with EHB=1
        uint64_t new_erdp = (ring->phys_base + (ring->dequeue * sizeof(XHCITrb))) & ~0x0FUL;
        *erdp = new_erdp | XHCI_ERDP_EHB;

        extern volatile uint64_t g_xhci_events;
        g_xhci_events++;

        // 4. Process event from local copy
        uint32_t type = (event.control >> 10) & 0x3F;
        uint32_t completion_code = (event.status >> 24) & 0xFF;
        
        if (type == TRB_COMMAND_COMPLETION_EVENT) {
            g_xhci_last_cmd_completion_code = completion_code;
            g_xhci_last_cmd_slot_id = (event.control >> 24) & 0xFF;
            g_xhci_cmd_complete = true;
        } else if (type == TRB_PORT_STATUS_CHANGE_EVENT) {
            // Optional: Port Status Change
        } else if (type == TRB_TRANSFER_EVENT) {
            uint32_t slot_id = (event.control >> 24) & 0xFF;
            uint32_t transfer_length = event.status & 0xFFFFFF;
            uint32_t endpoint_id = (event.control >> 16) & 0x1F;
            
            extern volatile uint32_t g_cfg_last_completion_code;
            extern volatile uint32_t g_cfg_last_transfer_length;
            extern volatile uint32_t g_cfg_last_slot_id;
            extern volatile uint32_t g_cfg_last_ep_id;
            extern volatile uint32_t g_cfg_last_trb_type;
            extern volatile bool g_cfg_event_arrived;

            g_cfg_event_arrived = true;
            g_cfg_last_completion_code = completion_code;
            g_cfg_last_transfer_length = transfer_length;
            g_cfg_last_slot_id = slot_id;
            g_cfg_last_ep_id = endpoint_id;
            g_xhci_transfer_length[slot_id] = transfer_length;
            g_xhci_transfer_complete[slot_id] = true;

            if (endpoint_id == 1) {
                // Dedicated EP0 Control Transfer completion
                g_xhci_ep0_completion_code[slot_id] = completion_code;
                g_xhci_ep0_transfer_length[slot_id] = transfer_length;
                g_xhci_ep0_transfer_complete[slot_id] = true;
            } else {
                extern void xhci_handle_transfer_event(uint32_t slot_id, uint32_t completion_code, uint32_t transfer_length, XHCITrb* trb);
                xhci_handle_transfer_event(slot_id, completion_code, transfer_length, &event);
            }
        } else {
            extern void display_print(const char*);
            extern void display_print_dec(uint64_t);
            extern void display_print_hex(uint64_t);
            display_print("[XHCI EVENT DEBUG]\n");
            display_print("Type="); display_print_dec(type);
            display_print(" CompletionCode="); display_print_dec(completion_code);
            display_print(" TRBPointer="); display_print_hex(((uint64_t)event.param2 << 32) | event.param1);
            display_print(" Cycle="); display_print_dec(cycle); display_print("\n");
        }
    }

    __atomic_clear(&g_xhci_event_processing, __ATOMIC_RELEASE);
}

void xhci_poll(void) {
    xhci_process_events();
    extern void usb_hid_ale_step(void);
    usb_hid_ale_step();
}

uint64_t xhci_isr(registers_t* regs) {
    (void)regs;
    if (!g_xhci_op_regs || !g_xhci_ir_regs) {
        *(volatile uint32_t*)0xFEE000B0ULL = 0;
        return 0;
    }

    volatile uint32_t* usbsts = g_xhci_op_regs + 1; // Operational base + 0x04
    uint32_t sts_val = *usbsts;
    if (!(sts_val & XHCI_STS_EINT)) {
        // Not this controller's event (shared IRQ line)
        *(volatile uint32_t*)0xFEE000B0ULL = 0;
        return 0;
    }

    // Acknowledge controller interrupt flags (RW1C)
    *usbsts = XHCI_STS_EINT;
    volatile uint32_t* iman = g_xhci_ir_regs + 0;
    *iman |= XHCI_IMAN_IP;

    g_xhci_interrupts++;

    // Drain event ring
    xhci_process_events();

    // Signal Local APIC EOI
    *(volatile uint32_t*)0xFEE000B0ULL = 0;
    return 0;
}
