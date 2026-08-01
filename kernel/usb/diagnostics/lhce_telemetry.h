#ifndef SIGNATURES_LHCE_TELEMETRY_H
#define SIGNATURES_LHCE_TELEMETRY_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    uint64_t total_transfers_submitted;
    uint64_t total_transfers_completed;
    uint64_t total_bytes_in;
    uint64_t total_bytes_out;
    uint64_t total_timeouts;
    uint64_t total_resets;
    uint64_t total_interrupts;
    uint32_t active_controllers;
    uint32_t active_ports;
    uint64_t peak_throughput_bps;
    atoms_spinlock_t lock;
} lhce_telemetry_t;

// Telemetry APIs
void lhce_telemetry_init(void);
void lhce_telemetry_record_submit(size_t len, bool is_in);
void lhce_telemetry_record_complete(size_t len, bool is_in);
void lhce_telemetry_record_timeout(void);
void lhce_telemetry_record_reset(void);
void lhce_telemetry_record_interrupt(void);

// Required Production Telemetry & Forensic APIs
void usb_dump_controller(uint32_t index);
void usb_dump_ports(void);
void usb_dump_scheduler(void);
void usb_dump_transfer(uint32_t req_id);
void usb_dump_dma(void);
void usb_dump_everything(void);

#endif // SIGNATURES_LHCE_TELEMETRY_H
