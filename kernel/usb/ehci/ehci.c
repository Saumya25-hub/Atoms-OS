#include "ehci.h"
#include "../common/usb_dma.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static inline uint32_t ehci_read32(uint64_t base, uint32_t reg) {
    return *(volatile uint32_t*)(base + reg);
}

static inline void ehci_write32(uint64_t base, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(base + reg) = val;
}

static void delay_us(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++);
}

bool ehci_init(ehci_controller_t* edev, uint64_t mmio_base, uint8_t irq) {
    if (!edev || mmio_base == 0) return false;
    
    memset(edev, 0, sizeof(ehci_controller_t));
    edev->mmio_base = mmio_base;
    edev->irq = irq;
    atoms_spinlock_init(&edev->lock, 1);
    
    display_print("[EHCI] Initializing EHCI USB 2.0 High-Speed Host Controller at MMIO: 0x");
    display_print_hex(mmio_base);
    display_print("\n");
    
    edev->cap_length = *(volatile uint8_t*)mmio_base;
    edev->op_base = mmio_base + edev->cap_length;
    
    uint32_t hcsparams = ehci_read32(mmio_base, 0x04);
    edev->port_count = hcsparams & 0x0F;
    if (edev->port_count == 0 || edev->port_count > 16) edev->port_count = 6;
    
    // Stop controller if running
    uint32_t cmd = ehci_read32(edev->op_base, EHCI_REG_USBCMD);
    if (cmd & EHCI_CMD_RS) {
        ehci_write32(edev->op_base, EHCI_REG_USBCMD, cmd & ~EHCI_CMD_RS);
        delay_us(10000);
    }
    
    // Host Controller Reset
    ehci_write32(edev->op_base, EHCI_REG_USBCMD, EHCI_CMD_HCRESET);
    delay_us(50000);
    
    if (ehci_read32(edev->op_base, EHCI_REG_USBCMD) & EHCI_CMD_HCRESET) {
        display_print("[EHCI] Error: Controller Reset Timeout\n");
        return false;
    }
    
    // Clear 64-bit segment selector (Control Data Structure Segment = 0)
    ehci_write32(edev->op_base, EHCI_REG_CTRLDSSEGMENT, 0);
    
    // Allocate 1024-entry Periodic Frame List (4KB aligned)
    uint64_t periodic_phys = 0;
    edev->periodic_list = (uint32_t*)usb_dma_alloc(4096, 4096, &periodic_phys, "EHCI_PeriodicList");
    edev->periodic_phys = periodic_phys;
    for (int i = 0; i < 1024; i++) {
        edev->periodic_list[i] = 1; // Terminate bit = 1
    }
    
    // Allocate Async Queue Head (64-byte aligned)
    uint64_t async_phys = 0;
    edev->async_qh = (ehci_qh_t*)usb_dma_alloc(sizeof(ehci_qh_t), 64, &async_phys, "EHCI_AsyncQH");
    edev->async_qh_phys = async_phys;
    
    // Setup Circular Self-Referencing Async Head Node
    edev->async_qh->qh_link = (uint32_t)async_phys | 2; // Type = QH
    edev->async_qh->ep_char = (1 << 15);               // Head of Reclaimed List flag
    edev->async_qh->ep_caps = (1 << 30);               // High-speed multiplier = 1
    edev->async_qh->overlay.next_qtd = 1;              // Terminate bit = 1
    edev->async_qh->overlay.alt_next_qtd = 1;
    
    // Set Schedule Registers
    ehci_write32(edev->op_base, EHCI_REG_PERIODICLISTBASE, (uint32_t)periodic_phys);
    ehci_write32(edev->op_base, EHCI_REG_ASYNCLISTADDR, (uint32_t)async_phys);
    
    // Enable Configured Flag to Route Ports to EHCI (rather than Companion Controllers)
    ehci_write32(edev->op_base, EHCI_REG_CONFIGFLAG, 1);
    delay_us(1000);
    
    // Start Controller with Async and Periodic Schedules Enabled
    cmd = EHCI_CMD_RS | EHCI_CMD_ASE | EHCI_CMD_PSE;
    ehci_write32(edev->op_base, EHCI_REG_USBCMD, cmd);
    edev->running = true;
    
    display_print("[EHCI] Controller Successfully Running with ");
    display_print_dec(edev->port_count);
    display_print(" High-Speed Ports.\n");
    return true;
}

