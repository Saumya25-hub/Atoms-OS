#ifndef ROOK_DASHBOARD_H
#define ROOK_DASHBOARD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ♜ ROOK V2 CERTIFICATION DASHBOARD (PHASE 0B LIVE TELEMETRY)
 * Dedicated real-time diagnostic gatekeeper between Boot Splash and Login V2.
 */

#define ROOK_SEV_INFO 0
#define ROOK_SEV_WARN 1
#define ROOK_SEV_FAIL 2

struct rook_page;
struct rook_page* rook_page_dashboard_get(void);
void              rook_dashboard_spin(uint32_t total_ms);

/* Flight Recorder Event Logging API (Static Memory, Zero-Heap) */
void              rook_flight_record(const char* subsystem, const char* message, uint8_t severity);

#endif /* ROOK_DASHBOARD_H */
