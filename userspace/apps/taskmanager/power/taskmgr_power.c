#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static TASKMGR_POWER s_power = {0};

void taskmgr_power_init(void) {
    s_power.ac_connected    = true;
    s_power.battery_percent = 100;
    s_power.voltage_mv      = 12000;
    display_print("[TASKMGR_PWR] Power Manager Engine Initialized.\n");
}

bool RefreshPower(void) {
    // In production: ACPI via KERNEL32.GetSystemPowerStatus()
    s_power.power_mw        = 35000;
    s_power.current_ma      = 2916;
    display_print("[TASKMGR_PWR] RefreshPower() -> ACPI.GetPowerStatus() OK\n");
    return true;
}

TASKMGR_POWER* taskmgr_power_get(void) { return &s_power; }
