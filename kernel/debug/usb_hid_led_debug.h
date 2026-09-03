#ifndef ATOMS_USB_HID_LED_DEBUG_H
#define ATOMS_USB_HID_LED_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

typedef struct {
    /* 1. Hardware Identity & Enumeration */
    bool keyboard_found;
    uint8_t address;
    uint8_t slot_id;
    uint16_t vid;
    uint16_t pid;
    uint8_t interface_num;
    uint8_t protocol;
    uint8_t subclass;

    /* 2. Endpoint Configuration */
    uint8_t interrupt_in_ep;
    uint16_t interrupt_in_max_pkt;
    bool has_interrupt_out;
    uint8_t interrupt_out_ep;
    uint16_t interrupt_out_max_pkt;
    const char *out_transport_type; /* "CONTROL SET_REPORT (EP0)" or "INTERRUPT OUT" */

    /* 3. Live Software & Hardware State */
    bool caps_state;
    bool num_state;
    bool scroll_state;
    uint8_t last_led_mask;
    bool last_transfer_success;
    uint32_t total_transfers;
    uint32_t successful_transfers;
    uint32_t failed_transfers;

    /* 4. Automated Benchmark Results (20 Cycles Each) */
    struct {
        uint32_t cycles_tested;
        uint32_t transfers_sent;
        uint32_t acks_received;
        bool pass;
    } caps_benchmark;

    struct {
        uint32_t cycles_tested;
        uint32_t transfers_sent;
        uint32_t acks_received;
        bool pass;
    } num_benchmark;

    struct {
        uint32_t cycles_tested;
        uint32_t transfers_sent;
        uint32_t acks_received;
        bool pass;
    } combined_benchmark;

    struct {
        uint32_t cycles_tested;
        uint32_t transfers_sent;
        uint32_t acks_received;
        bool pass;
    } scroll_benchmark;

} UsbHidLedAudit;

extern UsbHidLedAudit g_usb_hid_led_audit;

void usb_hid_led_debug_run(boot_info_t *boot_info);

#endif /* ATOMS_USB_HID_LED_DEBUG_H */
