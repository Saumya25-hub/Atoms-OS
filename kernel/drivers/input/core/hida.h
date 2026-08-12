#ifndef HIDA_H
#define HIDA_H

#include <stdint.h>
#include <stdbool.h>

#define HIDA_BACKEND_NONE 0
#define HIDA_BACKEND_VMMOUSE 120
#define HIDA_BACKEND_USB 121
#define HIDA_BACKEND_PS2 122
#define HIDA_BACKEND_USB_KBD 123
#define HIDA_BACKEND_PS2_KBD 124

typedef enum {
    INPUT_DEV_TYPE_UNKNOWN = 0,
    INPUT_DEV_TYPE_PS2_MOUSE,
    INPUT_DEV_TYPE_USB_HID_MOUSE,
    INPUT_DEV_TYPE_USB_TABLET,
    INPUT_DEV_TYPE_VMMOUSE,
    INPUT_DEV_TYPE_VBOX_TABLET,
    INPUT_DEV_TYPE_USB_HID_KEYBOARD,
    INPUT_DEV_TYPE_PS2_KEYBOARD
} InputDeviceType;

typedef enum {
    HIDA_STATE_DETECTING = 0,
    HIDA_STATE_ACTIVE,
    HIDA_STATE_DEGRADED,
    HIDA_STATE_FAILED,
    HIDA_STATE_FALLBACK
} HidaState;

typedef struct {
    uint32_t backend_id;
    InputDeviceType type;
    const char* device_name;
    const char* driver_name;
    bool is_supported;
    bool is_initialized;
    bool is_connected;
    bool is_enumerated;
    bool is_receiving_events;
    bool is_absolute;
    bool is_polling;
    bool is_eligible;
    uint32_t valid_packet_count;
    uint32_t invalid_packet_count;
    uint32_t parse_errors;
    uint32_t sync_errors;
    uint32_t priority_score;   // VMMouse=100, USB Tablet=90, USB Kbd=85, USB Mouse=80, PS2 Kbd=65, PS2 Mouse=60
    uint32_t health_score;     // 0..100
    uint32_t latency_us;
    HidaState status;
    uint64_t last_event_tick;
    uint64_t total_events;
} InputDeviceDescriptor;

void hida_init(void);
void hida_register_device(const InputDeviceDescriptor* desc);
void hida_set_device_connected(uint32_t backend_id, bool connected);
void hida_report_event_parsed(uint32_t backend_id, bool is_valid);
void hida_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll);
void hida_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll);
void hida_push_keyboard_event(uint32_t backend_id, const void* kevt);
uint32_t hida_get_active_owner(void);
const InputDeviceDescriptor* hida_get_device_descriptor(uint32_t backend_id);
void hida_dump_status(void);

#endif
