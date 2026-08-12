/* kernel/net/drivers/realtek/rtl8111.c - Isolated Legacy RTL8111 Gigabit Driver */
#include <kernel/net/drivers/realtek/rtl8111.h>
#include "kernel/drivers/display/display.h"

bool rtl8111_driver_probe(PCIDevice* pdev) {
    if (!pdev) return false;
    display_print("[RTL8111 DRIVER] Probing Legacy RTL8111 Family Controller...\n");
    // Delegates to certified 1G ring probe
    extern bool rtl8168_driver_probe(PCIDevice* pdev);
    return rtl8168_driver_probe(pdev);
}
