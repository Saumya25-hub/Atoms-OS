#ifndef BOS_GPU_VIRTIO_REGS_H
#define BOS_GPU_VIRTIO_REGS_H

#include <stdint.h>

/* VirtIO PCI Identifiers */
#define VIRTIO_PCI_VENDOR_ID                0x1AF4
#define VIRTIO_PCI_DEVICE_ID_GPU            0x1050
#define VIRTIO_PCI_DEVICE_ID_GPU_TRANS      0x1000

/* VirtIO MMIO Register Offsets (Standard VirtIO over MMIO / Legacy BAR) */
#define VIRTIO_MMIO_MAGIC_VALUE             0x000
#define VIRTIO_MMIO_VERSION                 0x004
#define VIRTIO_MMIO_DEVICE_ID               0x008
#define VIRTIO_MMIO_VENDOR_ID               0x00C
#define VIRTIO_MMIO_DEVICE_FEATURES         0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL     0x014
#define VIRTIO_MMIO_DRIVER_FEATURES         0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL     0x024
#define VIRTIO_MMIO_QUEUE_SEL               0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX           0x034
#define VIRTIO_MMIO_QUEUE_NUM               0x038
#define VIRTIO_MMIO_QUEUE_ALIGN             0x03C
#define VIRTIO_MMIO_QUEUE_PFN               0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY            0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS        0x060
#define VIRTIO_MMIO_INTERRUPT_ACK           0x064
#define VIRTIO_MMIO_STATUS                  0x070
#define VIRTIO_MMIO_CONFIG                 0x100

/* VirtIO Status Bits */
#define VIRTIO_STAT_RESET                   0x00
#define VIRTIO_STAT_ACK                     0x01
#define VIRTIO_STAT_DRIVER                  0x02
#define VIRTIO_STAT_DRIVER_OK               0x04
#define VIRTIO_STAT_FEATURES_OK            0x08
#define VIRTIO_STAT_FAILED                 0x80

/* VirtQueue Descriptor Flags */
#define VIRTQ_DESC_F_NEXT                   1
#define VIRTQ_DESC_F_WRITE                  2
#define VIRTQ_DESC_F_INDIRECT               4

/* VirtQueue Structures */
typedef struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed)) virtq_desc_t;

typedef struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __attribute__((packed)) virtq_avail_t;

typedef struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed)) virtq_used_elem_t;

typedef struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    virtq_used_elem_t ring[];
} __attribute__((packed)) virtq_used_t;

/* VirtIO GPU Command Types */
#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO     0x0100
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D   0x0101
#define VIRTIO_GPU_CMD_RESOURCE_UNREF       0x0102
#define VIRTIO_GPU_CMD_SET_SCANOUT          0x0103
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH       0x0104
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D  0x0105
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0106
#define VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING 0x0107

#define VIRTIO_GPU_RESP_OK_NODATA           0x1100
#define VIRTIO_GPU_RESP_OK_DISPLAY_INFO     0x1101
#define VIRTIO_GPU_RESP_OK_CAPSET_INFO      0x1102
#define VIRTIO_GPU_RESP_OK_CAPSET           0x1103
#define VIRTIO_GPU_RESP_ERR_UNSPEC          0x1200

/* VirtIO GPU Formats */
#define VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM    1
#define VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM    2
#define VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM    3
#define VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM    4
#define VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM    67

/* VirtIO GPU Command Request Structures */
typedef struct virtio_gpu_ctrl_hdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_ctrl_hdr_t;

typedef struct virtio_gpu_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_rect_t;

typedef struct virtio_gpu_resource_create_2d {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_resource_create_2d_t;

typedef struct virtio_gpu_mem_entry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_mem_entry_t;

typedef struct virtio_gpu_resource_attach_backing {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
    virtio_gpu_mem_entry_t entries[1];
} __attribute__((packed)) virtio_gpu_resource_attach_backing_t;

typedef struct virtio_gpu_set_scanout {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t scanout_id;
    uint32_t resource_id;
} __attribute__((packed)) virtio_gpu_set_scanout_t;

typedef struct virtio_gpu_transfer_to_host_2d {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_transfer_to_host_2d_t;

typedef struct virtio_gpu_resource_flush {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_resource_flush_t;

#endif /* BOS_GPU_VIRTIO_REGS_H */
