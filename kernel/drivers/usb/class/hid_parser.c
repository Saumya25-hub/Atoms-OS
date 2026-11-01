#include "hid_parser.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char*);
extern void display_print_dec(uint64_t);

/* =========================================================================
 * Universal Dynamic Bitfield Extractor
 * Extracts unaligned bit sequences across byte boundaries and sign-extends
 * ========================================================================= */
int32_t hid_extract_field(const uint8_t* report, uint32_t length_bytes, uint32_t bit_offset, uint32_t bit_size, bool is_signed) {
    if (!report || length_bytes == 0 || bit_size == 0 || bit_size > 32) return 0;
    if ((bit_offset + bit_size + 7) / 8 > length_bytes) return 0;

    uint32_t byte_start = bit_offset / 8;
    uint32_t bit_start = bit_offset % 8;

    uint64_t val = 0;
    for (uint32_t b = 0; b < 5 && (byte_start + b) < length_bytes; b++) {
        val |= ((uint64_t)report[byte_start + b]) << (b * 8);
    }

    val >>= bit_start;
    uint64_t mask = (bit_size == 32) ? 0xFFFFFFFFULL : ((1ULL << bit_size) - 1ULL);
    val &= mask;

    int32_t result = (int32_t)val;
    if (is_signed && bit_size < 32) {
        if (result & (1 << (bit_size - 1))) {
            result |= ~((int32_t)mask); // Two's complement sign-extend
        }
    }
    return result;
}

/* =========================================================================
 * HRE (Universal HID 1.11 Bytecode Parser Engine)
 * Parses raw HID Report Descriptors and builds an optimized field map
 * ========================================================================= */
typedef struct {
    uint16_t usage_page;
    int32_t  log_min;
    int32_t  log_max;
    uint32_t report_size;
    uint32_t report_count;
    uint8_t  report_id;
} hid_global_state_t;

#define HID_GLOBAL_STACK_SIZE 8

