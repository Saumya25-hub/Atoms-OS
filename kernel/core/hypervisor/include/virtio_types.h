/*
 * ATOMS OS — VirtIO Standard Constants and Data Structures
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_TYPES_H
#define ATOMS_VIRTIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard VirtIO PCI Vendor & Subsystem IDs */
#define VIRTIO_PCI_VENDOR_ID                0x1AF4
#define VIRTIO_PCI_DEVICE_NET               0x1000
#define VIRTIO_PCI_DEVICE_BLOCK             0x1001
#define VIRTIO_PCI_DEVICE_CONSOLE           0x1003
#define VIRTIO_PCI_DEVICE_ENTROPY           0x1005
#define VIRTIO_PCI_DEVICE_BALLOON           0x1002
#define VIRTIO_PCI_DEVICE_GPU               0x1050
#define VIRTIO_PCI_DEVICE_INPUT             0x1052
#define VIRTIO_PCI_DEVICE_SOCKET            0x1053

/* VirtIO Device IDs (Modern VirtIO Specification v1.1) */
#define VIRTIO_DEV_ID_RESERVED              0
#define VIRTIO_DEV_ID_NET                   1
#define VIRTIO_DEV_ID_BLOCK                 2
#define VIRTIO_DEV_ID_CONSOLE               3
#define VIRTIO_DEV_ID_ENTROPY               4
#define VIRTIO_DEV_ID_BALLOON               5
#define VIRTIO_DEV_ID_IOMEMORY              6
#define VIRTIO_DEV_ID_RPMSG                 7
#define VIRTIO_DEV_ID_SCSI                  8
#define VIRTIO_DEV_ID_9P                    9
#define VIRTIO_DEV_ID_MAC80211              10
#define VIRTIO_DEV_ID_GPU                   16
#define VIRTIO_DEV_ID_INPUT                 18
#define VIRTIO_DEV_ID_VSOCK                 19

/* VirtIO Device Status Bits */
#define VIRTIO_STATUS_RESET                 0x00
#define VIRTIO_STATUS_ACKNOWLEDGE           0x01
#define VIRTIO_STATUS_DRIVER                0x02
#define VIRTIO_STATUS_DRIVER_OK             0x04
#define VIRTIO_STATUS_FEATURES_OK           0x08
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET    0x40
#define VIRTIO_STATUS_FAILED                0x80

/* VirtIO Global Feature Bits */
#define VIRTIO_F_NOTIFY_ON_EMPTY            (1ULL << 24)
#define VIRTIO_F_ANY_LAYOUT                 (1ULL << 27)
#define VIRTIO_F_RING_INDIRECT_DESC         (1ULL << 28)
#define VIRTIO_F_RING_EVENT_IDX             (1ULL << 29)
#define VIRTIO_F_VERSION_1                  (1ULL << 32)
#define VIRTIO_F_ACCESS_PLATFORM            (1ULL << 33)
#define VIRTIO_F_RING_PACKED                (1ULL << 34)
#define VIRTIO_F_IN_ORDER                   (1ULL << 35)
#define VIRTIO_F_ORDER_PLATFORM             (1ULL << 36)
#define VIRTIO_F_SR_IOV                     (1ULL << 37)

/* VirtQueue Descriptor Flags */
#define VIRTQ_DESC_F_NEXT                   1
#define VIRTQ_DESC_F_WRITE                  2
#define VIRTQ_DESC_F_INDIRECT               4

/* VirtQueue Memory Structures (Standard Split Virtqueue Layout) */
typedef struct virtq_desc {
    uint64_t addr;   /* Guest Physical Address (GPA) */
    uint32_t len;    /* Buffer Length */
    uint16_t flags;  /* Descriptor Flags */
    uint16_t next;   /* Next descriptor index in chain if NEXT flag is set */
} __attribute__((packed)) virtq_desc_t;

typedef struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __attribute__((packed)) virtq_avail_t;

typedef struct virtq_used_elem {
    uint32_t id;     /* Descriptor chain head index */
    uint32_t len;    /* Number of bytes written by host device */
} __attribute__((packed)) virtq_used_elem_t;

typedef struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[];
} __attribute__((packed)) virtq_used_t;

/* VirtIO Block Device Specifics */
#define VIRTIO_BLK_F_SIZE_MAX               (1ULL << 1)
#define VIRTIO_BLK_F_SEG_MAX                (1ULL << 2)
#define VIRTIO_BLK_F_GEOMETRY               (1ULL << 4)
#define VIRTIO_BLK_F_RO                     (1ULL << 5)
#define VIRTIO_BLK_F_BLK_SIZE               (1ULL << 6)
#define VIRTIO_BLK_F_FLUSH                  (1ULL << 9)
#define VIRTIO_BLK_F_TOPOLOGY               (1ULL << 10)

#define VIRTIO_BLK_T_IN                     0   /* Read */
#define VIRTIO_BLK_T_OUT                    1   /* Write */
#define VIRTIO_BLK_T_FLUSH                  4   /* Cache Flush */
#define VIRTIO_BLK_T_GET_ID                 8   /* Get Device Serial */

#define VIRTIO_BLK_S_OK                     0
#define VIRTIO_BLK_S_IOERR                  1
#define VIRTIO_BLK_S_UNSUPP                 2

typedef struct virtio_blk_req_hdr {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed)) virtio_blk_req_hdr_t;

typedef struct virtio_blk_config {
    uint64_t capacity;      /* Number of 512-byte sectors */
    uint32_t size_max;
    uint32_t seg_max;
    struct {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;
    uint32_t blk_size;
} __attribute__((packed)) virtio_blk_config_t;

/* VirtIO Net Device Specifics */
#define VIRTIO_NET_F_CSUM                   (1ULL << 0)
#define VIRTIO_NET_F_GUEST_CSUM             (1ULL << 1)
#define VIRTIO_NET_F_MAC                    (1ULL << 5)
#define VIRTIO_NET_F_STATUS                 (1ULL << 16)
#define VIRTIO_NET_F_SPEED_DUPLEX           (1ULL << 63)

#define VIRTIO_NET_S_LINK_UP                1
#define VIRTIO_NET_S_ANNOUNCE               2

typedef struct virtio_net_hdr {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    uint16_t num_buffers;
} __attribute__((packed)) virtio_net_hdr_t;

typedef struct virtio_net_config {
    uint8_t mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
    uint32_t speed;
    uint8_t duplex;
} __attribute__((packed)) virtio_net_config_t;

/* VirtIO Input Device Specifics (Linux/FreeBSD standard input event encoding) */
#define VIRTIO_INPUT_CFG_UNSET              0x00
#define VIRTIO_INPUT_CFG_ID_NAME            0x01
#define VIRTIO_INPUT_CFG_ID_SERIAL          0x02
#define VIRTIO_INPUT_CFG_ID_DEVIDS          0x03
#define VIRTIO_INPUT_CFG_PROP_BITS          0x10
#define VIRTIO_INPUT_CFG_EV_BITS            0x11
#define VIRTIO_INPUT_CFG_ABS_INFO           0x12

typedef struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed)) virtio_input_event_t;

typedef struct virtio_input_config {
    uint8_t select;
    uint8_t subsel;
    uint8_t size;
    uint8_t reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
        struct {
            uint32_t min;
            uint32_t max;
            uint32_t fuzz;
            uint32_t flat;
            uint32_t res;
        } abs;
        struct {
            uint16_t bustype;
            uint16_t vendor;
            uint16_t product;
            uint16_t version;
        } ids;
    } u;
} __attribute__((packed)) virtio_input_config_t;

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_TYPES_H */
