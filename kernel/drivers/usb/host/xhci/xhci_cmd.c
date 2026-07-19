#include "kernel/drivers/usb/core/usb_core.h"
#include "xhci.h"

extern volatile uint32_t g_xhci_last_cmd_completion_code;
extern volatile uint32_t g_xhci_last_cmd_slot_id;
extern volatile bool g_xhci_cmd_complete;
extern XHCIRing g_xhci_cmd_ring;
extern volatile uint32_t* g_xhci_db_regs;

uint8_t xhci_enable_slot(void) {
    g_xhci_cmd_complete = false;
    g_xhci_last_cmd_completion_code = 0;
    g_xhci_last_cmd_slot_id = 0;

    // TRB_ENABLE_SLOT_CMD = 9
    xhci_ring_enqueue(&g_xhci_cmd_ring, 0, 0, 0, (TRB_ENABLE_SLOT_CMD << 10));
    
    // Ring Doorbell 0 (Host Controller)
    g_xhci_db_regs[0] = 0;

    // Wait for completion (xhci_poll will handle the event)
    uint32_t wait = 0;
    while (!g_xhci_cmd_complete) {
        extern void xhci_poll(void);
        xhci_poll();
        extern void delay_cycles(uint32_t);
        delay_cycles(1000);
        wait++;
        if (wait > 100000) {
            extern void display_print(const char*);
            display_print("[XHCI CMD] Enable Slot Timeout\n");
            return 0;
        }
    }

    if (g_xhci_last_cmd_completion_code != 1) { // 1 = Success
        extern void display_print(const char*);
        display_print("[XHCI CMD] Enable Slot Failed\n");
        return 0;
    }

    extern void display_print(const char*);
    display_print("[XHCI CMD] Enable Slot SUCCESS Slot=");
    extern void display_print_dec(uint64_t);
    display_print_dec(g_xhci_last_cmd_slot_id);
    display_print("\n");

    return (uint8_t)g_xhci_last_cmd_slot_id;
}

extern XHCIDcbaa* g_xhci_dcbaa;
extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);
extern void xhci_ring_init(XHCIRing* ring, uint32_t num_trbs);

// Global arrays to hold device contexts and EP0 rings
XHCIDeviceContext* g_xhci_dev_ctx[256];
XHCIInputContext*  g_xhci_in_ctx[256];
XHCIRing           g_xhci_ep0_ring[256];

bool xhci_address_device(uint8_t slot_id, uint8_t port, uint8_t speed) {
    g_xhci_cmd_complete = false;
    g_xhci_last_cmd_completion_code = 0;
    
    // 1. Allocate Device Context (output)
    uint64_t out_ctx_phys = 0;
    XHCIDeviceContext* out_ctx = (XHCIDeviceContext*)xhci_alloc_dma(sizeof(XHCIDeviceContext), &out_ctx_phys, "DevCtx");
    extern void* memset(void*, int, size_t);
    memset(out_ctx, 0, sizeof(XHCIDeviceContext));
    
    g_xhci_dev_ctx[slot_id] = out_ctx;
    g_xhci_dcbaa->pointers[slot_id] = out_ctx_phys;
    
    // 2. Allocate Input Context (input)
    uint64_t in_ctx_phys = 0;
    XHCIInputContext* in_ctx = (XHCIInputContext*)xhci_alloc_dma(sizeof(XHCIInputContext), &in_ctx_phys, "InCtx");
    memset(in_ctx, 0, sizeof(XHCIInputContext));
    g_xhci_in_ctx[slot_id] = in_ctx;
    
    // 3. Setup Input Control Context
    XHCIInputControlContext* ctrl_ctx = xhci_get_input_ctrl_ctx(in_ctx);
    ctrl_ctx->add_context_flags = (1 << 0) | (1 << 1); // Add Slot Context (0) and EP0 Context (1)
    
    // 4. Setup Slot Context
    XHCISlotContext* slot_ctx = xhci_get_slot_ctx(in_ctx, true);
    slot_ctx->field1 = (1 << 27); // Context Entries = 1
    if (speed == 1) slot_ctx->field1 |= (1 << 20); // Full speed
    else if (speed == 2) slot_ctx->field1 |= (2 << 20); // Low speed
    else if (speed == 3) slot_ctx->field1 |= (3 << 20); // High speed
    else if (speed == 4) slot_ctx->field1 |= (4 << 20); // Super speed
    
    slot_ctx->field2 = (port << 16); // Root Hub Port Number
    
    // 5. Setup EP0 Context
    xhci_ring_init(&g_xhci_ep0_ring[slot_id], 16); // Small ring for EP0
    
    uint32_t max_packet_size = 8;
    if (speed == 3) max_packet_size = 64;
    else if (speed == 4) max_packet_size = 512;
    
    XHCIEndpointContext* ep0_ctx = xhci_get_ep_ctx(in_ctx, true, 0);
    // CErr is bits 1-2. EP Type is bits 3-5. Max Packet Size is bits 16-31.
    ep0_ctx->field2 = (3 << 1) | (4 << 3) | ((max_packet_size & 0xFFFF) << 16); 
    ep0_ctx->tr_dequeue_ptr = g_xhci_ep0_ring[slot_id].phys_base | 1; // DCS = 1
    
    // 6. Issue Address Device Command TRB
    // BSR = 0 (Block Set Address request = false)
    uint32_t control = (TRB_ADDRESS_DEVICE_CMD << 10) | (slot_id << 24);
    
    xhci_ring_enqueue(&g_xhci_cmd_ring, 
                      (uint32_t)(in_ctx_phys & 0xFFFFFFFF), 
                      (uint32_t)((in_ctx_phys >> 32) & 0xFFFFFFFF), 
                      0, 
                      control);
                      
    g_xhci_db_regs[0] = 0; // Ring Doorbell 0
    
    uint32_t wait = 0;
    while (!g_xhci_cmd_complete) {
        extern void xhci_poll(void);
        xhci_poll();
        extern void delay_cycles(uint32_t);
        delay_cycles(1000);
        wait++;
        if (wait > 100000) {
            extern void display_print(const char*);
            display_print("[XHCI CMD] Address Device Timeout\n");
            return false;
        }
    }
    
    if (g_xhci_last_cmd_completion_code != 1) { // 1 = Success
        extern void display_print(const char*);
        extern void display_print_dec(uint64_t);
        display_print("[XHCI CMD] Address Device Failed. Code=");
        display_print_dec(g_xhci_last_cmd_completion_code);
        display_print("\n");
        return false;
    }
    
    extern void display_print(const char*);
    display_print("[XHCI CMD] Address Device SUCCESS Slot=");
    extern void display_print_dec(uint64_t);
    display_print_dec(slot_id);
    display_print("\n");
    
    // Dump OUTPUT Device Context
    display_print("[XHCI EP0 OUTPUT CONTEXT]\n");
    XHCISlotContext* out_slot = xhci_get_slot_ctx(out_ctx, false);
    XHCIEndpointContext* out_ep0 = xhci_get_ep_ctx(out_ctx, false, 0);
    
    extern void display_print_hex(uint64_t);
    display_print("Slot DW0="); display_print_hex(out_slot->field1); display_print("\n");
    display_print("Slot DW1="); display_print_hex(out_slot->field2); display_print("\n");
    display_print("EP0 DW0="); display_print_hex(out_ep0->field1); display_print("\n");
    display_print("EP0 DW1="); display_print_hex(out_ep0->field2); display_print("\n");
    display_print("EP0 TRDP="); display_print_hex(out_ep0->tr_dequeue_ptr); display_print("\n");
    
    return true;
}
