/*
 * ATOMS OS — VirtIO Input Device Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_input.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static void input_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    if (!dev) return;
    VirtIOInput *input = (VirtIOInput *)dev->backend_data;
    if (!input) return;

    if (q_idx == VIRTIO_INPUT_QUEUE_EVENT) {
        virtio_input_flush_events(input);
    }
}

static void input_reset_cb(VirtIODevice *dev) {
    if (!dev) return;
    VirtIOInput *input = (VirtIOInput *)dev->backend_data;
    if (input) {
        input->event_head = 0;
        input->event_tail = 0;
        input->event_count = 0;
    }
}

VirtIOInput *virtio_input_create(void) {
    VirtIOInput *input = (VirtIOInput *)kmalloc(sizeof(VirtIOInput));
    if (!input) return NULL;
    memset(input, 0, sizeof(VirtIOInput));

    /* Create underlying VirtIODevice (Device ID = 18 for Input, 2 Queues) */
    input->base = virtio_device_create(VIRTIO_DEV_ID_INPUT, VIRTIO_PCI_DEVICE_INPUT, 2, sizeof(virtio_input_config_t));
    if (!input->base) {
        kfree(input);
        return NULL;
    }

    input->base->backend_data = input;
    input->base->on_queue_notify = input_queue_notify_cb;
    input->base->on_reset = input_reset_cb;
    input->base->host_features = VIRTIO_F_VERSION_1;

    /* Populate Default Input Device Config */
    virtio_input_config_t *cfg = (virtio_input_config_t *)input->base->config_space;
    cfg->select = VIRTIO_INPUT_CFG_ID_NAME;
    strcpy(cfg->u.string, "ATOMS VirtIO Input Device");
    cfg->size = (uint8_t)strlen(cfg->u.string);

    return input;
}

void virtio_input_destroy(VirtIOInput *input) {
    if (!input) return;

    if (input->base) {
        virtio_device_destroy(input->base);
        input->base = NULL;
    }

    kfree(input);
}

bool virtio_input_inject_event(VirtIOInput *input, uint16_t type, uint16_t code, uint32_t value) {
    if (!input) return false;

    if (input->event_count >= VIRTIO_INPUT_EVENT_BUFFER_SIZE) {
        input->total_events_dropped++;
        return false; /* Buffer Full */
    }

    virtio_input_event_t *ev = &input->event_ring[input->event_tail];
    ev->type = type;
    ev->code = code;
    ev->value = value;

    input->event_tail = (input->event_tail + 1) % VIRTIO_INPUT_EVENT_BUFFER_SIZE;
    input->event_count++;

    virtio_input_flush_events(input);
    return true;
}

bool virtio_input_send_key(VirtIOInput *input, uint16_t keycode, bool pressed) {
    if (!input) return false;

    bool ok1 = virtio_input_inject_event(input, EV_KEY, keycode, pressed ? 1 : 0);
    bool ok2 = virtio_input_inject_event(input, EV_SYN, 0, 0); /* SYN Report */
    return ok1 && ok2;
}

bool virtio_input_send_mouse_rel(VirtIOInput *input, int32_t dx, int32_t dy, uint32_t buttons) {
    if (!input) return false;

    if (dx != 0) virtio_input_inject_event(input, EV_REL, REL_X, (uint32_t)dx);
    if (dy != 0) virtio_input_inject_event(input, EV_REL, REL_Y, (uint32_t)dy);

    if (buttons & 0x01) virtio_input_inject_event(input, EV_KEY, BTN_LEFT, 1);
    if (buttons & 0x02) virtio_input_inject_event(input, EV_KEY, BTN_RIGHT, 1);
    if (buttons & 0x04) virtio_input_inject_event(input, EV_KEY, BTN_MIDDLE, 1);

    virtio_input_inject_event(input, EV_SYN, 0, 0); /* SYN Report */
    return true;
}

void virtio_input_flush_events(VirtIOInput *input) {
    if (!input || !input->base || !input->base->vm || !input->base->vm->guest_mem) return;

    VirtIODevice *dev = input->base;
    VirtQueue *vq = dev->queues[VIRTIO_INPUT_QUEUE_EVENT];
    GuestMemory *mem = dev->vm->guest_mem;

    while (input->event_count > 0 && virtio_queue_has_available(vq, mem)) {
        VirtQueueChain chain;
        if (!virtio_queue_pop_chain(vq, mem, &chain)) {
            break;
        }

        if (chain.count < 1 || !chain.buffers[0].is_write || !chain.buffers[0].hva) {
            continue;
        }

        virtio_input_event_t *ev_src = &input->event_ring[input->event_head];
        virtio_input_event_t *ev_dst = (virtio_input_event_t *)chain.buffers[0].hva;

        if (chain.buffers[0].len >= sizeof(virtio_input_event_t)) {
            *ev_dst = *ev_src;

            input->event_head = (input->event_head + 1) % VIRTIO_INPUT_EVENT_BUFFER_SIZE;
            input->event_count--;
            input->total_events_sent++;

            /* Complete Chain */
            virtio_queue_complete_chain(vq, mem, chain.head_index, sizeof(virtio_input_event_t));

            /* Trigger Interrupt */
            virtio_device_raise_interrupt(dev, 0x01);
        }
    }
}
