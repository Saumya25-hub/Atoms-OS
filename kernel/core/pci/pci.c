#include "pci.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/drivers/display/display.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static PCIDevice g_pci_devices[MAX_PCI_DEVICES];
static uint32_t g_pci_device_count = 0;

static volatile uint32_t pci_lock = 0;

static inline unsigned long pci_acquire_lock(void) {
    unsigned long flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    while (__sync_lock_test_and_set(&pci_lock, 1)) {
        __asm__ volatile("pause");
    }
    return flags;
}

static inline void pci_release_lock(unsigned long flags) {
    __sync_lock_release(&pci_lock);
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory");
}

uint32_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    unsigned long flags = pci_acquire_lock();
    io_out32(PCI_CONFIG_ADDRESS, address);
    uint32_t val = io_in32(PCI_CONFIG_DATA);
    pci_release_lock(flags);
    return val;
}

uint16_t pci_read_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t val = pci_read_config_32(bus, slot, func, offset);
    return (uint16_t)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_read_config_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t val = pci_read_config_32(bus, slot, func, offset);
    return (uint8_t)((val >> ((offset & 3) * 8)) & 0xFF);
}

// Legacy wrapper
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return pci_read_config_32(bus, slot, func, offset);
}

void pci_write_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    unsigned long flags = pci_acquire_lock();
    io_out32(PCI_CONFIG_ADDRESS, address);
    io_out32(PCI_CONFIG_DATA, value);
    pci_release_lock(flags);
}

void pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    unsigned long flags = pci_acquire_lock();
    io_out32(PCI_CONFIG_ADDRESS, address);
    uint32_t current = io_in32(PCI_CONFIG_DATA);
    
    if ((offset & 2) == 0) {
        current = (current & 0xFFFF0000) | value;
    } else {
        current = (current & 0x0000FFFF) | ((uint32_t)value << 16);
    }
    io_out32(PCI_CONFIG_DATA, current);
    pci_release_lock(flags);
}

void pci_write_config_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    unsigned long flags = pci_acquire_lock();
    io_out32(PCI_CONFIG_ADDRESS, address);
    uint32_t current = io_in32(PCI_CONFIG_DATA);
    
    uint32_t shift = (offset & 3) * 8;
    uint32_t mask = ~(0xFF << shift);
    current = (current & mask) | ((uint32_t)value << shift);
    
    io_out32(PCI_CONFIG_DATA, current);
    pci_release_lock(flags);
}

void pci_enable_io_space(PCIDevice* dev) {
    if (!dev) return;
    uint16_t cmd = pci_read_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET);
    pci_write_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET, cmd | PCI_COMMAND_IO);
}

void pci_enable_memory_space(PCIDevice* dev) {
    if (!dev) return;
    uint16_t cmd = pci_read_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET);
    pci_write_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET, cmd | PCI_COMMAND_MEMORY);
}

void pci_enable_bus_mastering(PCIDevice* dev) {
    if (!dev) return;
    uint16_t cmd = pci_read_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET);
    pci_write_config_16(dev->bus, dev->slot, dev->func, PCI_COMMAND_OFFSET, cmd | PCI_COMMAND_MASTER);
}

static void pci_parse_bars(PCIDevice* dev) {
    for (int i = 0; i < 6; i++) {
        dev->bars[i].type = PCI_BAR_TYPE_NONE;
    }

    int max_bars = (dev->header_type & 0x7F) == 0x00 ? 6 : 2;
    if ((dev->header_type & 0x7F) == 0x02) max_bars = 0; // CardBus bridge
    
    for (int i = 0; i < max_bars; i++) {
        uint32_t bar_offset = 0x10 + (i * 4);
        uint32_t bar_val = pci_read_config_32(dev->bus, dev->slot, dev->func, bar_offset);
        
        if (bar_val == 0) continue;
        
        dev->bars[i].index = i;
        
        if (bar_val & 1) {
            // IO BAR
            dev->bars[i].type = PCI_BAR_TYPE_IO;
            dev->bars[i].base_address = bar_val & ~0x3ULL;
            dev->bars[i].prefetchable = false;
        } else {
            // MMIO BAR
            uint8_t mmio_type = (bar_val >> 1) & 0x3;
            dev->bars[i].prefetchable = (bar_val & 0x8) != 0;
            
            if (mmio_type == 0x00) {
                dev->bars[i].type = PCI_BAR_TYPE_MMIO32;
                dev->bars[i].base_address = bar_val & ~0xFULL;
            } else if (mmio_type == 0x02) {
                dev->bars[i].type = PCI_BAR_TYPE_MMIO64;
                dev->bars[i].base_address = bar_val & ~0xFULL;
                if (i + 1 < max_bars) {
                    uint32_t bar_high = pci_read_config_32(dev->bus, dev->slot, dev->func, bar_offset + 4);
                    dev->bars[i].base_address |= ((uint64_t)bar_high << 32);
                    i++; // consume next BAR slot
                }
            }
        }
    }
}