bool hid_parse_mouse_report_desc(const uint8_t* desc, uint16_t len, hid_mouse_field_map_t* out_map) {
    if (!desc || len == 0 || !out_map) return false;

    memset(out_map, 0, sizeof(hid_mouse_field_map_t));

    hid_global_state_t global;
    memset(&global, 0, sizeof(global));

    hid_global_state_t global_stack[HID_GLOBAL_STACK_SIZE];
    uint32_t stack_ptr = 0;

    uint16_t local_usages[64];
    uint32_t local_usage_count = 0;
    uint16_t local_usage_min = 0;
    uint16_t local_usage_max = 0;

    uint32_t bit_offset = 0;
    uint32_t i = 0;

    while (i < len) {
        uint8_t prefix = desc[i++];
        uint8_t bSize = prefix & 0x03;
        uint8_t bType = (prefix >> 2) & 0x03;
        uint8_t bTag  = (prefix >> 4) & 0x0F;

        if (bSize == 3) bSize = 4;
        if (i + bSize > len) break;

        uint32_t raw_val = 0;
        for (uint8_t b = 0; b < bSize; b++) {
            raw_val |= ((uint32_t)desc[i + b]) << (b * 8);
        }

        int32_t sval = (int32_t)raw_val;
        if (bSize == 1 && (sval & 0x80)) sval |= 0xFFFFFF00;
        else if (bSize == 2 && (sval & 0x8000)) sval |= 0xFFFF0000;

        i += bSize;

        if (bType == HID_ITEM_TYPE_GLOBAL) {
            switch (bTag) {
                case HID_GLOBAL_TAG_USAGE_PAGE:
                    global.usage_page = (uint16_t)raw_val;
                    break;
                case HID_GLOBAL_TAG_LOG_MIN:
                    global.log_min = sval;
                    break;
                case HID_GLOBAL_TAG_LOG_MAX:
                    global.log_max = sval;
                    break;
                case HID_GLOBAL_TAG_REPORT_SIZE:
                    global.report_size = raw_val;
                    break;
                case HID_GLOBAL_TAG_REPORT_COUNT:
                    global.report_count = raw_val;
                    break;
                case HID_GLOBAL_TAG_REPORT_ID:
                    global.report_id = (uint8_t)raw_val;
                    out_map->has_report_id = true;
                    out_map->report_id = global.report_id;
                    bit_offset = 8; // Report ID takes byte 0 on the wire
                    break;
                case HID_GLOBAL_TAG_PUSH:
                    if (stack_ptr < HID_GLOBAL_STACK_SIZE) {
                        global_stack[stack_ptr++] = global;
                    }
                    break;
                case HID_GLOBAL_TAG_POP:
                    if (stack_ptr > 0) {
                        global = global_stack[--stack_ptr];
                    }
                    break;
            }
        } else if (bType == HID_ITEM_TYPE_LOCAL) {
            switch (bTag) {
                case HID_LOCAL_TAG_USAGE:
                    if (local_usage_count < 64) {
                        local_usages[local_usage_count++] = (uint16_t)raw_val;
                    }
                    break;
                case HID_LOCAL_TAG_USAGE_MIN:
                    local_usage_min = (uint16_t)raw_val;
                    break;
                case HID_LOCAL_TAG_USAGE_MAX:
                    local_usage_max = (uint16_t)raw_val;
                    break;
            }
        } else if (bType == HID_ITEM_TYPE_MAIN) {
            if (bTag == HID_MAIN_TAG_INPUT) {
                bool is_constant = (raw_val & 0x01) != 0;

                if (is_constant) {
                    // Padding bits
                    bit_offset += global.report_size * global.report_count;
                } else {
                    for (uint32_t k = 0; k < global.report_count; k++) {
                        uint16_t usage = 0;
                        if (local_usage_count > 0) {
                            usage = (k < local_usage_count) ? local_usages[k] : local_usages[local_usage_count - 1];
                        } else if (local_usage_min != 0 && local_usage_max >= local_usage_min) {
                            usage = local_usage_min + k;
                            if (usage > local_usage_max) usage = local_usage_max;
                        }

                        if (global.usage_page == HID_UP_GENERIC_DESKTOP) {
                            if (usage == HID_USAGE_X && !out_map->has_x) {
                                out_map->has_x = true;
                                out_map->x_bit_offset = (uint16_t)bit_offset;
                                out_map->x_bit_size = (uint8_t)global.report_size;
                                out_map->x_is_signed = (global.log_min < 0);
                                out_map->x_logical_min = global.log_min;
                                out_map->x_logical_max = global.log_max;
                            } else if (usage == HID_USAGE_Y && !out_map->has_y) {
                                out_map->has_y = true;
                                out_map->y_bit_offset = (uint16_t)bit_offset;
                                out_map->y_bit_size = (uint8_t)global.report_size;
                                out_map->y_is_signed = (global.log_min < 0);
                                out_map->y_logical_min = global.log_min;
                                out_map->y_logical_max = global.log_max;
                            } else if (usage == HID_USAGE_WHEEL && !out_map->has_wheel) {
                                out_map->has_wheel = true;
                                out_map->wheel_bit_offset = (uint16_t)bit_offset;
                                out_map->wheel_bit_size = (uint8_t)global.report_size;
                                out_map->wheel_is_signed = (global.log_min < 0);
                            }
                        } else if (global.usage_page == HID_UP_BUTTONS) {
                            uint8_t btn_idx = (usage >= 1) ? (uint8_t)(usage - 1) : out_map->button_count;
                            if (btn_idx < HID_MAX_BUTTONS) {
                                out_map->button_bit_offset[btn_idx] = (uint16_t)bit_offset;
                                if (btn_idx >= out_map->button_count) {
                                    out_map->button_count = btn_idx + 1;
                                }
                            }
                        }

                        bit_offset += global.report_size;
                    }
                }

                local_usage_count = 0;
                local_usage_min = 0;
                local_usage_max = 0;
            } else if (bTag == HID_MAIN_TAG_OUTPUT || bTag == HID_MAIN_TAG_FEATURE) {
                local_usage_count = 0;
                local_usage_min = 0;
                local_usage_max = 0;
            }
        }
    }

    out_map->report_length_bits = bit_offset;

    if (out_map->has_x && out_map->has_y) {
        out_map->valid = true;
        display_print("[HRE] Mouse Report Descriptor Parsed Successfully!\n");
        display_print("[HRE] ReportID=");
        display_print_dec(out_map->has_report_id ? out_map->report_id : 0);
        display_print(" X: Bit=");
        display_print_dec(out_map->x_bit_offset);
        display_print(" Size=");
        display_print_dec(out_map->x_bit_size);
        display_print(" Y: Bit=");
        display_print_dec(out_map->y_bit_offset);
        display_print(" Size=");
        display_print_dec(out_map->y_bit_size);
        display_print(" Buttons=");
        display_print_dec(out_map->button_count);
        display_print(" Wheel=");
        display_print(out_map->has_wheel ? "YES\n" : "NO\n");
        return true;
    }

    display_print("[HRE] Warning: Descriptor did not declare both X and Y motion axes.\n");
    return false;
}

/* =========================================================================
 * Universal Mouse Report Decoder
 * Applies parsed field map to raw incoming packet with ZERO hardcoded offsets
 * ========================================================================= */
bool hid_decode_mouse_report(const hid_mouse_field_map_t* map, const uint8_t* report, uint32_t length_bytes,
                             int32_t* out_dx, int32_t* out_dy, uint8_t* out_buttons, int32_t* out_wheel) {
    if (!map || !map->valid || !report || length_bytes == 0) return false;

    // Check Report ID match
    if (map->has_report_id) {
        if (report[0] != map->report_id) {
            return false; // Belongs to different collection
        }
    }

    // Extract Buttons (up to 8 buttons supported by current HIDA format)
    uint8_t btns = 0;
    for (uint8_t b = 0; b < map->button_count && b < 8; b++) {
        int32_t state = hid_extract_field(report, length_bytes, map->button_bit_offset[b], 1, false);
        if (state) {
            btns |= (1 << b);
        }
    }
    *out_buttons = btns;

    // Extract X and Y Axis deltas
    *out_dx = hid_extract_field(report, length_bytes, map->x_bit_offset, map->x_bit_size, map->x_is_signed);
    *out_dy = hid_extract_field(report, length_bytes, map->y_bit_offset, map->y_bit_size, map->y_is_signed);

    // Extract Scroll Wheel
    if (map->has_wheel) {
        *out_wheel = hid_extract_field(report, length_bytes, map->wheel_bit_offset, map->wheel_bit_size, map->wheel_is_signed);
    } else {
        *out_wheel = 0;
    }

    return true;
}
