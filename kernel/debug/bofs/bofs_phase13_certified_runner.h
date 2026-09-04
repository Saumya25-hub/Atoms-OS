#ifndef BOFS_PHASE13_CERTIFIED_RUNNER_H
#define BOFS_PHASE13_CERTIFIED_RUNNER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"
#include "kernel/vfs/bofs/include/bofs_format.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Evidential Classification */
typedef enum {
    P13_EVID_PROVEN = 0,
    P13_EVID_OBSERVED,
    P13_EVID_DERIVED,
    P13_EVID_INFERRED,
    P13_EVID_UNKNOWN,
    P13_EVID_NOT_TESTED
} p13_evidence_class_t;

/* Test State */
typedef enum {
    P13_TEST_PENDING = 0,
    P13_TEST_RUNNING,
    P13_TEST_PASS,
    P13_TEST_FAIL,
    P13_TEST_BLOCKED,
    P13_TEST_NOT_TESTED
} p13_test_status_t;

/* Target Safety Gate Classification */
typedef enum {
    P13_TARGET_UNKNOWN = 0,
    P13_TARGET_FOREIGN_NTFS,
    P13_TARGET_FOREIGN_ESP,
    P13_TARGET_FOREIGN_MSR,
    P13_TARGET_FOREIGN_RECOVERY,
    P13_TARGET_DEDICATED_BOFS,
    P13_TARGET_MOCK_DEDICATED
} p13_target_type_t;

/* Safety Gate State */
typedef struct {
    char                device_name[32];
    char                model[40];
    char                serial[24];
    uint64_t            capacity_bytes;
    uint32_t            sector_size;
    int                 partition_index;
    uint64_t            start_lba;
    uint64_t            sector_count;
    p13_target_type_t   target_type;
    bool                foreign_storage_locked;
    uint64_t            foreign_writes_attempted;
    uint64_t            foreign_bytes_written;
    bool                human_authorized;
    bool                format_permitted;
} bofs_safety_gate_t;

/* Individual Test Result */
typedef struct {
    char                test_id[8];
    char                description[48];
    p13_test_status_t   status;
    p13_evidence_class_t classification;
    char                evidence[48];
} bofs_p13_test_item_t;

#define BOFS_P13_TOTAL_TESTS 53

/* Write Ledger Entry */
typedef struct {
    uint64_t timestamp_ms;
    uint64_t lba;
    uint32_t count;
    char     purpose[24];
    uint32_t transaction_id;
} bofs_write_ledger_entry_t;

#define BOFS_WRITE_LEDGER_MAX 64

/* Master Runner Entrypoint */
void bofs_phase13_certified_runner_run(boot_info_t* boot_info);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_PHASE13_CERTIFIED_RUNNER_H */
