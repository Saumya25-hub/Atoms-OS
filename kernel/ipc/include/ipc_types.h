/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_types.h — Core Type Definitions
 *
 * All status codes, handle types, message structures,
 * channel descriptors, shared memory descriptors, and
 * permission flags used across the IPC subsystem.
 */

#ifndef BOS_IPC_TYPES_H
#define BOS_IPC_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================
 * Status Return Codes
 * ============================================================ */
typedef enum {
    IPC_SUCCESS                    =  0,
    IPC_ERR_NULL_POINTER           = -1,
    IPC_ERR_INVALID_HANDLE         = -2,
    IPC_ERR_INVALID_NAME           = -3,
    IPC_ERR_INVALID_SIZE           = -4,
    IPC_ERR_INVALID_PID            = -5,
    IPC_ERR_INVALID_FLAGS          = -6,
    IPC_ERR_NAME_EXISTS            = -7,
    IPC_ERR_NAME_NOT_FOUND         = -8,
    IPC_ERR_NO_MEMORY              = -9,
    IPC_ERR_QUEUE_FULL             = -10,
    IPC_ERR_QUEUE_EMPTY            = -11,
    IPC_ERR_PERMISSION_DENIED      = -12,
    IPC_ERR_ALREADY_MAPPED         = -13,
    IPC_ERR_NOT_MAPPED             = -14,
    IPC_ERR_ALREADY_DESTROYED      = -15,
    IPC_ERR_DOUBLE_DESTROY         = -16,
    IPC_ERR_HANDLE_LEAK            = -17,
    IPC_ERR_MAX_CHANNELS           = -18,
    IPC_ERR_MAX_SHM_OBJECTS        = -19,
    IPC_ERR_MAX_PIPES              = -20,
    IPC_ERR_MAX_PORTS              = -21,
    IPC_ERR_BUFFER_TOO_SMALL       = -22,
    IPC_ERR_TIMEOUT                = -23,
    IPC_ERR_WOULD_BLOCK            = -24,
    IPC_ERR_PIPE_CLOSED            = -25,
    IPC_ERR_NOT_INITIALIZED        = -26
} ipc_status_t;

/* ============================================================
 * Handle Types (Opaque 32-bit Identifiers)
 * ============================================================ */
typedef uint32_t ipc_channel_handle_t;
typedef uint32_t ipc_shm_handle_t;
typedef uint32_t ipc_pipe_handle_t;
typedef uint32_t ipc_port_handle_t;

#define IPC_INVALID_HANDLE          0xFFFFFFFF

/* ============================================================
 * Channel Flags
 * ============================================================ */
#define IPC_CHANNEL_NAMED           0x0001
#define IPC_CHANNEL_ANONYMOUS       0x0002
#define IPC_CHANNEL_BIDIRECTIONAL   0x0004
#define IPC_CHANNEL_BROADCAST       0x0008

/* ============================================================
 * Send / Receive Flags
 * ============================================================ */
#define IPC_FLAG_BLOCKING           0x0000
#define IPC_FLAG_NONBLOCKING        0x0001
#define IPC_FLAG_TIMEOUT            0x0002

/* ============================================================
 * Shared Memory Permission Flags
 * ============================================================ */
#define IPC_SHM_READ                0x0001
#define IPC_SHM_WRITE               0x0002
#define IPC_SHM_RDWR                (IPC_SHM_READ | IPC_SHM_WRITE)
#define IPC_SHM_EXEC                0x0004

/* ============================================================
 * Capacity Limits
 * ============================================================ */
#define IPC_MAX_CHANNELS            64
#define IPC_MAX_SHM_OBJECTS         32
#define IPC_MAX_PIPES               32
#define IPC_MAX_PORTS               32
#define IPC_MAX_NAME_LENGTH         63
#define IPC_MAX_MESSAGE_SIZE        2048
#define IPC_MESSAGE_QUEUE_DEPTH     16
#define IPC_PIPE_BUFFER_SIZE        4096
#define IPC_MAX_SHM_PAGES           256
#define IPC_MAX_SHM_MAPPINGS        8
#define IPC_MAX_SUBSCRIBERS         16
#define IPC_MAX_ROUTER_ROUTES       64

/* ============================================================
 * IPC Message Header
 * ============================================================ */
