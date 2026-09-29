/*
 * ATOMS OS — SNACK FORENSIC TELEMETRY SYSTEM HEADER
 * Hierarchical Distributed Deep Forensic Debugger (Patient-Side Snack Agents)
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_SNACK_H
#define ATOMS_SNACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNACK_MAGIC                 "ATSNACK"
#define SNACK_PROTOCOL_VERSION      1
#define SNACK_MAX_PAYLOAD           1024
#define SNACK_UDP_TARGET_PORT       9999
#define SNACK_SERVER_IP             0xC0A80201  /* 192.168.2.1 */
#define SNACK_BROADCAST_IP          0xFFFFFFFF  /* 255.255.255.255 (Global Broadcast - zero ARP latency) */

/* 10 Specialized Forensic Layers */
typedef enum {
    SNACK_LAYER_CPU = 1,
    SNACK_LAYER_PCI = 2,
    SNACK_LAYER_ACPI = 3,
    SNACK_LAYER_MEMORY = 4,
    SNACK_LAYER_HYPERVISOR = 5,
    SNACK_LAYER_VIRTIO = 6,
    SNACK_LAYER_VTNET = 7,
    SNACK_LAYER_GUEST = 8,
    SNACK_LAYER_NETWORK = 9,
    SNACK_LAYER_STORAGE = 10
} SnackLayer;

/* Snack Agent Lifecycle States */
typedef enum {
    SNACK_STATE_IDLE = 0,
    SNACK_STATE_CREATED,
    SNACK_STATE_DISPATCHED,
    SNACK_STATE_RUNNING,
    SNACK_STATE_COLLECTING,
    SNACK_STATE_REPORTING,
    SNACK_STATE_COMPLETE,
    SNACK_STATE_FAILED,
    SNACK_STATE_TIMEOUT,
    SNACK_STATE_CANCELLED
} SnackState;

/* Event Severity */
typedef enum {
    SNACK_SEV_INFO = 1,
    SNACK_SEV_PASS = 2,
    SNACK_SEV_WARN = 3,
    SNACK_SEV_FAIL = 4,
    SNACK_SEV_CRITICAL = 5
} SnackSeverity;

/* Snack Agent Descriptor */
typedef struct {
    uint32_t snack_id;
    SnackLayer layer;
    SnackState state;
    const char *name;
    uint32_t events_recorded;
    uint64_t start_tick;
    uint64_t end_tick;
} SnackAgent;

/* Snack Job Lifecycle States */
typedef enum {
    SNACK_JOB_IDLE = 0,
    SNACK_JOB_QUEUED,
    SNACK_JOB_RUNNING,
    SNACK_JOB_COMPLETE,
    SNACK_JOB_FAILED,
    SNACK_JOB_CANCELLED
} SnackJobState;

/* Core Snack API */
void atoms_snack_init(void);
void atoms_snack_dispatch_command(const char *cmd);
bool atoms_snack_is_busy(void);
bool atoms_snack_step(void);
SnackJobState atoms_snack_get_job_state(void);

/* Individual Snack Runners */
void atoms_snack_run_cpu(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_pci(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_acpi(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_memory(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_hypervisor(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_virtio(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_vtnet(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_guest(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_network(uint32_t session_id, uint32_t job_id);
void atoms_snack_run_storage(uint32_t session_id, uint32_t job_id);

/* Composite Forensic Jobs */
void atoms_snack_run_job_vtnet_attach(uint32_t session_id);
void atoms_snack_run_deep_probe(uint32_t session_id);

/* Trace Targets */
void atoms_snack_trace_device(const char *bdf_str);
void atoms_snack_trace_bar(uint16_t bar_port);
void atoms_snack_trace_queue(uint32_t q_idx);
void atoms_snack_trace_vmexit(void);

/* Emission Utilities */
void atoms_snack_emit_packet(uint32_t session_id, uint32_t snack_id, uint32_t job_id,
                             SnackLayer layer, const char *event_type, SnackSeverity sev,
                             const char *key, const char *val);

void atoms_snack_emit_hex(uint32_t session_id, uint32_t snack_id, uint32_t job_id,
                          SnackLayer layer, const char *event_type, SnackSeverity sev,
                          const char *key, uint64_t val, int nibbles);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_SNACK_H */
