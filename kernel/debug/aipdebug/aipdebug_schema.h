#ifndef AIPDEBUG_SCHEMA_H
#define AIPDEBUG_SCHEMA_H

#include <stdint.h>
#include <stdbool.h>

#define AIPD_MAGIC                  0x41495044 /* "AIPD" (AI-PDebug Packet Magic) */
#define AIPD_UDP_PORT               9997
#define AIPD_MAX_RECORDS_PER_PKT    40
#define AIPD_RING_CAPACITY          2048       /* 2048 records * 32 bytes = 64 KB */

/* ========================================================================= */
/* 1. Truth Model Classification                                             */
/* ========================================================================= */
typedef enum {
    AIPD_TRUTH_NONE                 = 0,
    AIPD_TRUTH_OBSERVED             = 1, /* Direct physical register/wire/port reading */
    AIPD_TRUTH_DERIVED              = 2, /* Deterministic 1:1 bit translation/lookup */
    AIPD_TRUTH_INFERRED             = 3, /* Causal candidate or hypothesis */
    AIPD_TRUTH_UNKNOWN              = 4  /* Unverified / unprobed state */
} AIPDTruthClass;

/* ========================================================================= */
/* 2. Subsystems & Components                                                */
/* ========================================================================= */
typedef enum {
    AIPD_SUBSYS_NONE                = 0,
    AIPD_SUBSYS_CORE                = 1,
    AIPD_SUBSYS_PS2_CONTROLLER      = 2,
    AIPD_SUBSYS_PS2_KEYBOARD        = 3,
    AIPD_SUBSYS_PS2_MOUSE           = 4,
    AIPD_SUBSYS_USB_XHCI            = 5,
    AIPD_SUBSYS_NET_RTL8168         = 6,
    AIPD_SUBSYS_PMM                 = 7,
    AIPD_SUBSYS_VMM                 = 8,
    AIPD_SUBSYS_SCHEDULER           = 9,
    AIPD_SUBSYS_COMPOSITOR          = 10
} AIPDSubsystem;

typedef enum {
    AIPD_COMP_NONE                  = 0,
    AIPD_COMP_8042_PORT60           = 1,
    AIPD_COMP_8042_PORT64           = 2,
    AIPD_COMP_8042_CONFIG_REG       = 3,
    AIPD_COMP_8042_STATUS_REG       = 4,
    AIPD_COMP_LPC_CHANNEL1_WIRE     = 5,
    AIPD_COMP_LPC_CHANNEL2_WIRE     = 6,
    AIPD_COMP_KEYBOARD_DEVICE       = 7,
    AIPD_COMP_MOUSE_DEVICE          = 8
} AIPDComponent;

/* ========================================================================= */
/* 3. Event Types & Operations                                               */
/* ========================================================================= */
typedef enum {
    AIPD_EVT_NONE                   = 0,
    AIPD_EVT_REG_READ               = 1, /* inb / read MMIO */
    AIPD_EVT_REG_WRITE              = 2, /* outb / write MMIO */
    AIPD_EVT_CMD_DISPATCH           = 3, /* Command sent to controller or device */
    AIPD_EVT_RESP_RECEIVED          = 4, /* Byte received from device/controller */
    AIPD_EVT_TIMEOUT_EXPIRED        = 5, /* Microsecond or cycle timer expired */
    AIPD_EVT_STATE_TRANSITION       = 6, /* State machine state change */
    AIPD_EVT_CONFIG_CHANGE          = 7, /* Command byte / configuration byte change */
    AIPD_EVT_TX_BEGIN               = 8, /* Transaction started */
    AIPD_EVT_TX_END                 = 9  /* Transaction terminated */
} AIPDEventType;

