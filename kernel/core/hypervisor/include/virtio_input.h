/*
 * ATOMS OS — VirtIO Input Device Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_INPUT_H
#define ATOMS_VIRTIO_INPUT_H

#include "virtio_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTIO_INPUT_QUEUE_EVENT            0
#define VIRTIO_INPUT_QUEUE_STATUS           1
#define VIRTIO_INPUT_EVENT_BUFFER_SIZE      128

/* Standard Linux/FreeBSD Event Types */
#define EV_SYN                              0x00
#define EV_KEY                              0x01
#define EV_REL                              0x02
#define EV_ABS                              0x03

/* Standard Linux/FreeBSD Relative Axes */
#define REL_X                               0x00
#define REL_Y                               0x01
#define REL_WHEEL                           0x08

/* Standard Button Codes */
#define BTN_LEFT                            0x110
#define BTN_RIGHT                           0x111
#define BTN_MIDDLE                          0x112

/* VirtIO Input Device Instance */
typedef struct virtio_input_dev {
    VirtIODevice *base;

    /* Pending Event Buffer Ring */
    virtio_input_event_t event_ring[VIRTIO_INPUT_EVENT_BUFFER_SIZE];
    uint32_t event_head;
    uint32_t event_tail;
    uint32_t event_count;

    /* Metrics & Statistics */
    uint64_t total_events_sent;
    uint64_t total_events_dropped;
} VirtIOInput;

/* Core APIs */
VirtIOInput *virtio_input_create(void);
void virtio_input_destroy(VirtIOInput *input);

/* Event Injection APIs */
bool virtio_input_inject_event(VirtIOInput *input, uint16_t type, uint16_t code, uint32_t value);
bool virtio_input_send_key(VirtIOInput *input, uint16_t keycode, bool pressed);
bool virtio_input_send_mouse_rel(VirtIOInput *input, int32_t dx, int32_t dy, uint32_t buttons);
void virtio_input_flush_events(VirtIOInput *input);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_INPUT_H */
