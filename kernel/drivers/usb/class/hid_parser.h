#ifndef ATOMS_USB_HID_PARSER_H
#define ATOMS_USB_HID_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define HID_MAX_BUTTONS 16

/* =========================================================================
 * HID 1.11 Item Types and Tags
 * ========================================================================= */
#define HID_ITEM_TYPE_MAIN      0
#define HID_ITEM_TYPE_GLOBAL    1
#define HID_ITEM_TYPE_LOCAL     2

/* Main Items */
#define HID_MAIN_TAG_INPUT          0x8
#define HID_MAIN_TAG_OUTPUT         0x9
#define HID_MAIN_TAG_COLLECTION     0xA
#define HID_MAIN_TAG_FEATURE        0xB
#define HID_MAIN_TAG_END_COLLECTION 0xC

/* Global Items */
#define HID_GLOBAL_TAG_USAGE_PAGE   0x0
#define HID_GLOBAL_TAG_LOG_MIN      0x1
#define HID_GLOBAL_TAG_LOG_MAX      0x2
#define HID_GLOBAL_TAG_PHYS_MIN     0x3
#define HID_GLOBAL_TAG_PHYS_MAX     0x4
#define HID_GLOBAL_TAG_UNIT_EXP     0x5
#define HID_GLOBAL_TAG_UNIT         0x6
#define HID_GLOBAL_TAG_REPORT_SIZE  0x7
#define HID_GLOBAL_TAG_REPORT_ID    0x8
#define HID_GLOBAL_TAG_REPORT_COUNT 0x9
#define HID_GLOBAL_TAG_PUSH         0xA
#define HID_GLOBAL_TAG_POP          0xB

/* Local Items */
#define HID_LOCAL_TAG_USAGE         0x0
#define HID_LOCAL_TAG_USAGE_MIN     0x1
#define HID_LOCAL_TAG_USAGE_MAX     0x2

/* Usage Pages */
#define HID_UP_GENERIC_DESKTOP  0x01
#define HID_UP_KEYBOARD         0x07
#define HID_UP_LEDS             0x08
#define HID_UP_BUTTONS          0x09
#define HID_UP_CONSUMER         0x0C

/* Generic Desktop Usages */
#define HID_USAGE_POINTER   0x01
#define HID_USAGE_MOUSE     0x02
#define HID_USAGE_X         0x30
#define HID_USAGE_Y         0x31
#define HID_USAGE_Z         0x32
#define HID_USAGE_WHEEL     0x38

/* =========================================================================
 * Parsed Mouse Field Map
 * ========================================================================= */
typedef struct {
    bool     valid;
    bool     has_report_id;
    uint8_t  report_id;
    uint32_t report_length_bits;

    /* X Axis (Displacement) */
    bool     has_x;
    uint16_t x_bit_offset;
    uint8_t  x_bit_size;
    bool     x_is_signed;
    int32_t  x_logical_min;
    int32_t  x_logical_max;

    /* Y Axis (Displacement) */
    bool     has_y;
    uint16_t y_bit_offset;
    uint8_t  y_bit_size;
    bool     y_is_signed;
    int32_t  y_logical_min;
    int32_t  y_logical_max;

    /* Scroll Wheel */
    bool     has_wheel;
    uint16_t wheel_bit_offset;
    uint8_t  wheel_bit_size;
    bool     wheel_is_signed;

    /* Buttons (1 to 16) */
    uint8_t  button_count;
    uint16_t button_bit_offset[HID_MAX_BUTTONS];
} hid_mouse_field_map_t;

/* =========================================================================
 * HRE Engine Public API
 * ========================================================================= */
bool hid_parse_mouse_report_desc(const uint8_t* desc, uint16_t len, hid_mouse_field_map_t* out_map);

int32_t hid_extract_field(const uint8_t* report, uint32_t length_bytes, uint32_t bit_offset, uint32_t bit_size, bool is_signed);

bool hid_decode_mouse_report(const hid_mouse_field_map_t* map, const uint8_t* report, uint32_t length_bytes,
                             int32_t* out_dx, int32_t* out_dy, uint8_t* out_buttons, int32_t* out_wheel);

#endif // ATOMS_USB_HID_PARSER_H
