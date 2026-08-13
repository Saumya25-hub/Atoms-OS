#include "kernel/drivers/usb/core/usb_core.h"
#include "xhci.h"

extern void display_print(const char*);
extern void display_print_dec(uint64_t);
extern void display_print_hex(uint64_t);

extern XHCIRing g_xhci_ep0_ring[256];
extern volatile uint32_t* g_xhci_db_regs;
extern volatile bool g_xhci_transfer_complete[256];
extern volatile uint32_t g_xhci_transfer_length[256];

extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);
extern void* memcpy(void*, const void*, size_t);
extern void* memset(void*, int, size_t);

static void* g_ctrl_dma_buf = NULL;
static uint64_t g_ctrl_dma_phys = 0;

#include "kernel/drivers/usb/core/usb_forensic_phase3.h"

bool xhci_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data) {
    uint32_t slot_id = dev->slot_id;
    XHCIRing* ring = &g_xhci_ep0_ring[slot_id];

    /* Safety: if ring is not initialized yet, abort to prevent Divide-by-Zero */
    if (!ring->trbs || ring->size == 0) {
        display_print("[XHCI XFER] EP0 ring not initialized for slot=");
        display_print_dec(slot_id);
        display_print("\n");
        return false;
    }
    
    g_xhci_transfer_complete[slot_id] = false;
    
    uint32_t trt = 0; // No Data
    if (length > 0) {
        if (request_type & 0x80) trt = 3; // IN
        else trt = 2; // OUT
    }

    // Populate Phase 3 Diagnostic Setup Bytes
    g_usb_phase3.bmRequestType = request_type;
    g_usb_phase3.bRequest = request;
    g_usb_phase3.wValue = value;
    g_usb_phase3.wIndex = index;
    g_usb_phase3.wLength = length;

    g_usb_phase3.ep0_enq_before = ring->enqueue;
    g_usb_phase3.ep0_cyc_before = ring->cycle;
    extern XHCIRing g_xhci_event_ring;
    g_usb_phase3.evt_deq = g_xhci_event_ring.dequeue;
    g_usb_phase3.evt_cyc = g_xhci_event_ring.cycle;

    g_usb_phase3.setup_trb_sent = false;
    g_usb_phase3.data_trb_sent = false;
    g_usb_phase3.status_trb_sent = false;
    g_usb_phase3.doorbell_rung = false;
    g_usb_phase3.transfer_event_received = false;
    g_usb_phase3.timeout_occurred = false;
    g_usb_phase3.user_buf_virt = (uint64_t)data;
    memset(g_usb_phase3.dma_dump, 0, sizeof(g_usb_phase3.dma_dump));
    
    // Print what we're about to send
    display_print("[XHCI CTRL] req=");
    display_print_hex(request);
    display_print(" val=");
    display_print_hex(value);
    display_print(" len=");
    display_print_dec(length);
    display_print(" trt=");
    display_print_dec(trt);
    display_print(" enq=");
    display_print_dec(ring->enqueue);
    display_print(" cyc=");
    display_print_dec(ring->cycle);
    display_print("\n");
    
    // 1. Setup Stage TRB
    uint32_t setup_p1 = request_type | (request << 8) | (value << 16);
    uint32_t setup_p2 = index | (length << 16);
    uint32_t setup_status = 8; // TRB Transfer Length = 8
    
    uint32_t setup_index = ring->enqueue;
    uint8_t current_cycle = ring->cycle;
    uint8_t inverted_cycle = current_cycle ^ 1;
    
    // Setup TRB control must initially have inverted_cycle in bit 0 so hardware doesn't fetch it early
    uint32_t initial_setup_control = (TRB_SETUP_STAGE << 10) | (trt << 16) | (1 << 6) | inverted_cycle;
    extern void xhci_ring_enqueue_raw(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control);
    xhci_ring_enqueue_raw(ring, setup_p1, setup_p2, setup_status, initial_setup_control);

    g_usb_phase3.setup_dw0 = setup_p1;
    g_usb_phase3.setup_dw1 = setup_p2;
    g_usb_phase3.setup_dw2 = setup_status;
    g_usb_phase3.setup_dw3 = initial_setup_control;
    g_usb_phase3.setup_trb_sent = true;
    
    // 2. Data Stage TRB (Optional)
    if (length > 0 && data == NULL) {
        display_print("[XHCI ALERT] Error: NULL data buffer passed to control transfer with length=");
        display_print_dec(length);
        display_print("\n");
        return false;
    }

    if (length > 0 && data != NULL) {
        if (!g_ctrl_dma_buf) {
            g_ctrl_dma_buf = xhci_alloc_dma(4096, &g_ctrl_dma_phys, "CtrlDMA");
        }
        
        // Zero DMA buffer
        memset(g_ctrl_dma_buf, 0, 4096);
        
        // For OUT transfers (CPU to Device, e.g. SET_REPORT for LEDs), copy payload first
        if (!(request_type & 0x80)) {
            memcpy(g_ctrl_dma_buf, data, length);
        }

        // Flush DMA buffer from CPU cache to DRAM so xHCI DMA reads fresh data
        uint8_t* dma_ptr = (uint8_t*)g_ctrl_dma_buf;
        for (size_t i = 0; i < 4096; i += 64) {
            asm volatile ("clflush (%0)" :: "r"(dma_ptr + i) : "memory");
        }
        asm volatile ("mfence" ::: "memory");
        
        uint32_t data_p1 = (uint32_t)(g_ctrl_dma_phys & 0xFFFFFFFF);
        uint32_t data_p2 = (uint32_t)((g_ctrl_dma_phys >> 32) & 0xFFFFFFFF);
        uint32_t data_status = length & 0x1FFFF;
        
        uint32_t dir = (request_type & 0x80) ? (1 << 16) : 0; // DIR = bit 16
        uint32_t isp = (request_type & 0x80) ? (1 << 2) : 0;  // ISP = bit 2 (Interrupt On Short Packet - Linux/NT standard)
        uint32_t data_control = (TRB_DATA_STAGE << 10) | dir | isp;
        
        xhci_ring_enqueue(ring, data_p1, data_p2, data_status, data_control);

        g_usb_phase3.data_dw0 = data_p1;
        g_usb_phase3.data_dw1 = data_p2;
        g_usb_phase3.data_dw2 = data_status;
        g_usb_phase3.data_dw3 = (data_control & ~1) | ring->cycle;
        g_usb_phase3.data_trb_sent = true;
    }
    
    // 3. Status Stage TRB
    uint32_t status_dir = 0;
    if (trt == 0 || trt == 2) status_dir = (1 << 16); // IN for No Data or OUT Data
    
    uint32_t status_control = (TRB_STATUS_STAGE << 10) | status_dir | (1 << 5); // IOC = bit 5
    
    xhci_ring_enqueue(ring, 0, 0, 0, status_control);

    g_usb_phase3.status_dw0 = 0;
    g_usb_phase3.status_dw1 = 0;
    g_usb_phase3.status_dw2 = 0;
    g_usb_phase3.status_dw3 = (status_control & ~1) | ring->cycle;
    g_usb_phase3.status_trb_sent = true;
    
    asm volatile ("mfence" ::: "memory");
    
    // Flip the cycle bit of the Setup TRB to current_cycle now that all TRBs are in memory
    ring->trbs[setup_index].control = (ring->trbs[setup_index].control & ~1) | current_cycle;
    asm volatile ("clflush (%0)" :: "r"(&ring->trbs[setup_index]) : "memory");
    g_usb_phase3.setup_dw3 = ring->trbs[setup_index].control;

    g_usb_phase3.ep0_enq_after = ring->enqueue;
    g_usb_phase3.ep0_cyc_after = ring->cycle;
    
    // Ensure all TRB writes are committed to memory before ringing doorbell
    asm volatile ("mfence" ::: "memory");

    // Ring Doorbell (Target = 1 for EP0)
    g_xhci_db_regs[slot_id] = 1;
    g_usb_phase3.doorbell_slot = slot_id;
    g_usb_phase3.doorbell_target = 1;
    g_usb_phase3.doorbell_rung = true;
    
    // Wait for completion (50ms timeout)
    volatile uint32_t wait = 0;
    while (!g_xhci_transfer_complete[slot_id]) {
        extern void xhci_poll(void);
        xhci_poll();
        extern void delay_cycles(uint64_t);
        delay_cycles(1000);
        wait++;
        if (wait > 100000) {
            g_usb_phase3.timeout_occurred = true;
            display_print("[XHCI XFER] Control Transfer Timeout (50ms)! req=");
            display_print_hex(request);
            display_print("\n");
            extern bool xhci_reset_endpoint(uint8_t slot_id, uint8_t ep_index);
            xhci_reset_endpoint(slot_id, 1);
            return false;
        }
    }
    
    g_usb_phase3.transfer_event_received = true;
    extern volatile uint32_t g_cfg_last_completion_code;
    extern volatile uint32_t g_cfg_last_transfer_length;
    g_usb_phase3.completion_code = g_cfg_last_completion_code;
    g_usb_phase3.residual_length = g_cfg_last_transfer_length;

    g_usb_phase3.dma_phys = g_ctrl_dma_phys;
    g_usb_phase3.dma_virt = (uint64_t)g_ctrl_dma_buf;

    display_print("[XHCI FORENSIC AUDIT] req="); display_print_hex(request);
    display_print(" val="); display_print_hex(value);
    display_print(" len="); display_print_dec(length);
    display_print(" Code="); display_print_dec(g_cfg_last_completion_code);
    display_print(" ResidualLen="); display_print_dec(g_cfg_last_transfer_length);
    display_print("\n");

    if (g_cfg_last_completion_code != 1 && g_cfg_last_completion_code != 13) {
        display_print("[XHCI XFER] Transfer Error Completion Code: ");
        display_print_dec(g_cfg_last_completion_code);
        display_print("\n");
        // If STALL (Code 6), clear halt on EP0 immediately
        if (g_cfg_last_completion_code == 6) {
            extern bool xhci_reset_endpoint(uint8_t slot_id, uint8_t ep_index);
            xhci_reset_endpoint(slot_id, 1);
        }
        return false;
    }

    // Action A: Dump first 32 bytes of DMA buffer immediately after Transfer Event
    if (g_ctrl_dma_buf) {
        uint8_t* raw_dma = (uint8_t*)g_ctrl_dma_buf;
        for (int i = 0; i < 32; i++) {
            g_usb_phase3.dma_dump[i] = raw_dma[i];
        }

        display_print("[XHCI DMA DUMP 0..7] ");
        for (int i = 0; i < 8; i++) {
            display_print_hex(raw_dma[i]);
            display_print(" ");
        }
        display_print("\n");

        if (raw_dma[0] == 0 && raw_dma[1] == 0 && raw_dma[2] == 0) {
            g_usb_phase3.dma_forensic_result = "DMA_WRITE_FAILED (RAW_DMA_ZEROED)";
        } else if (raw_dma[0] == 9 && raw_dma[1] == 2) {
            g_usb_phase3.dma_forensic_result = "DMA_WRITE_OK (HEADER_VALID)";
        } else {
            g_usb_phase3.dma_forensic_result = "DMA_WRITE_OK (RAW_BYTES_NONZERO)";
        }
    }

    if (length > 0 && data != NULL && (request_type & 0x80)) {
        // Invalidate CPU L1/L2/L3 cache lines for DMA buffer so CPU reads fresh DRAM data written by PCI xHCI hardware
        uint8_t* dma_ptr = (uint8_t*)g_ctrl_dma_buf;
        for (size_t i = 0; i < length; i += 64) {
            asm volatile ("clflush (%0)" :: "r"(dma_ptr + i) : "memory");
        }
        asm volatile ("mfence" ::: "memory");

        memcpy(data, g_ctrl_dma_buf, length);
    }
    
    display_print("[XHCI CTRL] req=");
    display_print_hex(request);
    display_print(" DONE OK\n");
    
    return true;
}

