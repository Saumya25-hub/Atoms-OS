/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_types.h — Core Data Types, Status Codes, Capability Flags & Definitions
 */

#ifndef BOS_SANDBOX_TYPES_H
#define BOS_SANDBOX_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOS_MAX_SANDBOX_CONTEXTS 64
#define BOS_MAX_PATH_LENGTH      256
#define BOS_MAX_AUDIT_LOGS       128
#define BOS_SANDBOX_GUARD_SIZE   4096

/* ============================================================
 * Structured Sandbox Status Codes
 * ============================================================ */
typedef enum {
    BOS_SANDBOX_OK                          = 0,
    BOS_SANDBOX_ERR_INVALID_PARAM           = -1,
    BOS_SANDBOX_ERR_NOT_INITIALIZED         = -2,
    BOS_SANDBOX_ERR_ALREADY_INITIALIZED      = -3,
    BOS_SANDBOX_ERR_NO_MEMORY               = -4,
    BOS_SANDBOX_ERR_CONTEXT_NOT_FOUND       = -5,
    BOS_SANDBOX_ERR_TOKEN_FORGED            = -6,
    BOS_SANDBOX_ERR_CAPABILITY_DENIED       = -7,
    BOS_SANDBOX_ERR_PERMISSION_DENIED       = -8,
    BOS_SANDBOX_ERR_MEMORY_VIOLATION        = -9,
    BOS_SANDBOX_ERR_SYSCALL_BLOCKED         = -10,
    BOS_SANDBOX_ERR_IPC_DENIED              = -11,
    BOS_SANDBOX_ERR_PATH_DENIED             = -12,
    BOS_SANDBOX_ERR_NETWORK_DENIED          = -13,
    BOS_SANDBOX_ERR_RESOURCE_EXHAUSTED      = -14,
    BOS_SANDBOX_ERR_CRASH_ISOLATED          = -15,
    BOS_SANDBOX_ERR_GUARD_PAGE_VIOLATION    = -16,
    BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION    = -17
} bos_sandbox_status_t;

#define BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION BOS_SANDBOX_ERR_RESOURCE_EXHAUSTED

/* ============================================================
 * Fine-Grained Capability Flags (64-bit Bitfield)
 * ============================================================ */
#define BOS_CAP_NONE            (0ULL)
#define BOS_CAP_READ_FILES      (1ULL << 0)
#define BOS_CAP_WRITE_FILES     (1ULL << 1)
#define BOS_CAP_DELETE_FILES    (1ULL << 2)
#define BOS_CAP_NETWORK         (1ULL << 3)
#define BOS_CAP_CLIPBOARD       (1ULL << 4)
#define BOS_CAP_AUDIO           (1ULL << 5)
#define BOS_CAP_CAMERA          (1ULL << 6)
#define BOS_CAP_MICROPHONE      (1ULL << 7)
#define BOS_CAP_PRINTER         (1ULL << 8)
#define BOS_CAP_NOTIFICATIONS   (1ULL << 9)
#define BOS_CAP_DOWNLOADS       (1ULL << 10)
#define BOS_CAP_TEMP_STORAGE    (1ULL << 11)
#define BOS_CAP_SYS_SETTINGS    (1ULL << 12)
#define BOS_CAP_POWER_MGMT      (1ULL << 13)
#define BOS_CAP_USB             (1ULL << 14)
#define BOS_CAP_BLUETOOTH       (1ULL << 15)
#define BOS_CAP_LOCATION        (1ULL << 16)
#define BOS_CAP_ALL             (0xFFFFFFFFFFFFFFFFULL)

/* ============================================================
 * Central Permission Operations
 * ============================================================ */
typedef enum {
    BOS_PERM_OPEN_FILE          = 1,
    BOS_PERM_DELETE_FILE        = 2,
    BOS_PERM_CONNECT_NETWORK    = 3,
    BOS_PERM_CREATE_SHM         = 4,
    BOS_PERM_ACCESS_CLIPBOARD   = 5,
    BOS_PERM_OPEN_CAMERA        = 6,
    BOS_PERM_ACCESS_MICROPHONE  = 7,
    BOS_PERM_SPAWN_PROCESS      = 8,
    BOS_PERM_CREATE_PIPE        = 9,
    BOS_PERM_MAP_MEMORY         = 10
} bos_permission_op_t;

/* ============================================================
 * Security Token Definition
 * ============================================================ */
typedef struct {
    uint64_t token_id;
    uint32_t owner_pid;
    uint64_t creation_time;
    uint64_t signature;
    bool     is_valid;
} bos_token_t;

/* ============================================================
 * Resource Limit & Usage Engine Structures
 * ============================================================ */
typedef struct {
    uint32_t cpu_max_percent;
    uint64_t ram_max_bytes;
    uint32_t max_threads;
    uint32_t max_handles;
    uint32_t max_ipc_objects;
    uint64_t max_shm_bytes;
    uint32_t max_open_files;
    uint32_t max_net_conns;
} bos_resource_limits_t;

typedef struct {
    uint32_t cpu_current_percent;
    uint64_t ram_current_bytes;
    uint32_t current_threads;
    uint32_t current_handles;
    uint32_t current_ipc_objects;
    uint64_t current_shm_bytes;
    uint32_t current_open_files;
    uint32_t current_net_conns;
    uint32_t total_allocations;
    uint32_t total_frees;
} bos_resource_usage_t;

/* ============================================================
 * Master Process Sandbox Context
 * ============================================================ */
typedef struct {
    uint32_t              context_id;
    uint32_t              pid;
    bos_token_t           token;
    uint64_t              capabilities;
    bos_resource_limits_t limits;
    bos_resource_usage_t  usage;
    uint64_t              page_table_base;
    uint64_t              virt_mem_start;
    uint64_t              virt_mem_end;
    char                  allowed_fs_prefix[BOS_MAX_PATH_LENGTH];
    bool                  allow_raw_sockets;
    bool                  active;
    bool                  crashed;
} bos_sandbox_context_t;

/* ============================================================
 * Structured Security Audit Record
 * ============================================================ */
typedef enum {
    BOS_AUDIT_PERMISSION_DENIED  = 1,
    BOS_AUDIT_INVALID_SYSCALL    = 2,
    BOS_AUDIT_CAPABILITY_VIOLATION = 3,
    BOS_AUDIT_ESCAPE_ATTEMPT     = 4,
    BOS_AUDIT_MEMORY_VIOLATION   = 5,
    BOS_AUDIT_UNAUTHORIZED_IPC   = 6,
    BOS_AUDIT_RESOURCE_EXHAUSTION= 7,
    BOS_AUDIT_PROCESS_CRASH      = 8
} bos_audit_event_type_t;

typedef struct {
    uint64_t               timestamp;
    uint32_t               context_id;
    uint32_t               pid;
    bos_audit_event_type_t event_type;
    char                   description[128];
    int                    result_code;
} bos_audit_record_t;

/* ============================================================
 * Debug & Tracing Flags
 * ============================================================ */
#define BOS_TRACE_SANDBOX       (1ULL << 0)
#define BOS_TRACE_PERMISSION    (1ULL << 1)
#define BOS_TRACE_CAPABILITY    (1ULL << 2)
#define BOS_TRACE_MEMORY        (1ULL << 3)
#define BOS_TRACE_IPC           (1ULL << 4)
#define BOS_TRACE_SYSCALL       (1ULL << 5)
#define BOS_TRACE_PERF          (1ULL << 6)
#define BOS_TRACE_ALL           (0xFFFFFFFFFFFFFFFFULL)

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_TYPES_H */
