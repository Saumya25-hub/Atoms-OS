#ifndef SIGNATURES_PCI_H
#define SIGNATURES_PCI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_PCI_DEVICES 128

#define PCI_COMMAND_OFFSET 0x04
#define PCI_COMMAND_IO     (1 << 0)
#define PCI_COMMAND_MEMORY (1 << 1)
#define PCI_COMMAND_MASTER (1 << 2)

typedef enum {
    PCI_BAR_TYPE_NONE = 0,
    PCI_BAR_TYPE_IO = 1,
    PCI_BAR_TYPE_MMIO32 = 2,
    PCI_BAR_TYPE_MMIO64 = 3
} PCIBarType;

typedef struct {
    uint8_t index;
    PCIBarType type;
    uint64_t base_address;
    uint64_t size;
    bool prefetchable;
} PCIBar;

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    
    uint16_t vendor_id;
    uint16_t device_id;
    
    uint8_t base_class;
    uint8_t sub_class;
    uint8_t prog_if;
    uint8_t revision_id;
    uint8_t header_type;
    
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    
    PCIBar bars[6];
} PCIDevice;

void pci_init(void);

// Centralized Access API
uint8_t pci_read_config_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_write_config_8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value);
void pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
void pci_write_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

// Legacy wrapper (now mapped to 32-bit read)
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

// Command Register API
void pci_enable_io_space(PCIDevice* dev);
void pci_enable_memory_space(PCIDevice* dev);
void pci_enable_bus_mastering(PCIDevice* dev);

uint32_t pci_get_device_count(void);
PCIDevice* pci_get_device(uint32_t index);
bool pci_find_by_class(uint8_t base_class, uint8_t sub_class, PCIDevice* out_device);

void pci_print_diagnostics(void);

#endif // SIGNATURES_PCI_H