extern XHCIInputContext* g_xhci_in_ctx[256];
extern XHCIRing g_xhci_cmd_ring;
extern volatile bool g_xhci_cmd_complete;
extern volatile uint32_t g_xhci_last_cmd_completion_code;

XHCIRing g_xhci_ep_ring[256][32];
bool g_xhci_ep_configured[256][32];

bool xhci_interrupt_in_transfer(USBDevice* dev, uint8_t ep_num, uint16_t max_packet_size, void* buffer, uint32_t length) {
    uint8_t slot_id = dev->slot_id;
    uint8_t dci = (ep_num * 2) + 1; // IN endpoint
    
    if (!g_xhci_ep_configured[slot_id][dci]) {
        // 1. Configure Endpoint
        XHCIInputContext* in_ctx = g_xhci_in_ctx[slot_id];
        extern void* memset(void*, int, size_t);
        memset(in_ctx, 0, sizeof(XHCIInputContext));
        
        XHCIInputControlContext* ctrl_ctx = xhci_get_input_ctrl_ctx(in_ctx);
        ctrl_ctx->add_context_flags = (1 << 0) | (1 << dci); // Must include bit 0 (Slot Context) when adding endpoints
        ctrl_ctx->drop_context_flags = 0;
        
        // Copy existing Slot Context from output Device Context so Port Number, Speed, and Route String are preserved!
        extern XHCIDeviceContext* g_xhci_dev_ctx[256];
        XHCISlotContext* orig_slot_ctx = xhci_get_slot_ctx(g_xhci_dev_ctx[slot_id], false);
        XHCISlotContext* slot_ctx = xhci_get_slot_ctx(in_ctx, true);
        *slot_ctx = *orig_slot_ctx;
        
        uint32_t ctx_entries = (slot_ctx->field1 >> 27) & 31;
        if (dci > ctx_entries) {
            slot_ctx->field1 = (slot_ctx->field1 & ~(31 << 27)) | (dci << 27);
        }
        
        if (g_xhci_ep_ring[slot_id][dci].size == 0) {
            xhci_ring_init(&g_xhci_ep_ring[slot_id][dci], 64);
            extern void usb_forensic_mark_stage(int stage, bool success);
            usb_forensic_mark_stage(12, true); // USB_STAGE_INTERRUPT_IN_RING_CREATED
            
            XHCIEndpointContext* ep_ctx = xhci_get_ep_ctx(in_ctx, true, dci - 1);
            uint32_t ep_type = 7; // Interrupt IN
            uint32_t interval_val = 7; // Exponent 7 = 8ms (2^(7-1) * 125us = 8ms) for HID
            ep_ctx->field1 = (interval_val << 16);
            ep_ctx->field2 = (3 << 1) | (ep_type << 3) | ((max_packet_size & 0xFFFF) << 16);
            ep_ctx->tr_dequeue_ptr = g_xhci_ep_ring[slot_id][dci].phys_base | 1;
            ep_ctx->field5 = (max_packet_size & 0xFFFF); // Average TRB Length (DW4) MUST BE NON-ZERO for Haswell H81 hardware!
        }
        
        // Flush input context from CPU cache
        uint8_t* in_ptr = (uint8_t*)in_ctx;
        for (size_t i = 0; i < sizeof(XHCIInputContext); i += 64) {
            asm volatile ("clflush (%0)" :: "r"(in_ptr + i) : "memory");
        }
        asm volatile ("mfence" ::: "memory");

        // Issue Configure Endpoint Command
        uint64_t in_ctx_phys = (uint64_t)in_ctx;
        uint32_t control = (TRB_CONFIGURE_ENDPOINT_CMD << 10) | (slot_id << 24);
        
        g_xhci_cmd_complete = false;
        g_xhci_last_cmd_completion_code = 0;
        
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(10, true); // USB_STAGE_CONFIGURE_ENDPOINT_CMD_SENT
        xhci_ring_enqueue(&g_xhci_cmd_ring, 
                          (uint32_t)(in_ctx_phys & 0xFFFFFFFF), 
                          (uint32_t)((in_ctx_phys >> 32) & 0xFFFFFFFF), 
                          0, control);
                          
        g_xhci_db_regs[0] = 0;
        
        uint32_t wait = 0;
        while (!g_xhci_cmd_complete) {
            extern void xhci_poll(void);
            xhci_poll();
            extern void delay_cycles(uint64_t);
            delay_cycles(1000);
            wait++;
            if (wait > 100000) {
                usb_forensic_mark_stage(11, false);
                return false;
            }
        }
        
        extern void display_print(const char*);
        extern void display_print_dec(uint64_t);
        display_print("[XHCI] Configure Endpoint Command Completion Code: ");
        display_print_dec(g_xhci_last_cmd_completion_code);
        display_print("\n");
        
        if (g_xhci_last_cmd_completion_code != 1) {
            usb_forensic_mark_stage(11, false);
            return false;
        }
        
        usb_forensic_mark_stage(11, true); // USB_STAGE_CONFIGURE_ENDPOINT_EVENT_RECEIVED
        g_xhci_ep_configured[slot_id][dci] = true;
        g_usb_diag.configure_ep_pass = true;
        display_print("[XHCI] Configured Interrupt IN EP\n");
    }
    
    // 2. Queue Normal TRB
    XHCIRing* ring = &g_xhci_ep_ring[slot_id][dci];
    uint64_t buf_phys = (uint64_t)buffer; // assuming identity mapped
    
    uint32_t status = length & 0x1FFFF;
    uint32_t control = (TRB_NORMAL << 10) | (1 << 5); // IOC = bit 5
    
    xhci_ring_enqueue(ring, 
                      (uint32_t)(buf_phys & 0xFFFFFFFF), 
                      (uint32_t)((buf_phys >> 32) & 0xFFFFFFFF), 
                      status, control);
    usb_forensic_mark_stage(13, true); // USB_STAGE_INTERRUPT_IN_TRB_QUEUED
                      
    asm volatile ("mfence" ::: "memory");
                      
    // 3. Ring Doorbell
    g_xhci_db_regs[slot_id] = dci;
    usb_forensic_mark_stage(14, true); // USB_STAGE_INTERRUPT_IN_DOORBELL_RUNG
    
    return true;
}

