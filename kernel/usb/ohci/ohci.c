#include "ohci.h"
#include "../common/usb_dma.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static inline uint32_t ohci_read32(uint64_t base, uint32_t reg) {
    return *(volatile uint32_t*)(base + reg);
}

static inline void ohci_write32(uint64_t base, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(base + reg) = val;
}

static void delay_us(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++);
}

bool ohci_init(ohci_controller_t* odev, uint64_t mmio_base, uint8_t irq) {
    if (!odev || mmio_base == 0) return false;
    
    memset(odev, 0, sizeof(ohci_controller_t));
    odev->mmio_base = mmio_base;
    odev->irq = irq;
    atoms_spinlock_init(&odev->lock, 1);
    
    display_print("[OHCI] Initializing OHCI Host Controller at MMIO: 0x");
    display_print_hex(mmio_base);
    display_print("\n");
    
    uint32_t rev = ohci_read32(mmio_base, OHCI_REG_REVISION);
    display_print("[OHCI] Controller Revision: 0x");
    display_print_hex(rev);
    display_print("\n");
    
    // Software Reset
    ohci_write32(mmio_base, OHCI_REG_CMD_STATUS, 1); // HostControllerReset
    delay_us(10000);
    
    // Allocate HCCA (256-byte aligned)
    uint64_t hcca_phys = 0;
    odev->hcca = (ohci_hcca_t*)usb_dma_alloc(sizeof(ohci_hcca_t), 256, &hcca_phys, "OHCI_HCCA");
    odev->hcca_phys = hcca_phys;
    
    // Allocate Control and Bulk ED Head nodes
    uint64_t ctrl_phys = 0, bulk_phys = 0;
    odev->control_head = (ohci_ed_t*)usb_dma_alloc(sizeof(ohci_ed_t), 16, &ctrl_phys, "OHCI_CtrlED");
    odev->bulk_head = (ohci_ed_t*)usb_dma_alloc(sizeof(ohci_ed_t), 16, &bulk_phys, "OHCI_BulkED");
    
    odev->control_head->flags = (1 << 14); // Skip ED
    odev->bulk_head->flags = (1 << 14);    // Skip ED
    
    // Set HCCA, ControlHead, and BulkHead Registers
    ohci_write32(mmio_base, OHCI_REG_HCCA, (uint32_t)hcca_phys);
    ohci_write32(mmio_base, OHCI_REG_CONTROL_HEAD_ED, (uint32_t)ctrl_phys);
    ohci_write32(mmio_base, OHCI_REG_BULK_HEAD_ED, (uint32_t)bulk_phys);
    
    // Read Root Hub Descriptor A to get port count
    uint32_t rh_a = ohci_read32(mmio_base, OHCI_REG_RH_DESCRIPTOR_A);
    odev->port_count = rh_a & 0xFF;
    if (odev->port_count == 0 || odev->port_count > 15) odev->port_count = 2;
    
    // Set Operational Control State (ControlListEnable | BulkListEnable | Operational)
    ohci_write32(mmio_base, OHCI_REG_CONTROL, (1 << 4) | (1 << 5) | (2 << 6));
    odev->running = true;
    
    display_print("[OHCI] Controller Operational with ");
    display_print_dec(odev->port_count);
    display_print(" Root Ports.\n");
    return true;
}

void ohci_shutdown(ohci_controller_t* odev) {
    if (!odev || !odev->running) return;
    ohci_write32(odev->mmio_base, OHCI_REG_CONTROL, 0); // Suspend state
    odev->running = false;
    display_print("[OHCI] Controller Shutdown Complete.\n");
}

bool ohci_reset(ohci_controller_t* odev) {
    if (!odev) return false;
    ohci_shutdown(odev);
    return ohci_init(odev, odev->mmio_base, odev->irq);
}

bool ohci_port_reset(ohci_controller_t* odev, uint8_t port) {
    if (!odev || port >= odev->port_count) return false;
    
    uint32_t port_reg = OHCI_REG_RH_PORT_STATUS + (port * 4);
    uint32_t status = ohci_read32(odev->mmio_base, port_reg);
    
    if (!(status & 1)) { // CurrentConnectStatus
        display_print("[OHCI PORT] Issuing port reset on empty port ");
        display_print_dec(port); display_print("\n");
        ohci_write32(odev->mmio_base, port_reg, (1 << 4));
        delay_us(10000);
        return true;
    }
    
    // SetPortReset (bit 4)
    ohci_write32(odev->mmio_base, port_reg, (1 << 4));
    delay_us(50000); // 50ms reset assertion
    
    // Enable Port (SetPortEnable - bit 1)
    ohci_write32(odev->mmio_base, port_reg, (1 << 1));
    display_print("[OHCI PORT] Port "); display_print_dec(port); display_print(" Reset & Enabled.\n");
    return true;
}

bool ohci_submit_bulk(ohci_controller_t* odev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len) {
    if (!odev || !odev->running || !buf) return false;
    
    uint64_t td_phys = 0, ed_phys = 0;
    ohci_td_t* td = (ohci_td_t*)usb_dma_alloc(sizeof(ohci_td_t), 16, &td_phys, "OHCI_BulkTD");
    ohci_ed_t* ed = (ohci_ed_t*)usb_dma_alloc(sizeof(ohci_ed_t), 16, &ed_phys, "OHCI_BulkED");
    
    // Setup TD
    td->flags = (dir_in ? 2 : 1) << 19; // DP: IN=2, OUT=1
    td->cbp = (uint32_t)(uint64_t)buf;
    td->be = (uint32_t)(uint64_t)buf + len - 1;
    td->next_td = 0;
    
    // Setup ED
    ed->flags = (dev_addr & 0x7F) | ((ep & 0x0F) << 7) | (64 << 16); // MPS 64
    ed->head_p = (uint32_t)td_phys;
    ed->tail_p = 0;
    ed->next_ed = 0;
    
    // Link to Bulk Head ED
    odev->bulk_head->next_ed = (uint32_t)ed_phys;
    ohci_write32(odev->mmio_base, OHCI_REG_CMD_STATUS, (1 << 2)); // BulkListFilled
    
    return true;
}

void ohci_poll(ohci_controller_t* odev) {
    if (!odev || !odev->running) return;
    uint32_t intr = ohci_read32(odev->mmio_base, OHCI_REG_INTR_STATUS);
    if (intr) {
        ohci_write32(odev->mmio_base, OHCI_REG_INTR_STATUS, intr); // Write 1 to clear
    }
}
