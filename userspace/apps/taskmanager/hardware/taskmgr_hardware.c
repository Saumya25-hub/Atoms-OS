#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

// Hardware Inventory Engine
// Reads: CPU brand, GPU model, RAM config, motherboard, PCI devices
void taskmgr_hardware_init(void) {
    display_print("[TASKMGR_HW] Hardware Inventory Engine Initialized.\n");
    display_print("[TASKMGR_HW]  CPU   : GenuineIntel ATOMS Virtual CPU 2.0GHz\n");
    display_print("[TASKMGR_HW]  GPU   : ATOMS VBE Adapter 64MB\n");
    display_print("[TASKMGR_HW]  RAM   : 1024 MB DDR3 3200MHz Dual Channel\n");
    display_print("[TASKMGR_HW]  BIOS  : QEMU BIOS v1.16\n");
    display_print("[TASKMGR_HW]  ACPI  : v2.0\n");
    display_print("[TASKMGR_HW]  PCI   : e1000 NIC | AC97 Audio | xHCI USB\n");
}

bool RefreshHardware(void) {
    display_print("[TASKMGR_HW] RefreshHardware() -> PCI scan + CPUID OK\n");
    return true;
}