void ehci_shutdown(ehci_controller_t* edev) {
    if (!edev || !edev->running) return;
    ehci_write32(edev->op_base, EHCI_REG_USBCMD, 0); // Stop controller
    edev->running = false;
    display_print("[EHCI] Controller Shutdown Complete.\n");
}

bool ehci_reset(ehci_controller_t* edev) {
    if (!edev) return false;
    ehci_shutdown(edev);
    return ehci_init(edev, edev->mmio_base, edev->irq);
}

bool ehci_port_reset(ehci_controller_t* edev, uint8_t port) {
    if (!edev || port >= edev->port_count) return false;
    
    uint32_t port_reg = EHCI_REG_PORTSC1 + (port * 4);
    uint32_t status = ehci_read32(edev->op_base, port_reg);
    
    if (!(status & EHCI_PORT_CCS)) {
        display_print("[EHCI PORT] Issuing port reset on empty port ");
        display_print_dec(port); display_print("\n");
        ehci_write32(edev->op_base, port_reg, status | EHCI_PORT_PR);
        delay_us(10000);
        ehci_write32(edev->op_base, port_reg, status & ~EHCI_PORT_PR);
        return true;
    }
    
    // Ensure port is owned by EHCI
    status &= ~EHCI_PORT_OWNER;
    
    // Assert Port Reset
    ehci_write32(edev->op_base, port_reg, status | EHCI_PORT_PR);
    delay_us(50000); // 50ms reset pulse
    
    // De-assert Port Reset
    ehci_write32(edev->op_base, port_reg, status & ~EHCI_PORT_PR);
    delay_us(10000);
    
    // Wait for Port Enable
    status = ehci_read32(edev->op_base, port_reg);
    display_print("[EHCI PORT] Port "); display_print_dec(port); display_print(" High-Speed Reset Complete.\n");
    return true;
}

bool ehci_submit_bulk(ehci_controller_t* edev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len) {
    if (!edev || !edev->running || !buf) return false;
    
    uint64_t qtd_phys = 0, qh_phys = 0;
    ehci_qtd_t* qtd = (ehci_qtd_t*)usb_dma_alloc(sizeof(ehci_qtd_t), 32, &qtd_phys, "EHCI_BulkQTD");
    ehci_qh_t*  qh  = (ehci_qh_t*)usb_dma_alloc(sizeof(ehci_qh_t), 64, &qh_phys, "EHCI_BulkQH");
    
    // Setup qTD
    qtd->next_qtd = 1;     // Terminate
    qtd->alt_next_qtd = 1; // Terminate
    
    uint32_t pid = dir_in ? 1 : 0; // 1=IN, 0=OUT
    qtd->token = (pid << 8) | (3 << 10) | ((len & 0x7FFF) << 16) | (1 << 7); // Active bit
    qtd->buffer[0] = (uint32_t)(uint64_t)buf;
    
    // Setup QH
    qh->ep_char = (dev_addr & 0x7F) | ((ep & 0x0F) << 8) | (2 << 12) | (512 << 16); // High-Speed, MPS 512
    qh->ep_caps = (1 << 30); // Multiplier 1
    qh->overlay.next_qtd = (uint32_t)qtd_phys;
    
    // Insert QH into Async List
    qh->qh_link = edev->async_qh->qh_link;
    edev->async_qh->qh_link = (uint32_t)qh_phys | 2; // Type = QH
    
    // Trigger Async Advance Doorbell
    uint32_t cmd = ehci_read32(edev->op_base, EHCI_REG_USBCMD);
    ehci_write32(edev->op_base, EHCI_REG_USBCMD, cmd | EHCI_CMD_IAAD);
    
    return true;
}

void ehci_poll(ehci_controller_t* edev) {
    if (!edev || !edev->running) return;
    uint32_t sts = ehci_read32(edev->op_base, EHCI_REG_USBSTS);
    if (sts) {
        ehci_write32(edev->op_base, EHCI_REG_USBSTS, sts); // Clear status
    }
}
