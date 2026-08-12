/* kernel/net/drivers/realtek/realtek_master.c - Realtek Multi-Generation PCI Master Engine */
#include <kernel/net/drivers/realtek/realtek_master.h>
#include <kernel/net/net_framework.h>
#include <kernel/core/pci/pci.h>
#include "kernel/drivers/display/display.h"

extern bool rtl8168_driver_probe(PCIDevice* pdev);
extern bool rtl8111_driver_probe(PCIDevice* pdev);
extern bool rtl8125_driver_probe(PCIDevice* pdev);

bool realtek_master_probe(PCIDevice* pdev) {
    if (!pdev || pdev->vendor_id != REALTEK_VENDOR_ID) return false;

    display_print("[REALTEK MASTER] Realtek Controller Detected! PCI ID 0x10EC:0x");
    display_print_hex(pdev->device_id);
    display_print(" Rev 0x");
    display_print_hex(pdev->revision_id);
    display_print("\n");
    display_print("[REALTEK MASTER] [ISOLATED B760M-K MODE] Forcing RTL8125 2.5GbE Engine Exclusive Test...\n");

    return rtl8168_driver_probe(pdev);
}

void realtek_master_init(void) {
    display_print("[REALTEK MASTER] Initializing Realtek Multi-Family Engine...\n");
}
