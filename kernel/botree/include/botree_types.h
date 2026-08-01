#ifndef BOTREE_TYPES_H
#define BOTREE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Max Path and Name Length Constants
#define BDE_PATH_MAX          512
#define BDE_NAME_MAX          256
#define BDE_HISTORY_MAX       32
#define BDE_MAX_ENTRIES       256
#define BDE_HASH_SLOTS        128

// Handle Type Definitions
typedef uint32_t BDeNavHandle;
typedef uint32_t BDeWatchHandle;
typedef uint64_t BDeTxHandle;

// File Attribute Flags
#define BDE_ATTR_READONLY    (1 << 0)
#define BDE_ATTR_HIDDEN      (1 << 1)
#define BDE_ATTR_SYSTEM      (1 << 2)
#define BDE_ATTR_DIRECTORY   (1 << 3)
#define BDE_ATTR_ARCHIVE     (1 << 4)
#define BDE_ATTR_VIRTUAL     (1 << 5)

// Role Permissions
typedef enum {
    BDE_PERM_ROLE_GUEST  = 0,
    BDE_PERM_ROLE_USER   = 1,
    BDE_PERM_ROLE_ADMIN  = 2,
    BDE_PERM_ROLE_SYSTEM = 3
} BDePermRole;

// Transaction Operations & Status
typedef enum {
    BDE_TX_OP_COPY   = 1,
    BDE_TX_OP_MOVE   = 2,
    BDE_TX_OP_DELETE = 3,
    BDE_TX_OP_RENAME = 4
} BDeTxOpType;

typedef enum {
    BDE_TX_STATUS_PENDING   = 0,
    BDE_TX_STATUS_RUNNING   = 1,
    BDE_TX_STATUS_PAUSED    = 2,
    BDE_TX_STATUS_COMPLETED = 3,
    BDE_TX_STATUS_CANCELLED = 4,
    BDE_TX_STATUS_FAILED    = 5
} BDeTxStatus;

typedef struct {
    BDeTxHandle tx_id;
    BDeTxOpType op_type;
    uint32_t    total_files;
    uint32_t    processed_files;
    uint64_t    total_bytes;
    uint64_t    processed_bytes;
    uint32_t    progress_percent;
    BDeTxStatus status;
    char        current_source[BDE_PATH_MAX];
    char        current_target[BDE_PATH_MAX];
} BDeTxProgress;

// Unified Directory Entry Structure
typedef struct {
    char     name[BDE_NAME_MAX];
    char     full_path[BDE_PATH_MAX];
    uint64_t size_bytes;
    uint32_t attributes;
    uint64_t time_created;
    uint64_t time_modified;
    bool     is_directory;
    bool     is_virtual;
    uint32_t icon_role;
} BDeDirEntry;

#endif // BOTREE_TYPES_H
