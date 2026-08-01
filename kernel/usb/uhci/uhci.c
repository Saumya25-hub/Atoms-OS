#include "uhci.h"
#include "../common/usb_dma.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static void delay_us(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++);
}

bool uhci_init(uhci_controller_t* udev, uint16_t io_base, uint8_t irq) {
    if (!udev || io_base == 0) return false;
    
    memset(udev, 0, sizeof(uhci_controller_t));
    udev->io_base = io_base;
    udev->irq = irq;
    udev->port_count = 2; // UHCI standard has 2 root ports
    atoms_spinlock_init(&udev->lock, 1);
    
    display_print("[UHCI] Initializing UHCI Host Controller at IO Port: 0x");
    display_print_hex(io_base);
    display_print("\n");
    
    // Stop controller
    io_out16(io_base + UHCI_REG_USBCMD, 0);
    delay_us(1000);
    
    // Host Controller Reset
    io_out16(io_base + UHCI_REG_USBCMD, UHCI_CMD_HCRESET);
    delay_us(10000);
    
    uint16_t cmd = io_in16(io_base + UHCI_REG_USBCMD);
    if ((cmd & UHCI_CMD_HCRESET) && cmd != 0xFFFF) {
        display_print("[UHCI] Warning: Hardware reset pending\n");
    }
    
    // Allocate 1024-entry Frame List (4KB aligned)
    uint64_t fl_phys = 0;
    udev->frame_list = (uint32_t*)usb_dma_alloc(4096, 4096, &fl_phys, "UHCI_FrameList");
    udev->frame_list_phys = fl_phys;
    
    // Allocate Async Queue Head (QH)
    uint64_t async_phys = 0;
    udev->async_qh = (uhci_qh_t*)usb_dma_alloc(sizeof(uhci_qh_t), 16, &async_phys, "UHCI_AsyncQH");
    udev->async_qh->head_link = UHCI_PTR_TERMINATE;
    udev->async_qh->element_link = UHCI_PTR_TERMINATE;
    
    // Point all 1024 frame entries to Async QH
    for (int i = 0; i < 1024; i++) {
        udev->frame_list[i] = (uint32_t)async_phys | UHCI_PTR_QH;
    }
    
    // Set Frame List Base Address Register
    io_out32(io_base + UHCI_REG_FRBASEADD, (uint32_t)fl_phys);
    io_out16(io_base + UHCI_REG_FRNUM, 0);
    
    // Start Controller (Set Run/Stop & MaxP 64)
    io_out16(io_base + UHCI_REG_USBCMD, UHCI_CMD_RS | UHCI_CMD_MAXP);
    udev->running = true;
    
    display_print("[UHCI] Controller Successfully Started and Processing Frames.\n");
    return true;
}

void uhci_shutdown(uhci_controller_t* udev) {
    if (!udev || !udev->running) return;
    io_out16(udev->io_base + UHCI_REG_USBCMD, 0);
    udev->running = false;
    display_print("[UHCI] Controller Shutdown Complete.\n");
}

bool uhci_reset(uhci_controller_t* udev) {
    if (!udev) return false;
    uhci_shutdown(udev);
    return uhci_init(udev, udev->io_base, udev->irq);
}

bool uhci_port_reset(uhci_controller_t* udev, uint8_t port) {
    if (!udev || port >= udev->port_count) return false;
    
    uint16_t port_reg = udev->io_base + (port == 0 ? UHCI_REG_PORTSC1 : UHCI_REG_PORTSC2);
    uint16_t status = io_in16(port_reg);
    
    if (!(status & UHCI_PORT_CCS)) {
        display_print("[UHCI PORT] Issuing port reset on empty port ");
        display_print_dec(port); display_print("\n");
        io_out16(port_reg, status | UHCI_PORT_PR);
        delay_us(10000);
        io_out16(port_reg, status & ~UHCI_PORT_PR);
        return true;
    }
    
    // Assert Port Reset bit
    io_out16(port_reg, status | UHCI_PORT_PR);
    delay_us(50000); // 50ms reset assertion
    
    // Clear Port Reset bit
    io_out16(port_reg, status & ~UHCI_PORT_PR);
    delay_us(10000);
    
    // Enable Port
    io_out16(port_reg, io_in16(port_reg) | UHCI_PORT_PE);
    display_print("[UHCI PORT] Port "); display_print_dec(port); display_print(" Reset & Enabled.\n");
    return true;
}

bool uhci_submit_bulk(uhci_controller_t* udev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len) {
    if (!udev || !udev->running || !buf) return false;
    
    uint64_t td_phys = 0;
    uhci_td_t* td = (uhci_td_t*)usb_dma_alloc(sizeof(uhci_td_t), 16, &td_phys, "UHCI_BulkTD");
    
    td->link = UHCI_PTR_TERMINATE;
    td->status = (0x7F << 16) | (1 << 23); // Active, 3 retries
    
    uint32_t pid = dir_in ? 0x69 : 0xE1; // IN=0x69, OUT=0xE1
    td->token = pid | ((dev_addr & 0x7F) << 8) | ((ep & 0x0F) << 15) | (((len - 1) & 0x7FF) << 21);
    td->buffer = (uint32_t)(uint64_t)buf;
    
    // Link TD into Async Queue Head
    udev->async_qh->element_link = (uint32_t)td_phys;
    usb_dma_cache_flush(td, sizeof(uhci_td_t));
    
    return true;
}

void uhci_poll(uhci_controller_t* udev) {
    if (!udev || !udev->running) return;
    uint16_t status = io_in16(udev->io_base + UHCI_REG_USBSTS);
    if (status) {
        // Acknowledge interrupts
        io_out16(udev->io_base + UHCI_REG_USBSTS, status);
    }
}