/* ========================================================================= */
/* 4. Transaction Types & Results                                            */
/* ========================================================================= */
typedef enum {
    AIPD_TX_NONE                    = 0,
    AIPD_TX_8042_SELFTEST           = 1, /* CMD 0xAA */
    AIPD_TX_8042_IFACE1_TEST        = 2, /* CMD 0xAB */
    AIPD_TX_8042_IFACE2_TEST        = 3, /* CMD 0xA9 */
    AIPD_TX_8042_RECONFIG           = 4, /* CMD 0x60 */
    AIPD_TX_PS2_RESET_BAT           = 5, /* CMD 0xFF -> ACK (0xFA) -> BAT (0xAA) */
    AIPD_TX_PS2_ENABLE_SCANNING     = 6, /* CMD 0xF4 */
    AIPD_TX_PS2_ECHO_PROBE          = 7, /* CMD 0xEE */
    AIPD_TX_PS2_SET_LEDS            = 8  /* CMD 0xED -> 0xFA -> Mask -> 0xFA */
} AIPDTransactionType;

typedef enum {
    AIPD_RES_PENDING                = 0,
    AIPD_RES_SUCCESS                = 1,
    AIPD_RES_FAILURE                = 2,
    AIPD_RES_TIMEOUT                = 3,
    AIPD_RES_RESEND                 = 4,
    AIPD_RES_BLOCKED                = 5
} AIPDTransactionResult;

/* ========================================================================= */
/* 5. Subsystem Profiling Masks                                              */
/* ========================================================================= */
typedef enum {
    AIPD_PROF_DISABLED              = 0,
    AIPD_PROF_PS2_KEYBOARD          = (1 << 0),
    AIPD_PROF_PS2_MOUSE             = (1 << 1),
    AIPD_PROF_USB_XHCI              = (1 << 2),
    AIPD_PROF_PMM                   = (1 << 3),
    AIPD_PROF_VMM                   = (1 << 4),
    AIPD_PROF_SCHEDULER             = (1 << 5),
    AIPD_PROF_ALL                   = 0xFFFF
} AIPDProfileMask;

#pragma pack(push, 1)

/* ========================================================================= */
/* 6. Binary Forensic Event Record (Exactly 32 Bytes)                        */
/* ========================================================================= */
typedef struct {
    uint64_t tsc;                   /* 8B: Monotonic CPU TSC timestamp */
    uint16_t time_us_delta;         /* 2B: Delta microseconds from transaction start */
    uint8_t  subsystem_id;          /* 1B: AIPDSubsystem */
    uint8_t  component_id;          /* 1B: AIPDComponent */
    uint8_t  truth_class;           /* 1B: AIPDTruthClass (OBSERVED/DERIVED/INFERRED) */
    uint8_t  event_type;            /* 1B: AIPDEventType */
    uint16_t transaction_id;        /* 2B: Parent transaction ID (0 = none) */
    uint16_t transaction_type;      /* 2B: AIPDTransactionType */
    uint32_t address_or_port;       /* 4B: I/O Port or MMIO physical address */
    uint32_t raw_value_before;      /* 4B: Value prior to operation */
    uint32_t raw_value_after;       /* 4B: Value following operation */
    uint16_t result_status;         /* 2B: Result / Error / AIPDTransactionResult */
} AIPDEventRecord;                  /* Total: 8+2+1+1+1+1+2+2+4+4+4+2 = 32 Bytes */

/* ========================================================================= */
/* 7. UDP Packet Wire Header (16 Bytes)                                      */
/* ========================================================================= */
typedef struct {
    uint32_t magic;                 /* 4B: 0x41495044 ("AIPD") */
    uint32_t session_id;            /* 4B: Test run / session identifier */
    uint32_t sequence_number;       /* 4B: Monotonic packet counter */
    uint16_t record_count;          /* 2B: Number of AIPDEventRecords in packet */
    uint16_t active_profile;        /* 2B: Bitmask of active profiling subsystems */
} AIPDPacketHeader;                 /* Total: 16 Bytes */

#pragma pack(pop)

#endif /* AIPDEBUG_SCHEMA_H */
