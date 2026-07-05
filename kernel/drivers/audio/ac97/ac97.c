#include "ac97.h"
#include "ac97_codec.h"
#include "ac97_dma.h"
#include "ac97_playback.h"
#include "../../../audio/audio_player.h"
#include "../../../../arch/x86_64/io/port_io.h"
#include "../../display/display.h"

// PCI Config Space Access
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(PCI_CONFIG_ADDRESS, address);
    return io_in32(PCI_CONFIG_DATA);
}

static void pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(PCI_CONFIG_ADDRESS, address);
    
    // Read current 32-bit value
    uint32_t current = io_in32(PCI_CONFIG_DATA);
    
    // Overwrite the specific 16 bits
    if ((offset & 2) == 0) {
        current = (current & 0xFFFF0000) | value;
    } else {
        current = (current & 0x0000FFFF) | ((uint32_t)value << 16);
    }
    
    io_out32(PCI_CONFIG_DATA, current);
}

void ac97_init(void) {
    uint8_t target_bus = 0;
    uint8_t target_slot = 0;
    bool found = false;
    uint32_t vendor_device = 0;
    
    // Simple PCI brute-force scan
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vd = pci_read_config((uint8_t)bus, slot, 0, 0); // Vendor & Device ID
            if (vd != 0xFFFFFFFF) {
                uint32_t class_code = pci_read_config((uint8_t)bus, slot, 0, 0x08);
                uint8_t base_class = (class_code >> 24) & 0xFF;
                uint8_t sub_class = (class_code >> 16) & 0xFF;
                
                // Base Class 0x04 (Multimedia), Sub Class 0x01 (Audio Controller)
                if (base_class == 0x04 && sub_class == 0x01) {
                    target_bus = (uint8_t)bus;
                    target_slot = slot;
                    vendor_device = vd;
                    found = true;
                    break;
                }
            }
        }
        if (found) break;
    }
    
    if (!found) {
        return;
    }
    
    // Read BAR0 (NAM) and BAR1 (NABM)
    uint32_t bar0 = pci_read_config(target_bus, target_slot, 0, 0x10);
    uint32_t bar1 = pci_read_config(target_bus, target_slot, 0, 0x14);
    
    // Ensure they are I/O mapped (bit 0 = 1)
    if (!(bar0 & 1) || !(bar1 & 1)) {
        display_print("[AC97] FAILED: BARs are not I/O mapped. AC97 Requires I/O BARs.\n");
        return;
    }
    
    uint16_t nam_bar = (uint16_t)(bar0 & ~3);
    uint16_t nabm_bar = (uint16_t)(bar1 & ~3);
    
    // Read IRQ (Line and Pin)
    uint32_t irq_reg = pci_read_config(target_bus, target_slot, 0, 0x3C);
    uint8_t irq_line = irq_reg & 0xFF;
    uint8_t irq_pin = (irq_reg >> 8) & 0xFF;
    
    // Enable Bus Mastering and I/O Space (Command Register 0x04)
    uint32_t cmd = pci_read_config(target_bus, target_slot, 0, 0x04);
    pci_write_config_16(target_bus, target_slot, 0, 0x04, (uint16_t)((cmd & 0xFFFF) | 0x0005));
    
    ac97_codec_init_base(nam_bar, nabm_bar);
    
    if (!ac97_codec_verify_and_configure()) {
        return;
    }
    
    ac97_dma_init(nabm_bar);
}
