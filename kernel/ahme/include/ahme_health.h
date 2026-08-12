#ifndef ATOMS_AHME_HEALTH_H
#define ATOMS_AHME_HEALTH_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AHME_HEALTH_OPTIMAL = 0,
    AHME_HEALTH_DEGRADED,
    AHME_HEALTH_FAILED
} AHMEHealthStatus;

typedef struct {
    uint32_t driver_id;
    const char *driver_name;
    uint32_t timeout_count;
    uint32_t error_count;
    uint32_t success_count;
    AHMEHealthStatus status;
} AHMEDriverHealthRecord;

void ahme_health_register_driver(uint32_t driver_id, const char *name);
void ahme_health_report_event(uint32_t driver_id, bool success, bool is_timeout);
AHMEHealthStatus ahme_health_get_status(uint32_t driver_id);

#endif // ATOMS_AHME_HEALTH_H