typedef struct {
    uint32_t    src_pid;
    uint32_t    dst_pid;
    uint32_t    channel_id;
    uint32_t    msg_type;
    uint32_t    payload_size;
    uint32_t    sequence_number;
    uint32_t    flags;
    uint32_t    reserved;
} ipc_message_header_t;

/* ============================================================
 * IPC Message (Header + Inline Payload)
 * ============================================================ */
typedef struct {
    ipc_message_header_t header;
    uint8_t              payload[IPC_MAX_MESSAGE_SIZE];
} ipc_message_t;

/* ============================================================
 * Channel Descriptor
 * ============================================================ */
typedef enum {
    IPC_CHANNEL_STATE_FREE = 0,
    IPC_CHANNEL_STATE_OPEN,
    IPC_CHANNEL_STATE_CONNECTED,
    IPC_CHANNEL_STATE_CLOSED
} ipc_channel_state_t;

typedef struct {
    uint32_t             id;
    char                 name[IPC_MAX_NAME_LENGTH + 1];
    uint32_t             flags;
    ipc_channel_state_t  state;
    uint32_t             owner_pid;
    uint32_t             peer_pid;
    uint32_t             ref_count;
    uint32_t             messages_sent;
    uint32_t             messages_received;
} ipc_channel_t;

/* ============================================================
 * Shared Memory Object Descriptor
 * ============================================================ */
typedef enum {
    IPC_SHM_STATE_FREE = 0,
    IPC_SHM_STATE_CREATED,
    IPC_SHM_STATE_DESTROYED
} ipc_shm_state_t;

typedef struct {
    uint32_t    pid;
    uint64_t    virt_addr;
    uint32_t    permissions;
    bool        active;
} ipc_shm_mapping_t;

typedef struct {
    uint32_t            id;
    char                name[IPC_MAX_NAME_LENGTH + 1];
    ipc_shm_state_t     state;
    uint32_t            owner_pid;
    uint32_t            size_bytes;
    uint32_t            page_count;
    uint64_t            phys_pages[IPC_MAX_SHM_PAGES];
    ipc_shm_mapping_t   mappings[IPC_MAX_SHM_MAPPINGS];
    uint32_t            mapping_count;
    uint32_t            ref_count;
    uint32_t            flags;
} ipc_shm_object_t;

/* ============================================================
 * Pipe Descriptor
 * ============================================================ */
typedef enum {
    IPC_PIPE_STATE_FREE = 0,
    IPC_PIPE_STATE_OPEN,
    IPC_PIPE_STATE_CLOSED
} ipc_pipe_state_t;

typedef struct {
    uint32_t            id;
    ipc_pipe_state_t    state;
    uint32_t            reader_pid;
    uint32_t            writer_pid;
    uint8_t             buffer[IPC_PIPE_BUFFER_SIZE];
    uint32_t            head;
    uint32_t            tail;
    uint32_t            count;
    uint32_t            bytes_transferred;
} ipc_pipe_t;

/* ============================================================
 * Port Descriptor
 * ============================================================ */
typedef enum {
    IPC_PORT_STATE_FREE = 0,
    IPC_PORT_STATE_BOUND,
    IPC_PORT_STATE_CLOSED
} ipc_port_state_t;

typedef struct {
    uint32_t            id;
    char                name[IPC_MAX_NAME_LENGTH + 1];
    ipc_port_state_t    state;
    uint32_t            owner_pid;
    ipc_channel_handle_t bound_channel;
    uint32_t            ref_count;
} ipc_port_t;

/* ============================================================
 * Router Route Entry
 * ============================================================ */
typedef struct {
    uint32_t             channel_id;
    uint32_t             subscriber_pids[IPC_MAX_SUBSCRIBERS];
    uint32_t             subscriber_count;
    bool                 active;
} ipc_route_t;

/* ============================================================
 * Sync Primitives
 * ============================================================ */
typedef struct {
    volatile uint32_t    locked;
    volatile uint32_t    owner;
} ipc_spinlock_t;

typedef struct {
    volatile uint32_t    locked;
    volatile uint32_t    owner;
    volatile uint32_t    wait_count;
} ipc_mutex_t;

typedef struct {
    volatile int32_t     readers;
    volatile uint32_t    writer;
    volatile uint32_t    writer_owner;
} ipc_rwlock_t;

typedef struct {
    volatile uint32_t    signaled;
    volatile uint32_t    auto_reset;
} ipc_event_t;

#endif /* BOS_IPC_TYPES_H */
