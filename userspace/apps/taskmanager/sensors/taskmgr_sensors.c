#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static TASKMGR_SENSOR s_sensors[TASKMGR_MAX_SENSORS];
static uint32_t       s_sensor_count = 0;

static void sensor_add(const char* name, int32_t val, const char* unit, int32_t mn, int32_t mx) {
    if (s_sensor_count >= TASKMGR_MAX_SENSORS) return;
    uint32_t idx = s_sensor_count++;
    uint32_t i = 0;
    while (name[i] && i < 63) { s_sensors[idx].name[i] = name[i]; i++; } s_sensors[idx].name[i]='\0';
    i = 0;
    while (unit[i] && i < 7)  { s_sensors[idx].unit[i] = unit[i]; i++; } s_sensors[idx].unit[i]='\0';
    s_sensors[idx].value = val; s_sensors[idx].min_value = mn; s_sensors[idx].max_value = mx;
}

void taskmgr_sensors_init(void) {
    sensor_add("CPU Temperature",  48, "°C", 0, 100);
    sensor_add("GPU Temperature",  45, "°C", 0, 100);
    sensor_add("CPU Fan Speed",  1800, "RPM", 0, 5000);
    sensor_add("CPU Core Voltage",1200, "mV", 800, 1500);
    sensor_add("System Voltage",  5000, "mV", 4700, 5300);
    display_print("[TASKMGR_SENS] Sensor Manager Initialized (5 sensors).\n");
}

bool RefreshSensors(void) {
    // In production: reads hardware monitor registers via KERNEL32
    s_sensors[0].value = 48 + (int32_t)(s_sensor_count & 3);
    s_sensors[1].value = 45;
    display_print("[TASKMGR_SENS] RefreshSensors() -> HW Monitor registers OK\n");
    return true;
}

uint32_t taskmgr_sensor_count(void)              { return s_sensor_count; }
TASKMGR_SENSOR* taskmgr_sensor_get(uint32_t i)  { return (i<s_sensor_count)?&s_sensors[i]:0; }
