#include "xhci.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/core/pci/pci.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/usb/core/usb_core.h"

XHCIDcbaa* g_xhci_dcbaa;
XHCIRing g_xhci_cmd_ring;
XHCIRing g_xhci_event_ring;
XHCIEventRingSegmentTableEntry* g_xhci_erst;
volatile uint32_t* g_xhci_db_regs;
volatile uint32_t* g_xhci_ir_regs;

// Delay helper
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
    void* pml4 = vmm_get_active_pml4();
    for (uint64_t i = 0; i < 512; i++) {
        uint64_t phys = (mmio_base + i * 4096) & PAGE_PHYS_ADDRESS_MASK;
        vmm_map_page(pml4, phys, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
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
    
    diag_set_step("XHCI DMA & RINGS ALLOCATION");
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
    
    diag_set_step("XHCI RUNNING & SCANNING PORTS");
    // Start controller
    *usbcmd |= 1; // Set Run/Stop
    
    display_print("[XHCI] Controller ready and running\n");
    
    // Phase 2A: Port Detection
    volatile uint32_t* portsc_base = op_regs + 256; // 0x400 / 4
    
    for (uint32_t p = 1; p <= max_ports; p++) {
        volatile uint32_t* portsc = portsc_base + (p - 1) * 4;
        uint32_t val = *portsc;
        
        // Check CCS (Current Connect Status, bit 0)
        if (val & 1) {
            uint32_t speed = (val >> 10) & 0x0F;
            display_print("[XHCI PORT] Device connected: Port=");
            display_print_dec(p);
            display_print(" Speed=");
            display_print_dec(speed);
            display_print("\n");
            
            diag_set_step("XHCI PORT RESETTING");
            // Reset port (Set PR bit 4)
            *portsc = (*portsc & 0x0E00C3E0) | (1 << 4);
            
            // Wait for PRC (Port Reset Change, bit 21) or PED (Port Enabled, bit 1)
            uint32_t pt_wait = 0;
            while (((*portsc & (1 << 21)) == 0) && ((*portsc & (1 << 1)) == 0)) {
                delay_cycles(1000);
                pt_wait++;
                if (pt_wait > 50000) break;
            }
            
            if (*portsc & (1 << 1)) {
                // Clear PRC if set
                if (*portsc & (1 << 21)) {
                    *portsc = (*portsc & 0x0E00C3E0) | (1 << 21);
                }
                display_print("[XHCI PORT] Port Reset Complete. Port Enabled.\n");
                
                diag_set_step("USB DEVICE ENUMERATION");
                extern void usb_device_connected(uint8_t port, uint8_t speed);
                usb_device_connected(p, speed);
            } else {
                display_print("[XHCI PORT] Port Reset Timeout\n");
            }
        }
    }
}

// Global variables to track command completion
volatile uint32_t g_xhci_last_cmd_completion_code = 0;
volatile uint32_t g_xhci_last_cmd_slot_id = 0;
volatile bool g_xhci_cmd_complete = false;

// Global array for transfer event completion tracking (simple hack for now)
volatile bool g_xhci_transfer_complete[256]; // indexed by slot ID
volatile uint32_t g_xhci_transfer_length[256];

void xhci_poll(void) {
    if (!g_xhci_ir_regs) return;
    
    // Prevent reentrancy if called concurrently from GUI thread and IRQ0 Timer
    static volatile uint32_t s_xhci_poll_lock = 0;
    if (__sync_lock_test_and_set(&s_xhci_poll_lock, 1)) {
        return;
    }

    // Process all events in the ring
    uint32_t events_processed = 0;
    volatile uint64_t* erdp = (volatile uint64_t*)(g_xhci_ir_regs + 6);
    XHCIRing* ring = &g_xhci_event_ring;
    
    while (true) {
        XHCITrb* trb = &ring->trbs[ring->dequeue];
        uint32_t cycle = trb->control & 1;
        
        if (cycle != ring->cycle) {
            break; // No more events
        }
        
        uint32_t type = (trb->control >> 10) & 0x3F;
        uint32_t completion_code = (trb->status >> 24) & 0xFF;
        
        if (type == TRB_COMMAND_COMPLETION_EVENT) {
            g_xhci_last_cmd_completion_code = completion_code;
            g_xhci_last_cmd_slot_id = (trb->control >> 24) & 0xFF;
            g_xhci_cmd_complete = true;
        } else if (type == TRB_PORT_STATUS_CHANGE_EVENT) {
            // Optional: Port Status Change
        } else if (type == TRB_TRANSFER_EVENT) {
            uint32_t slot_id = (trb->control >> 24) & 0xFF;
            uint32_t endpoint_id = (trb->control >> 16) & 0x1F;
            uint32_t transfer_length = trb->status & 0xFFFFFF;
            
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
            g_cfg_last_trb_type = type;

            extern void display_print(const char*);
            extern void display_print_dec(uint64_t);
            extern void display_print_hex(uint64_t);
            
            display_print("[XHCI EVENT] XFER Slot=");
            display_print_dec(slot_id);
            display_print(" EP=");
            display_print_dec(endpoint_id);
            display_print(" Code=");
            display_print_dec(completion_code);
            display_print("\n");
            
            g_xhci_transfer_length[slot_id] = transfer_length;
            g_xhci_transfer_complete[slot_id] = true;
            
            extern void xhci_handle_transfer_event(uint32_t slot_id, uint32_t completion_code, uint32_t transfer_length, XHCITrb* trb);
            xhci_handle_transfer_event(slot_id, completion_code, transfer_length, trb);
        } else {
            extern void display_print(const char*);
            extern void display_print_dec(uint64_t);
            extern void display_print_hex(uint64_t);
            display_print("[XHCI EVENT DEBUG]\n");
            display_print("Type="); display_print_dec(type);
            display_print(" CompletionCode="); display_print_dec(completion_code);
            display_print(" TRBPointer="); display_print_hex(((uint64_t)trb->param2 << 32) | trb->param1);
            display_print(" Cycle="); display_print_dec(cycle); display_print("\n");
        }
        
        ring->dequeue++;
        if (ring->dequeue == ring->size) { // Event rings do NOT have Link TRBs, they wrap exactly at size
            ring->dequeue = 0;
            ring->cycle ^= 1;
        }
        
        events_processed++;
    }

    if (events_processed > 0) {
        extern volatile uint64_t g_xhci_events;
        g_xhci_events += events_processed;
        // Update ERDP (Clear EHB bit 3, preserve 16-byte alignment)
        uint64_t new_erdp = (ring->phys_base + (ring->dequeue * sizeof(XHCITrb))) & ~0x0FUL;
        *erdp = new_erdp | (1 << 3); 
    }

    __sync_lock_release(&s_xhci_poll_lock);
}
