#include "kernel/ahme/include/ahme_health.h"
#include <stddef.h>

#define AHME_MAX_DRIVERS 16

static AHMEDriverHealthRecord s_health_records[AHME_MAX_DRIVERS];
static uint32_t s_driver_count = 0;

void ahme_health_register_driver(uint32_t driver_id, const char *name) {
    if (s_driver_count >= AHME_MAX_DRIVERS) return;
    s_health_records[s_driver_count].driver_id = driver_id;
    s_health_records[s_driver_count].driver_name = name;
    s_health_records[s_driver_count].timeout_count = 0;
    s_health_records[s_driver_count].error_count = 0;
    s_health_records[s_driver_count].success_count = 0;
    s_health_records[s_driver_count].status = AHME_HEALTH_OPTIMAL;
    s_driver_count++;
}

void ahme_health_report_event(uint32_t driver_id, bool success, bool is_timeout) {
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (s_health_records[i].driver_id == driver_id) {
            if (success) {
                s_health_records[i].success_count++;
            } else {
                s_health_records[i].error_count++;
                if (is_timeout) {
                    s_health_records[i].timeout_count++;
                }
            }

            if (s_health_records[i].timeout_count > 3 || s_health_records[i].error_count > 10) {
                s_health_records[i].status = AHME_HEALTH_FAILED;
            } else if (s_health_records[i].error_count > 2) {
                s_health_records[i].status = AHME_HEALTH_DEGRADED;
            }
            return;
        }
    }
}

AHMEHealthStatus ahme_health_get_status(uint32_t driver_id) {
    for (uint32_t i = 0; i < s_driver_count; i++) {
        if (s_health_records[i].driver_id == driver_id) {
            return s_health_records[i].status;
        }
    }
    return AHME_HEALTH_OPTIMAL;
}