// Define the transfer event handler here since it's closely related
void xhci_handle_transfer_event(uint32_t slot_id, uint32_t completion_code, uint32_t transfer_length, XHCITrb* trb) {
    if (completion_code != 1 && completion_code != 13) {
        // Not Success or Short Packet
        return;
    }
    
    extern volatile uint64_t g_xhci_transfers;
    g_xhci_transfers++;
    
    uint32_t dci = (trb->control >> 16) & 0x1F; // Endpoint ID from Transfer Event TRB
    if (dci != 1) {
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(15, true); // USB_STAGE_FIRST_TRANSFER_EVENT_RECEIVED
        // It's not EP0, it's an interrupt endpoint
        uint8_t ep_num = dci / 2;
        extern USBDevice* usb_get_device_by_slot(uint8_t slot_id);
        USBDevice* dev = usb_get_device_by_slot(slot_id);
        
        extern void display_print(const char*);
        extern void display_print_dec(uint64_t);
        if (dev && dev->driver_data) {
            uint32_t requested_length = 8;
            uint32_t actual_length = requested_length - transfer_length;
            
            extern volatile uint64_t g_usb_reports_count;
            g_usb_reports_count++;
            extern void usb_hid_report_received(USBDevice* dev, uint8_t* report, uint32_t length, uint8_t protocol);
            uint8_t prot = dev->protocol ? dev->protocol : 2;
            usb_hid_report_received(dev, (uint8_t*)dev->driver_data, actual_length, prot);
            
            // Requeue TRB to continue polling
            xhci_interrupt_in_transfer(dev, ep_num, 8, dev->driver_data, 8);
            
            XHCIRing* ring = &g_xhci_ep_ring[slot_id][dci];
            /*
            display_print("[XHCI REQUEUE] EnqueueIndex="); display_print_dec(ring->enqueue);
            display_print(" CycleBit="); display_print_dec(ring->cycle);
            display_print(" Doorbell target="); display_print_dec(dci);
            display_print("\n");
            */
            
        } else {
            display_print("[XHCI TRANSFER] NO MATCH Slot="); display_print_dec(slot_id);
            display_print(" EP="); display_print_dec(ep_num);
            display_print(" DCI="); display_print_dec(dci);
            display_print("\n");
        }
    }
}
