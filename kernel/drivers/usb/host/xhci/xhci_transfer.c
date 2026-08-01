#include "kernel/drivers/usb/core/usb_core.h"
#include "xhci.h"

extern XHCIRing g_xhci_ep0_ring[256];
extern volatile uint32_t* g_xhci_db_regs;
extern volatile bool g_xhci_transfer_complete[256];
extern volatile uint32_t g_xhci_transfer_length[256];

extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);
extern void* memcpy(void*, const void*, size_t);

static void* g_ctrl_dma_buf = NULL;
static uint64_t g_ctrl_dma_phys = 0;

bool xhci_control_transfer(USBDevice* dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length, void* data) {
    uint32_t slot_id = dev->slot_id;
    XHCIRing* ring = &g_xhci_ep0_ring[slot_id];

    /* Safety: if ring is not initialized yet, abort to prevent Divide-by-Zero */
    if (!ring->trbs || ring->size == 0) {
        extern void display_print(const char*);
        display_print("[XHCI XFER] EP0 ring not initialized for slot=");
        extern void display_print_dec(uint64_t);
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
    
    // 1. Setup Stage TRB
    uint32_t setup_p1 = request_type | (request << 8) | (value << 16);
    uint32_t setup_p2 = index | (length << 16);
    uint32_t setup_status = 8; // TRB Transfer Length = 8
    uint32_t setup_control = (TRB_SETUP_STAGE << 10) | (trt << 16) | (1 << 6); // IDT = bit 6
    
    uint32_t setup_index = ring->enqueue;
    uint8_t current_cycle = ring->cycle;
    
    // Invert the cycle bit for the Setup TRB initially to prevent hardware from executing it early
    uint32_t deferred_setup_control = setup_control ^ 1;
    extern void xhci_ring_enqueue_raw(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control);
    xhci_ring_enqueue_raw(ring, setup_p1, setup_p2, setup_status, deferred_setup_control);
    
    uint32_t data_p1 = 0, data_p2 = 0, data_status = 0, data_control = 0;
    
    // 2. Data Stage TRB (Optional)
    if (length > 0 && data != NULL) {
        if (!g_ctrl_dma_buf) {
            g_ctrl_dma_buf = xhci_alloc_dma(4096, &g_ctrl_dma_phys, "CtrlDMA");
        }
        
        if (!(request_type & 0x80)) {
            memcpy(g_ctrl_dma_buf, data, length);
        }
        
        data_p1 = (uint32_t)(g_ctrl_dma_phys & 0xFFFFFFFF);
        data_p2 = (uint32_t)((g_ctrl_dma_phys >> 32) & 0xFFFFFFFF);
        extern void display_print_hex(uint64_t);
        display_print_hex(data_p1);
        data_status = length & 0x1FFFF;
        
        uint32_t dir = (request_type & 0x80) ? (1 << 16) : 0; // DIR = bit 16
        data_control = (TRB_DATA_STAGE << 10) | dir;
        
        xhci_ring_enqueue(ring, data_p1, data_p2, data_status, data_control);
    }
    
    // 3. Status Stage TRB
    uint32_t status_dir = 0;
    if (trt == 0 || trt == 2) status_dir = (1 << 16); // IN for No Data or OUT Data
    
    uint32_t status_control = (TRB_STATUS_STAGE << 10) | status_dir | (1 << 5); // IOC = bit 5
    
    xhci_ring_enqueue(ring, 0, 0, 0, status_control);
    
    asm volatile ("mfence" ::: "memory");
    
    // Flip the cycle bit of the Setup TRB to the correct value now that all TRBs are in memory
    ring->trbs[setup_index].control = (ring->trbs[setup_index].control & ~1) | current_cycle;
    
    // Ensure all TRB writes are committed to memory before ringing doorbell
    asm volatile ("mfence" ::: "memory");
    
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);
    
    display_print("[XHCI TRANSFER RING DUMP]\n");
    for (int i = 0; i < 4; i++) {
        if (ring->size == 0) break;  /* guard: ring not initialized */
        uint32_t idx = (setup_index + i) % ring->size;
        XHCITrb* trb = &ring->trbs[idx];
        display_print("Idx="); display_print_dec(idx);
        display_print(" LO="); display_print_hex(trb->param1);
        display_print(" HI="); display_print_hex(trb->param2);
        display_print(" STS="); display_print_hex(trb->status);
        display_print(" CTL="); display_print_hex(trb->control); display_print("\n");
    }
    
    display_print("[XHCI EP0 DEBUG]\n");
    display_print("Slot="); display_print_dec(slot_id);
    display_print(" DoorbellTarget=1\n");
    display_print("RingPhys="); display_print_hex(ring->phys_base);
    display_print(" EnqueueIndex="); display_print_dec(ring->enqueue);
    display_print(" Cycle="); display_print_dec(ring->cycle); display_print("\n");
    
    display_print("SETUP:\n");
    display_print("PARAM_LO="); display_print_hex(setup_p1);
    display_print(" PARAM_HI="); display_print_hex(setup_p2);
    display_print(" STATUS="); display_print_hex(setup_status);
    display_print(" CONTROL="); display_print_hex(setup_control); display_print("\n");
    
    if (length > 0) {
        display_print("DATA:\n");
        display_print("PARAM_LO="); display_print_hex(data_p1);
        display_print(" PARAM_HI="); display_print_hex(data_p2);
        display_print(" STATUS="); display_print_hex(data_status);
        display_print(" CONTROL="); display_print_hex(data_control); display_print("\n");
    }
    
    display_print("STATUS:\n");
    display_print("PARAM_LO="); display_print_hex(0);
    display_print(" PARAM_HI="); display_print_hex(0);
    display_print(" STATUS="); display_print_hex(0);
    display_print(" CONTROL="); display_print_hex(status_control); display_print("\n");

    // Ring Doorbell (Target = 1 for EP0)
    g_xhci_db_regs[slot_id] = 1;
    
    // Wait for completion
    uint32_t wait = 0;
    while (!g_xhci_transfer_complete[slot_id]) {
        extern void xhci_poll(void);
        xhci_poll();
        extern void delay_cycles(uint64_t);
        delay_cycles(1000);
        wait++;
        if (wait > 10000000) { // Increased timeout significantly
            display_print("[XHCI XFER] Control Transfer Timeout\n");
            extern volatile uint32_t* g_xhci_ir_regs;
            if (g_xhci_ir_regs) {
                volatile uint64_t* erdp = (volatile uint64_t*)(g_xhci_ir_regs + 6);
                display_print("[XHCI EVENT DEBUG] ERDP="); display_print_hex(*erdp); display_print("\n");
                
                // Dump next 8 TRBs from event ring
                extern XHCIRing g_xhci_event_ring;
                display_print("[XHCI RAW EVENT RING DUMP]\n");
                uint32_t start_idx = g_xhci_event_ring.dequeue;
                for (int i = 0; i < 8; i++) {
                    uint32_t idx = (start_idx + i) % g_xhci_event_ring.size;
                    XHCITrb* trb = &g_xhci_event_ring.trbs[idx];
                    display_print("Idx="); display_print_dec(idx);
                    display_print(" LO="); display_print_hex(trb->param1);
                    display_print(" HI="); display_print_hex(trb->param2);
                    display_print(" STS="); display_print_hex(trb->status);
                    display_print(" CTL="); display_print_hex(trb->control);
                    
                    uint32_t type = (trb->control >> 10) & 0x3F;
                    uint32_t cycle = trb->control & 1;
                    display_print(" Type="); display_print_dec(type);
                    display_print(" Cyc="); display_print_dec(cycle);
                    display_print("\n");
                }
            }
            return false;
        }
    }
    
    if (length > 0 && data != NULL && (request_type & 0x80)) {
        memcpy(data, g_ctrl_dma_buf, length);
        
        display_print("[XHCI EP0] GET_DESCRIPTOR SUCCESS. First 8 bytes:\n");
        uint8_t* dump = (uint8_t*)data;
        for (int i = 0; i < (length > 8 ? 8 : length); i++) {
            display_print_hex(dump[i]); display_print(" ");
        }
        display_print("\n");
    }
    
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
        
        XHCISlotContext* slot_ctx = xhci_get_slot_ctx(in_ctx, true);
        uint32_t ctx_entries = (slot_ctx->field1 >> 27) & 31;
        if (dci > ctx_entries) {
            slot_ctx->field1 = (slot_ctx->field1 & ~(31 << 27)) | (dci << 27);
        }
        
        if (g_xhci_ep_ring[slot_id][dci].size == 0) {
            xhci_ring_init(&g_xhci_ep_ring[slot_id][dci], 16);
            
            XHCIEndpointContext* ep_ctx = xhci_get_ep_ctx(in_ctx, true, dci - 1);
            uint32_t ep_type = 7; // Interrupt IN
            ep_ctx->field1 = (6 << 16); // Interval (e.g., 6)
            ep_ctx->field2 = (3 << 1) | (ep_type << 3) | ((max_packet_size & 0xFFFF) << 16);
            ep_ctx->tr_dequeue_ptr = g_xhci_ep_ring[slot_id][dci].phys_base | 1;
        }
        
        // Issue Configure Endpoint Command
        uint64_t in_ctx_phys = (uint64_t)in_ctx;
        uint32_t control = (TRB_CONFIGURE_ENDPOINT_CMD << 10) | (slot_id << 24);
        
        g_xhci_cmd_complete = false;
        g_xhci_last_cmd_completion_code = 0;
        
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
            if (wait > 100000) return false;
        }
        
        extern void display_print(const char*);
        extern void display_print_dec(uint64_t);
        display_print("[XHCI] Configure Endpoint Command Completion Code: ");
        display_print_dec(g_xhci_last_cmd_completion_code);
        display_print("\n");
        
        if (g_xhci_last_cmd_completion_code != 1) {
            return false;
        }
        
        g_xhci_ep_configured[slot_id][dci] = true;
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
                      
    asm volatile ("mfence" ::: "memory");
                      
    // 3. Ring Doorbell
    g_xhci_db_regs[slot_id] = dci;
    
    return true;
}

// Define the transfer event handler here since it's closely related
void xhci_handle_transfer_event(uint32_t slot_id, uint32_t completion_code, uint32_t transfer_length, XHCITrb* trb) {
    if (completion_code != 1 && completion_code != 13) {
        // Not Success or Short Packet
        return;
    }
    
    uint32_t dci = (trb->control >> 16) & 0x1F; // Endpoint ID from Transfer Event TRB
    if (dci != 1) {
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
            usb_hid_report_received(dev, (uint8_t*)dev->driver_data, actual_length, 2);
            
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
