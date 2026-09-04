#ifndef BOFS_FORENSIC_DASHBOARD_H
#define BOFS_FORENSIC_DASHBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Evidential Classification Categories (Section 1 & 62) */
typedef enum {
    EVID_OBSERVED   = 0, /* Directly read from hardware register, memory, or disk */
    EVID_DERIVED    = 1, /* Computed/calculated from primary observed evidence */
    EVID_PROVEN     = 2, /* Formally verified through diagnostic assertions */
    EVID_INFERRED   = 3, /* Plausible correlation without direct hardware proof */
    EVID_UNKNOWN    = 4, /* Unverified or probe failed */
    EVID_NOT_TESTED = 5  /* Deliberately excluded or reserved for future phase */
} bofs_evidence_class_t;

/* Cross-Layer Event Timeline Item (Section 41) */
#define BOFS_TIMELINE_MAX_EVENTS 16
#define BOFS_TIMELINE_STR_LEN    32

typedef struct {
    uint64_t timestamp_ms;
    char     layer[BOFS_TIMELINE_STR_LEN];
    char     action[BOFS_TIMELINE_STR_LEN];
    char     result[BOFS_TIMELINE_STR_LEN];
    bool     success;
    bofs_evidence_class_t classification;
} bofs_timeline_event_t;

/* In-Kernel Forensic Snapshot Structure (Section 44) */
typedef struct {
    uint64_t magic;
    uint32_t cpu_cores;
    char     cpu_brand[64];
    uint64_t total_ram_mb;
    uint32_t block_devices_detected;
    uint32_t partitions_detected;
    uint32_t bofs_superblock_magic;
    uint32_t bofs_version;
    uint64_t bofs_total_blocks;
    uint64_t bofs_allocated_blocks;
    uint32_t bofs_active_inodes;
    uint32_t bofs_tree_depth;
    bool     bofs_wal_active;
    uint32_t vfs_mount_count;
    uint32_t open_fd_count;
    bool     first_failure_detected;
    char     first_failure_layer[32];
    char     first_failure_cause[64];
    uint32_t timeline_count;
    bofs_timeline_event_t timeline[BOFS_TIMELINE_MAX_EVENTS];
} bofs_forensic_snapshot_t;

/* Master Entry Point for Phase 12 Forensic Dashboard */
void bofs_forensic_dashboard_run(boot_info_t* boot_info);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_FORENSIC_DASHBOARD_H */
