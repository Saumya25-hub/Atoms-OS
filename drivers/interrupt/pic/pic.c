#include "drivers/interrupt/pic/pic.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/drivers/display/display.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI   0x20

#define ICW1_ICW4       0x01
#define ICW1_SINGLE     0x02
#define ICW1_INTERVAL4  0x04
#define ICW1_LEVEL      0x08
#define ICW1_INIT       0x10

#define ICW4_8086       0x01
#define ICW4_AUTO       0x02
#define ICW4_BUF_SLAVE  0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM       0x10

static void io_wait(void) {
    // Port 0x80 is used for 'checkpoints' during POST.
    // Linux uses this port to wait a very short amount of time.
    io_out8(0x80, 0);
}

void pic_init(void) {
    // Save current masks
    uint8_t a1 = io_in8(PIC1_DATA);
    uint8_t a2 = io_in8(PIC2_DATA);

    // Start initialization sequence in cascade mode
    io_out8(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    io_out8(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Master PIC vector offset (32)
    io_out8(PIC1_DATA, 0x20);
    io_wait();
    // ICW2: Slave PIC vector offset (40)
    io_out8(PIC2_DATA, 0x28);
    io_wait();

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2
    io_out8(PIC1_DATA, 4);
    io_wait();
    // ICW3: Tell Slave PIC its cascade identity
    io_out8(PIC2_DATA, 2);
    io_wait();

    // ICW4: Use 8086 mode
    io_out8(PIC1_DATA, ICW4_8086);
    io_wait();
    io_out8(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks (or set all to masked initially)
    // For now, we will restore the saved masks, though they are usually 0xFF or BIOS defaults.
    // Since we don't want hardware IRQs firing yet before handlers are set, we will mask all.
    io_out8(PIC1_DATA, 0xFF);
    io_out8(PIC2_DATA, 0xFF);

    // CRITICAL: Unmask IRQ2 (Cascade Line) on the Master PIC.
    // If this is masked, NO interrupts from the Slave PIC (IRQ8-15) will ever reach the CPU!
    pic_clear_mask(2);
    
    display_print("[DIAG] PIC Cascade Line (IRQ2) Unmasked\n");
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        io_out8(PIC2_CMD, PIC_EOI);
    }
    io_out8(PIC1_CMD, PIC_EOI);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = io_in8(port) | (1 << irq);
    io_out8(port, value);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = io_in8(port) & ~(1 << irq);
    io_out8(port, value);
}