static void pci_probe_function(uint8_t bus, uint8_t slot, uint8_t func) {
    if (g_pci_device_count >= MAX_PCI_DEVICES) {
        display_print("[PCI] WARNING: MAX_PCI_DEVICES limit reached. Ignoring further devices.\n");
        return;
    }
    
    uint32_t vd = pci_read_config_32(bus, slot, func, 0);
    uint32_t class_code = pci_read_config_32(bus, slot, func, 0x08);
    
    PCIDevice* dev = &g_pci_devices[g_pci_device_count];
    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    
    dev->vendor_id = vd & 0xFFFF;
    dev->device_id = (vd >> 16) & 0xFFFF;
    
    dev->revision_id = class_code & 0xFF;
    dev->prog_if = (class_code >> 8) & 0xFF;
    dev->sub_class = (class_code >> 16) & 0xFF;
    dev->base_class = (class_code >> 24) & 0xFF;
    
    dev->header_type = pci_read_config_8(bus, slot, func, 0x0E);
    dev->interrupt_line = pci_read_config_8(bus, slot, func, 0x3C);
    dev->interrupt_pin = pci_read_config_8(bus, slot, func, 0x3D);
    
    pci_parse_bars(dev);
    
    g_pci_device_count++;
}

void pci_init(void) {
    g_pci_device_count = 0;
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vd = pci_read_config_32((uint8_t)bus, slot, 0, 0);
            if ((vd & 0xFFFF) != 0xFFFF) {
                pci_probe_function((uint8_t)bus, slot, 0);
                
                uint8_t header_type = pci_read_config_8((uint8_t)bus, slot, 0, 0x0E);
                if (header_type & 0x80) { // Multi-function
                    for (uint8_t func = 1; func < 8; func++) {
                        uint32_t f_vd = pci_read_config_32((uint8_t)bus, slot, func, 0);
                        if ((f_vd & 0xFFFF) != 0xFFFF) {
                            pci_probe_function((uint8_t)bus, slot, func);
                        }
                    }
                }
            }
        }
    }
    
    pci_print_diagnostics();
}

uint32_t pci_get_device_count(void) {
    return g_pci_device_count;
}

PCIDevice* pci_get_device(uint32_t index) {
    if (index >= g_pci_device_count) return NULL;
    return &g_pci_devices[index];
}

bool pci_find_by_class(uint8_t base_class, uint8_t sub_class, PCIDevice* out_device) {
    if (!out_device) return false;
    for (uint32_t i = 0; i < g_pci_device_count; i++) {
        if (g_pci_devices[i].base_class == base_class && g_pci_devices[i].sub_class == sub_class) {
            *out_device = g_pci_devices[i];
            return true;
        }
    }
    return false;
}

void pci_print_diagnostics(void) {
    extern void display_print_hex(uint64_t num);
    extern void display_print_dec(uint64_t num);
    
    display_print("\n=== PCI V2 DIAGNOSTICS ===\n");
    display_print("[PCI] Device count: ");
    display_print_dec(g_pci_device_count);
    display_print("\n");
    
    for (uint32_t i = 0; i < g_pci_device_count; i++) {
        PCIDevice* dev = &g_pci_devices[i];
        display_print("[PCI] ");
        display_print_dec(dev->bus); display_print(":");
        display_print_dec(dev->slot); display_print(".");
        display_print_dec(dev->func); display_print(" vendor=");
        display_print_hex(dev->vendor_id); display_print(" device=");
        display_print_hex(dev->device_id); display_print(" class=");
        display_print_hex(dev->base_class); display_print(" subclass=");
        display_print_hex(dev->sub_class); display_print("\n");
        
        display_print("[PCI] IRQ line="); display_print_dec(dev->interrupt_line);
        display_print(" pin="); display_print_dec(dev->interrupt_pin); display_print("\n");
        
        for (int b = 0; b < 6; b++) {
            if (dev->bars[b].type != PCI_BAR_TYPE_NONE) {
                display_print("[PCI] BAR"); display_print_dec(b);
                if (dev->bars[b].type == PCI_BAR_TYPE_IO) display_print(" IO ");
                else if (dev->bars[b].type == PCI_BAR_TYPE_MMIO32) display_print(" MMIO32 ");
                else if (dev->bars[b].type == PCI_BAR_TYPE_MMIO64) display_print(" MMIO64 ");
                display_print("base="); display_print_hex(dev->bars[b].base_address); display_print("\n");
            }
        }
    }
    display_print("==========================\n\n");
}
